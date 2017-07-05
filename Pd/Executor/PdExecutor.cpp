#include <Pd/Executor/PdExecutor.hpp>

#include <Explorer/DocumentPlugin/DeviceDocumentPlugin.hpp>
#include <Engine/OSSIA2iscore.hpp>
#include <Engine/iscore2OSSIA.hpp>
#include <Engine/Executor/DocumentPlugin.hpp>
#include <vector>

#include <ossia/editor/state/message.hpp>
#include <ossia/editor/state/state.hpp>
#include <ossia/editor/scenario/time_constraint.hpp>
#include <ossia/dataflow/node_process.hpp>
#include <Pd/PdProcess.hpp>
#include <QFileInfo>
namespace Pd
{


struct libpd_list_wrapper
{
  t_atom* impl{};
  int size{};

  template<typename Functor>
  void iterate(const Functor& f) const {
    for(auto it = impl; it; it = libpd_next_atom(it))
    {
      if(libpd_is_float(it))
        f(libpd_get_float(it));
      else if(libpd_is_symbol(it))
        f(libpd_get_symbol(it));
    }
  }

  struct value_visitor
  {
    std::vector<ossia::value>& v;
    void operator()(float f) const
    { v.push_back(f); }
    void operator()(const char* s) const
    { v.push_back(std::string(s)); }
  };

