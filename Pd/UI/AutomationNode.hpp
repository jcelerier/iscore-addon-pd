#pragma once
#include <Automation/AutomationModel.hpp>
#include <Pd/DocumentPlugin.hpp>
#include <Engine/Executor/ProcessComponent.hpp>
#include <Process/Dataflow/DataflowObjects.hpp>
#include <Process/Process.hpp>

#include <ossia/editor/automation/automation.hpp>
#include <ossia/editor/value/value.hpp>

#include <iscore_addon_pd_export.h>

namespace Dataflow
{


class AutomationComponent :
    public ProcessComponent_T<Automation::ProcessModel>
{
  COMPONENT_METADATA("17b049e1-222e-4f86-b879-eccb05937cbb")
  public:
    AutomationComponent(
        Automation::ProcessModel& autom,
        DocumentPlugin& doc,
        const Id<iscore::Component>& id,
        QObject* parent_obj);

    std::size_t audioInlets() const override { return 0; }
    std::size_t messageInlets() const override { return 0; }
    std::size_t midiInlets() const override { return 0; }

    std::size_t audioOutlets() const override { return 0; }
    std::size_t messageOutlets() const override{ return 1; }
    std::size_t midiOutlets() const override{ return 0; }

    std::vector<Process::Port> inlets() const override { return {}; }
    std::vector<Process::Port> outlets() const override {
      std::vector<Process::Port> v(1);
      Process::Port& p = v[0];
      p.address = process().address();
      p.type = Process::PortType::Message;
      return v;
    }

    ~AutomationComponent()
    {
      cleanup();
    }

    std::vector<Id<Process::Cable>> cables() const override
    { return m_cables; }
    void addCable(Id<Process::Cable> c) override
    { m_cables.push_back(c); }
    void removeCable(Id<Process::Cable> c) override
    { m_cables.erase(ossia::find(m_cables, c)); }

  private:
    std::vector<Id<Process::Cable>> m_cables;
};

using AutomationComponentFactory = ProcessComponentFactory_T<AutomationComponent>;

class ISCORE_ADDON_PD_EXPORT AutomationGraphNode final :
    public ossia::graph_node
{
public:
  AutomationGraphNode(
      ossia::Destination dest,
      std::shared_ptr<ossia::curve_abstract> curve):
    m_dest{dest}
  , m_curve{curve}
  {

  }

  ~AutomationGraphNode()
  {

  }

private:
  void run(ossia::execution_state& e) override;

  ossia::Destination m_dest;
  std::shared_ptr<ossia::curve_abstract> m_curve{};
};

class ExecComponent final
    : public ::Engine::Execution::
          ProcessComponent_T<Automation::ProcessModel, ossia::automation>
{
  COMPONENT_METADATA("f3ac9746-e994-42cc-a3d5-cfef89bcb7aa")
public:
  ExecComponent(
      ::Engine::Execution::ConstraintComponent& parentConstraint,
      Automation::ProcessModel& element,
      const Dataflow::DocumentPlugin& df,
      const ::Engine::Execution::Context& ctx,
      const Id<iscore::Component>& id,
      QObject* parent);

private:
  void recompute();

  std::shared_ptr<ossia::curve_abstract>
  on_curveChanged(ossia::val_type, const optional<ossia::Destination>&);

  template <typename T>
  std::shared_ptr<ossia::curve_abstract>
  on_curveChanged_impl(const optional<ossia::Destination>&);

  std::shared_ptr<ossia::graph_node> node;
  const Dataflow::DocumentPlugin& m_df;
};
using ExecComponentFactory
    = ::Engine::Execution::ProcessComponentFactory_T<ExecComponent>;

}
