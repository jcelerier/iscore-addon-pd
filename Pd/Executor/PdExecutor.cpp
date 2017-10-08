#include <Pd/Executor/PdExecutor.hpp>

#include <Explorer/DocumentPlugin/DeviceDocumentPlugin.hpp>
#include <Engine/OSSIA2score.hpp>
#include <Engine/score2OSSIA.hpp>
#include <Engine/Executor/DocumentPlugin.hpp>
#include <vector>

#include <ossia/editor/state/message.hpp>
#include <ossia/editor/state/state.hpp>
#include <ossia/dataflow/node_process.hpp>
#include <Pd/PdProcess.hpp>
#include <QFileInfo>
#include <Pd/UI/PdNode.hpp>
#include <Engine/score2OSSIA.hpp>
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

  std::vector<ossia::value> to_list()
  {
    std::vector<ossia::value> vals;
    vals.reserve(size);
    iterate(value_visitor{vals});
    return vals;
  }
};


PdGraphNode::PdGraphNode(
    ossia::string_view folder, ossia::string_view file,
    const Engine::Execution::Context& ctx,
    std::size_t audio_inputs,
    std::size_t audio_outputs,
    std::vector<Process::Port*> inport,
    std::vector<Process::Port*> outport,
    bool midi_in, bool midi_out)
  : m_audioIns{audio_inputs}
  , m_audioOuts{audio_outputs}
  , m_file{file}
{
  m_currentInstance = nullptr;

  for(auto proc : inport) {
    if(proc->type == Process::PortType::Message)
      m_inmess.push_back(proc->customData().toStdString());
  }
  for(auto proc : outport) {
    if(proc->type == Process::PortType::Message)
      m_outmess.push_back(proc->customData().toStdString());
  }
  // Set-up buffers
  const std::size_t bs = libpd_blocksize();
  m_inbuf.resize(m_audioIns * bs);
  m_outbuf.resize(m_audioOuts * bs);

  // Create instance
  m_instance = pdinstance_new();
  qDebug() << "PdGraphNode: Allocating " << m_instance;
  pd_setinstance(m_instance);

  // Enable audio
  libpd_init_audio(m_audioIns, m_audioOuts, 44100);

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
  bool hasAudioIn = m_audioIns > 0;
  bool hasAudioOut = m_audioOuts > 0;
  int audioInlets = hasAudioIn ? 1 : 0;
  int audioOutlets = hasAudioOut ? 1 : 0;
  m_inlets.reserve(audioInlets + m_inmess.size() + int(midi_in));
  if(hasAudioIn)
  {
    auto port = ossia::make_inlet<ossia::audio_port>();
    m_audio_inlet = port->data.target<ossia::audio_port>();
    if(auto addr = Engine::score_to_ossia::makeDestination(ctx.devices.list(), inport[0]->address()))
    {
      port->address = &addr->address();
    }
    m_inlets.push_back(std::move(port));
  }
  if(midi_in)
  {
    auto port = ossia::make_inlet<ossia::midi_port>();
    m_midi_inlet = port->data.target<ossia::midi_port>();
    if(auto addr = Engine::score_to_ossia::makeDestination(ctx.devices.list(), inport[1]->address()))
    {
      port->address = &addr->address();
    }
    m_inlets.push_back(std::move(port));
  }
  m_firstInMessage = m_inlets.size();
  for(std::size_t i = 0; i < m_inmess.size(); i++)
  {
    auto port = ossia::make_inlet<ossia::value_port>();
    if(auto addr = Engine::score_to_ossia::makeDestination(ctx.devices.list(), inport[m_firstInMessage + i]->address()))
    {
      port->address = &addr->address();
    }
    m_inlets.push_back(port);
  }

  m_outlets.reserve(audioOutlets + m_outmess.size() + int(midi_out));
  if(hasAudioOut)
  {
    auto port = ossia::make_outlet<ossia::audio_port>();
    m_audio_outlet = port->data.target<ossia::audio_port>();
    if(auto addr = Engine::score_to_ossia::makeDestination(ctx.devices.list(), outport[0]->address()))
    {
      port->address = &addr->address();
    }
    m_outlets.push_back(std::move(port));
  }
  if(midi_out)
  {
    auto port = ossia::make_outlet<ossia::midi_port>();
    m_midi_outlet = port->data.target<ossia::midi_port>();
    if(auto addr = Engine::score_to_ossia::makeDestination(ctx.devices.list(), outport[1]->address()))
    {
      port->address = &addr->address();
    }
    m_outlets.push_back(std::move(port));
  }
  m_firstOutMessage = m_outlets.size();
  for(std::size_t i = 0; i < m_outmess.size(); i++)
  {
    libpd_bind(m_outmess[i].c_str());
    auto port = ossia::make_outlet<ossia::value_port>();
    if(auto addr = Engine::score_to_ossia::makeDestination(ctx.devices.list(), outport[m_firstOutMessage + i]->address()))
    {
      port->address = &addr->address();
    }
    m_outlets.push_back(port);
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
      v->data.push_back(libpd_list_wrapper{argv, argc}.to_list());
    }
  });
  libpd_set_messagehook([] (const char *recv, const char *msg, int argc, t_atom *argv) {
    if(auto v = m_currentInstance->get_value_port(recv))
    {
      v->data.push_back(libpd_list_wrapper{argv, argc}.to_list());
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
  qDebug() << "PdGraphNode: Freeeing " << m_instance;
  pdinstance_free(m_instance);
}


ossia::outlet*PdGraphNode::get_outlet(const char* str) const
{
  ossia::string_view s = str;
  auto it = ossia::find(m_outmess, s);
  if(it != m_outmess.end())
    return m_outlets[std::distance(m_outmess.begin(), it) + m_audioOuts].get();

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
  libpd_init_audio(m_audioIns, m_audioOuts, 44100);

  const auto bs = libpd_blocksize();

  // Copy audio inputs
  if(m_audioIns > 0)
  {
    auto ap = m_inlets[0]->data.target<ossia::audio_port>();

    for(std::size_t i = 0U; i < m_audioIns; i++)
    {
      std::copy_n(ap->samples[i].begin(), bs, m_inbuf.begin() + i * bs);
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
  for(std::size_t i = 0; i < m_inmess.size(); ++i)
  {
    auto& dat = m_inlets[m_firstInMessage + i]->data.target<ossia::value_port>()->data;

    auto mess = m_inmess[i].c_str();
    for(const auto& val : dat)
    {
      val.apply(ossia_to_pd_value{mess});
    }
    dat.clear();
  }

  // Process
  libpd_process_raw(m_inbuf.data(), m_outbuf.data());

  if(m_audioOuts > 0)
  {
    auto ap = m_outlets[0]->data.target<ossia::audio_port>();
    ap->samples.clear();
    ap->samples.resize(m_audioOuts);
    // Copy audio outputs. Message inputs are copied in callbacks.
    for(std::size_t i = 0U; i < m_audioOuts; ++i)
    {
      ap->samples[i].resize(bs);
      std::copy_n(m_outbuf.begin() + i * bs, bs, ap->samples[i].begin());
    }
  }

  // Teardown
  m_currentInstance = nullptr;
}


void PdGraphNode::add_dzero(std::string& s) const
{
  s = std::to_string(m_dollarzero) + "-" + s;
}


Component::Component(
    ::Engine::Execution::IntervalComponent& parentInterval,
    Pd::ProcessModel& element,
    const ::Engine::Execution::Context& ctx,
    const Id<score::Component>& id,
    QObject* parent):
  DataflowProcessComponent{
      parentInterval, element, ctx, id, "PdComponent", parent}
{
  QFileInfo f(element.script());
  if(!f.exists())
  {
    qDebug() << "Missing script. Returning";
    return;
  }

  const std::vector<Process::Port*>& model_inlets = element.inlets();
  const std::vector<Process::Port*>& model_outlets = element.outlets();

  std::vector<std::string> in_mess, out_mess;
  for(std::size_t i = 0; i < model_inlets.size(); i++)
  {
    const Process::Port& e = *model_inlets[i];
    if(e.type == Process::PortType::Message)
      in_mess.push_back(e.customData().toStdString());

  }

  for(std::size_t i = 0; i < model_outlets.size(); i++)
  {
    const Process::Port& e = *model_outlets[i];
    if(e.type == Process::PortType::Message)
      out_mess.push_back(e.customData().toStdString());
  }

  node = std::make_shared<PdGraphNode>(
        f.canonicalPath().toStdString(),
        f.fileName().toStdString(),
           ctx,
        element.audioInputs(), element.audioOutputs(),
        model_inlets, model_outlets,
        element.midiInput(), element.midiOutput()
        );

  m_ossia_process =
      std::make_shared<ossia::node_process>(ctx.plugin.execGraph, node);

  for(auto p : model_inlets)
    ctx.plugin.nodes.insert({p, node});
  for(auto p : model_outlets)
    ctx.plugin.nodes.insert({p, node});

  ctx.plugin.execGraph->add_node(node);
}

PdGraphNode* PdGraphNode::m_currentInstance{};

}