  std::vector<ossia::value> to_tuple()
  {
    std::vector<ossia::value> vals;
    vals.reserve(size);
    iterate(value_visitor{vals});
    return vals;
  }
};


PdGraphNode::PdGraphNode(
    ossia::string_view folder, ossia::string_view file,
    std::size_t inputs, std::size_t outputs,
    std::vector<std::string> in_val, std::vector<std::string> out_val,
    bool midi_in, bool midi_out)
  : m_inputs{inputs}
  , m_outputs{outputs}
  , m_inmess{std::move(in_val)}
  , m_outmess{std::move(out_val)}
  , m_file{file}
{
  m_currentInstance = nullptr;

  // Set-up buffers
  const std::size_t bs = libpd_blocksize();
  m_inbuf.resize(m_inputs * bs);
  m_outbuf.resize(m_outputs * bs);

  // Create instance
  m_instance = pdinstance_new();
  pd_setinstance(m_instance);

  // Enable audio
  libpd_start_message(1);
  libpd_add_float(1.0f);
  libpd_finish_message("pd", "dsp");

  // Open
  auto handle = libpd_openfile(file.data(), folder.data());
  m_dollarzero = libpd_getdollarzero(handle);

  for(auto& mess : m_inmess)
    add_dzero(mess);
  for(auto& mess : m_outmess)
    add_dzero(mess);

  // Create correct I/Os
  m_inlets.reserve(inputs + m_inmess.size() + int(midi_in));
  for(std::size_t i = 0U; i < inputs ; i++)
    m_inlets.push_back(ossia::make_inlet<ossia::audio_port>());
  for(auto& str : m_inmess)
  {
    m_inlets.push_back(ossia::make_inlet<ossia::value_port>());
  }

  m_outlets.reserve(outputs + m_outmess.size()+ int(midi_out));
  for(std::size_t i = 0U; i < outputs ; i++)
    m_outlets.push_back(ossia::make_outlet<ossia::audio_port>());
  for(auto& mess : m_outmess)
  {
    libpd_bind(mess.c_str());
    m_outlets.push_back(ossia::make_outlet<ossia::value_port>());
  }

  if(midi_in)
  {
    auto mid = ossia::make_inlet<ossia::midi_port>();
    m_midi_inlet = mid->data.target<ossia::midi_port>();
    m_inlets.push_back(std::move(mid));
  }
  if(midi_out)
  {
    auto mid = ossia::make_outlet<ossia::midi_port>();
    m_midi_outlet = mid->data.target<ossia::midi_port>();
    m_outlets.push_back(std::move(mid));
  }

  // Set-up message callbacks
  libpd_set_printhook([] (const char *s) { qDebug() << "string: " << s; });

  libpd_set_floathook([] (const char *recv, float f) {
    if(auto v = m_currentInstance->get_value_port(recv))
    {
      v->data.push_back(f);
    }
  });
  libpd_set_banghook([] (const char *recv) {
    if(auto v = m_currentInstance->get_value_port(recv))
    {
      v->data.push_back(ossia::impulse{});
    }
  });
  libpd_set_symbolhook([] (const char *recv, const char *sym) {
    if(auto v = m_currentInstance->get_value_port(recv))
    {
      v->data.push_back(std::string(sym));
    }
  });

  libpd_set_listhook([] (const char *recv, int argc, t_atom *argv) {
    if(auto v = m_currentInstance->get_value_port(recv))
    {
      v->data.push_back(libpd_list_wrapper{argv, argc}.to_tuple());
    }
  });
  libpd_set_messagehook([] (const char *recv, const char *msg, int argc, t_atom *argv) {
    if(auto v = m_currentInstance->get_value_port(recv))
    {
      v->data.push_back(libpd_list_wrapper{argv, argc}.to_tuple());
    }
  });

  libpd_set_noteonhook([] (int channel, int pitch, int velocity) {
    if(auto v = m_currentInstance->get_midi_out())
    {
      v->messages.push_back(
            (velocity > 0)
            ? mm::MakeNoteOn(channel, pitch, velocity)
            : mm::MakeNoteOff(channel, pitch, velocity)
              );
    }
  });
  libpd_set_controlchangehook([] (int channel, int controller, int value) {
    if(auto v = m_currentInstance->get_midi_out())
    {
      v->messages.push_back(mm::MakeControlChange(channel, controller, value + 8192));
    }
  });

  libpd_set_programchangehook([] (int channel, int value) {
    if(auto v = m_currentInstance->get_midi_out())
    {
      v->messages.push_back(mm::MakeProgramChange(channel, value));
    }
  });
  libpd_set_pitchbendhook([] (int channel, int value) {
    if(auto v = m_currentInstance->get_midi_out())
    {
      v->messages.push_back(mm::MakePitchBend(channel, value));
    }
  });
  libpd_set_aftertouchhook([] (int channel, int value) {
    if(auto v = m_currentInstance->get_midi_out())
    {
      v->messages.push_back(mm::MakeAftertouch(channel, value));
    }
  });
  libpd_set_polyaftertouchhook([] (int channel, int pitch, int value) {
    if(auto v = m_currentInstance->get_midi_out())
    {
      v->messages.push_back(mm::MakePolyPressure(channel, pitch, value));
    }
  });
  libpd_set_midibytehook([] (int port, int byte) {
    // TODO
  });


}


PdGraphNode::~PdGraphNode()
{
  pdinstance_free(m_instance);
}


ossia::outlet*PdGraphNode::get_outlet(const char* str) const
{
  ossia::string_view s = str;
  auto it = ossia::find(m_outmess, s);
  if(it != m_outmess.end())
    return m_outlets[std::distance(m_outmess.begin(), it) + m_outputs].get();
  return nullptr;
}


ossia::value_port*PdGraphNode::get_value_port(const char* str) const
{
  return get_outlet(str)->data.target<ossia::value_port>();
}


ossia::midi_port*PdGraphNode::get_midi_in() const
{
  return m_midi_inlet;
}


ossia::midi_port*PdGraphNode::get_midi_out() const
{
  return m_midi_outlet;
}


void PdGraphNode::run(ossia::execution_state& e)
{
  // Setup
  pd_setinstance(m_instance);
  m_currentInstance = this;
  libpd_init_audio(m_inputs, m_outputs, 44100);

  const auto bs = libpd_blocksize();

  // Copy audio inputs
  for(std::size_t i = 0U; i < m_inputs; i++)
  {
    auto ap = m_inlets[i]->data.target<ossia::audio_port>();
    const auto pos = i * bs;
    if(!ap->samples.empty())
    {
      auto& arr = ap->samples.back();
      std::copy_n(arr.begin(), bs, m_inbuf.begin() + pos);
    }
  }

  // Copy midi inputs
  if(m_midi_inlet)
  {
    auto& dat = m_midi_inlet->messages;
    for(const auto& mess : dat)
    {
      switch(mess.getMessageType())
      {
        case mm::MessageType::NOTE_OFF:
          libpd_noteon(mess.getChannel(), mess.data[1], 0);
          break;
        case mm::MessageType::NOTE_ON:
          libpd_noteon(mess.getChannel(), mess.data[1], mess.data[2]);
          break;
        case mm::MessageType::POLY_PRESSURE:
          libpd_polyaftertouch(mess.getChannel(), mess.data[1], mess.data[2]);
          break;
        case mm::MessageType::CONTROL_CHANGE:
          libpd_controlchange(mess.getChannel(), mess.data[1], mess.data[2]);
          break;
        case mm::MessageType::PROGRAM_CHANGE:
          libpd_programchange(mess.getChannel(), mess.data[1]);
          break;
        case mm::MessageType::AFTERTOUCH:
          libpd_aftertouch(mess.getChannel(), mess.data[1]);
          break;
        case mm::MessageType::PITCH_BEND:
          libpd_pitchbend(mess.getChannel(), mess.data[1] - 8192);
          break;
        case mm::MessageType::INVALID:
        default:
          break;
      }
    }

    dat.clear();
  }

  // Copy message inputs
  for(std::size_t i = m_inputs; i < m_inlets.size(); ++i)
  {
    auto& dat = m_inlets[i]->data.target<ossia::value_port>()->data;

    auto mess = m_inmess[i - m_inputs].c_str();
    for(const auto& val : dat)
    {
      val.apply(ossia_to_pd_value{mess});
    }
    dat.clear();
  }

  // Process
  libpd_process_raw(m_inbuf.data(), m_outbuf.data());

  // Copy audio outputs. Message inputs are copied in callbacks.
  for(std::size_t i = 0U; i < m_outputs; ++i)
  {
    auto ap = m_outlets[i]->data.target<ossia::audio_port>();
    ap->samples.resize(ap->samples.size() + 1);

    std::copy_n(m_outbuf.begin() + i * bs, bs, ap->samples.back().begin());
  }

  // Teardown
  m_currentInstance = nullptr;
}


void PdGraphNode::add_dzero(std::string& s) const
{
  s = std::to_string(m_dollarzero) + "-" + s;
}




Component::Component(
    ::Engine::Execution::ConstraintComponent& parentConstraint,
    Pd::ProcessModel& element,
    const Dataflow::DocumentPlugin& df,
    const ::Engine::Execution::Context& ctx,
    const Id<iscore::Component>& id,
    QObject* parent):
  DataflowProcessComponent{
      parentConstraint, element, ctx, id, "PdComponent", parent}
{
  QFileInfo f(element.script());
  if(!f.exists())
  {
    qDebug() << "Missing script. Returning";
    return;
  }

  std::vector<ossia::net::address_base*> inlet_addresses, outlet_addresses;
  const std::vector<Process::Port>& model_inlets = element.inlets();
  const std::vector<Process::Port>& model_outlets = element.outlets();
  inlet_addresses.resize(model_inlets.size());
  outlet_addresses.resize(model_outlets.size());

  std::vector<std::string> in_mess, out_mess;
  for(std::size_t i = 0; i < model_inlets.size(); i++)
  {
    const Process::Port& e = model_inlets[i];
    if(e.type == Process::PortType::Message)
      in_mess.push_back(e.customData.toStdString());

    inlet_addresses[i] = df.resolve(e.address);
  }

  for(std::size_t i = 0; i < model_outlets.size(); i++)
  {
    const Process::Port& e = model_outlets[i];
    if(e.type == Process::PortType::Message)
      out_mess.push_back(e.customData.toStdString());

    outlet_addresses[i] = df.resolve(e.address);
  }

  qDebug() << "inlets";
  for(auto addr : inlet_addresses)
    qDebug() << (addr ? ossia::net::address_string_from_node(*addr).c_str() : "No addr");

  qDebug() << "outlets";
  for(auto addr : outlet_addresses)
    qDebug() << (addr ? ossia::net::address_string_from_node(*addr).c_str() : "No addr");

  node = std::make_shared<PdGraphNode>(
        f.canonicalPath().toStdString(),
        f.fileName().toStdString(),
        element.audioInlets(), element.audioOutlets(),
        std::move(in_mess), std::move(out_mess),
        false, false
        );

  for(std::size_t i = 0; i < inlet_addresses.size(); i++)
  {
    if(auto addr = inlet_addresses[i])
      node->inputs()[i]->address = addr;
  }
  for(std::size_t i = 0; i < outlet_addresses.size(); i++)
  {
    if(auto addr = outlet_addresses[i])
      node->outputs()[i]->address = addr;
  }
  df.execGraph->add_node(node);

  m_ossia_process =
      std::make_shared<ossia::node_process>(df.execGraph, node);


  ///  Connect the cables
  ///
  for(auto id : element.cables)
  {
    std::cerr << "\n\nConnect 1\n";
    auto& cable = df.cables.at(id);
    // Check if there is already a cable
    if(!cable.exec)
    {
      std::cerr << "\n\nConnect 2\n";
      if(cable.source == &element)
        cable.source_node = node;
      if(cable.sink == &element)
        cable.sink_node = node;

      std::cerr << cable.source_node.get() << " && " << cable.sink_node.get() << "\n";
      if(cable.source_node && cable.sink_node && cable.inlet && cable.outlet)
      {
        std::cerr << "\n\nConnect 3\n";
        auto& outlet = cable.source_node->outputs()[*cable.outlet];
        auto& inlet = cable.sink_node->inputs()[*cable.inlet];
        switch(cable.type)
        {
          case Process::CableType::ImmediateStrict:
          {
           std::cerr << "\n\nConnect 4\n";
           cable.exec = ossia::make_edge(
                 ossia::immediate_strict_connection{},
                 outlet, inlet, cable.source_node, cable.sink_node);
            break;
          }
          case Process::CableType::ImmediateGlutton:
          {
           cable.exec = ossia::make_edge(
                 ossia::immediate_glutton_connection{},
                 outlet, inlet, cable.source_node, cable.sink_node);
            break;
          }
          case Process::CableType::DelayedStrict:
          {
           cable.exec = ossia::make_edge(
                 ossia::delayed_strict_connection{},
                 outlet, inlet, cable.source_node, cable.sink_node);
            break;
          }
          case Process::CableType::DelayedGlutton:
          {
           cable.exec = ossia::make_edge(
                 ossia::delayed_glutton_connection{},
                 outlet, inlet, cable.source_node, cable.sink_node);
            break;
          }
        }

        df.execGraph->connect(cable.exec);
      }
    }
  }

}

PdGraphNode* PdGraphNode::m_currentInstance;

}

