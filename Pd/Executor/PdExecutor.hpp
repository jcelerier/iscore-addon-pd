#pragma once
#define PDINSTANCE

#include <ossia/editor/scenario/time_process.hpp>
#include <QJSEngine>
#include <QJSValue>
#include <QString>
#include <memory>
#include <Engine/Executor/ProcessComponent.hpp>
#include <Engine/Executor/ExecutorContext.hpp>
#include <iscore/document/DocumentContext.hpp>
#include <iscore/document/DocumentInterface.hpp>
#include <ossia/editor/scenario/time_value.hpp>
#include <Device/Protocol/DeviceList.hpp>
#include <z_libpd.h>
#include <Pd/DocumentPlugin.hpp>
#include <ossia/detail/string_view.hpp>
#include <iscore_addon_pd_export.h>

namespace Pd
{

template <typename ProcessComponent_T>
class ProcessComponentFactory_T
    : public iscore::
          GenericComponentFactoryImpl<ProcessComponent_T, Engine::Execution::ProcessComponentFactory>
{
public:
  using model_type = typename ProcessComponent_T::model_type;

  std::shared_ptr<Engine::Execution::ProcessComponent> make(
      Engine::Execution::ConstraintComponent& cst,
      Process::ProcessModel& proc,
      const Engine::Execution::Context& ctx,
      const Id<iscore::Component>& id,
      QObject* parent) const final override
  {
    auto& df_plug = ctx.doc.plugin<Dataflow::DocumentPlugin>();
    auto comp = std::make_shared<ProcessComponent_T>(
          cst, static_cast<typename ProcessComponent_T::model_type&>(proc), df_plug, ctx, id, parent);
    this->init(comp.get());
    return comp;
  }
};


class ProcessModel;

class ISCORE_ADDON_PD_EXPORT PdGraphNode final :
    public ossia::graph_node
{
public:
  PdGraphNode(
      ossia::string_view folder,
      ossia::string_view file,
      std::size_t audio_inputs,
      std::size_t audio_outputs,
      std::size_t message_inputs,
      std::size_t message_outputs,
      std::vector<std::string> in_val,
      std::vector<std::string> out_val,
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

  void run(ossia::execution_state& e) override;
  void add_dzero(std::string& s) const;

  static PdGraphNode* m_currentInstance;

  t_pdinstance * m_instance{};
  int m_dollarzero = 0;

  std::size_t m_audioIns{}, m_audioOuts{};
  std::size_t m_messageIns{}, m_messageOuts{};
  std::vector<std::string> m_inmess, m_outmess;
  std::vector<float> m_inbuf, m_outbuf;
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
                Engine::Execution::ConstraintComponent& parentConstraint,
                Pd::ProcessModel& element,
                const Dataflow::DocumentPlugin& df,
                const Engine::Execution::Context& ctx,
                const Id<iscore::Component>& id,
                QObject* parent);
};


using ComponentFactory = Pd::ProcessComponentFactory_T<Pd::Component>;
}
