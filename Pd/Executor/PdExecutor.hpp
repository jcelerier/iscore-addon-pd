#pragma once
#define PDINSTANCE

#include <ossia/editor/scenario/time_process.hpp>
#include <QJSEngine>
#include <QJSValue>
#include <QString>
#include <memory>
#include <Engine/Executor/ProcessComponent.hpp>
#include <Engine/Executor/ExecutorContext.hpp>
#include <score/document/DocumentContext.hpp>
#include <score/document/DocumentInterface.hpp>
#include <ossia/editor/scenario/time_value.hpp>
#include <Device/Protocol/DeviceList.hpp>
#include <z_libpd.h>
#include <Dataflow/DocumentPlugin.hpp>
#include <ossia/detail/string_view.hpp>
#include <boost/circular_buffer.hpp>
#include <score_addon_pd_export.h>

namespace Pd
{


class ProcessModel;

class SCORE_ADDON_PD_EXPORT PdGraphNode final :
    public ossia::graph_node
{
public:
  PdGraphNode(
      ossia::string_view folder, ossia::string_view file,
      const Engine::Execution::Context& ctx,
      std::size_t audio_inputs,
      std::size_t audio_outputs,
      std::vector<Process::Port*> inmess,
      std::vector<Process::Port*> outmess,
      bool midi_in = true,
      bool midi_out = true
      );

  ~PdGraphNode();

private:
  ossia::outlet* get_outlet(const char* str) const;

  ossia::value_port* get_value_port(const char* str) const;

  ossia::midi_port* get_midi_in() const;
  ossia::midi_port* get_midi_out() const;

  struct ossia_to_pd_value
  {
    const char* mess{};
    void operator()() const { }
    template<typename T>
    void operator()(const T&) const { }

    void operator()(float f) const { libpd_float(mess, f); }
    void operator()(int f) const { libpd_float(mess, f); }
    void operator()(const std::string& f) const { libpd_symbol(mess, f.c_str()); }
    void operator()(const ossia::impulse& f) const { libpd_bang(mess); }

    // TODO convert other types
  };

  void run(ossia::token_request t, ossia::execution_state& e) override;
  void add_dzero(std::string& s) const;

  static PdGraphNode* m_currentInstance;

  t_pdinstance * m_instance{};
  int m_dollarzero = 0;

  std::size_t m_audioIns{};
  std::size_t m_audioOuts{};
  std::vector<Process::Port*> m_inport, m_outport;
  std::vector<std::string> m_inmess, m_outmess;

  std::vector<float> m_inbuf, m_outbuf;
  std::vector<boost::circular_buffer<float>> m_prev_outbuf;
  std::size_t m_firstInMessage{}, m_firstOutMessage{};
  ossia::audio_port* m_audio_inlet{};
  ossia::audio_port* m_audio_outlet{};
  ossia::midi_port* m_midi_inlet{};
  ossia::midi_port* m_midi_outlet{};
  std::string m_file;
};


class DataflowProcessComponent :
    public Engine::Execution::ProcessComponent
{

    public:
        using Engine::Execution::ProcessComponent::ProcessComponent;

    ~DataflowProcessComponent()
    {
      if(node) node->clear();
    }

  ossia::node_ptr node;
};

class Component final :
    public DataflowProcessComponent
{
        COMPONENT_METADATA("78657f42-3a2a-4b80-8736-8736463442b4")

    public:
        using model_type = Pd::ProcessModel;
        Component(
                Engine::Execution::IntervalComponent& parentInterval,
                Pd::ProcessModel& element,
                const Engine::Execution::Context& ctx,
                const Id<score::Component>& id,
                QObject* parent);
};


using ComponentFactory = Engine::Execution::ProcessComponentFactory_T<Pd::Component>;
}
