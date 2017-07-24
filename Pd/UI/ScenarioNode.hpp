#pragma once
#include <Pd/DocumentPlugin.hpp>
#include <Pd/PdProcess.hpp>
#include <Scenario/Document/Components/ConstraintComponent.hpp>
#include <Scenario/Document/Components/ProcessComponent.hpp>
#include <Scenario/Document/Components/ScenarioComponent.hpp>
#include <Process/Process.hpp>
#include <Automation/AutomationModel.hpp>
#include <iscore/model/Component.hpp>
#include <iscore/model/ComponentHierarchy.hpp>
#include <iscore/plugins/customfactory/ModelFactory.hpp>
#include <iscore/model/ComponentFactory.hpp>
#include <iscore_addon_pd_export.h>
#include <Process/Dataflow/DataflowObjects.hpp>
namespace Dataflow
{
class NodeItem;

class DataflowProcessNode: public Process::Node
{
  public:
    DataflowProcessNode(
          Dataflow::DocumentPlugin& doc,
          Process::DataflowProcess& proc,
          Id<Node> c,
          QObject* parent);

    Process::DataflowProcess& process;
    ossia::node_ptr exec;

    ~DataflowProcessNode()
    {
      cleanup();
    }

    QString getText() const override { return process.metadata().getName(); }
    std::size_t audioInlets() const override { return process.audioInlets(); }
    std::size_t messageInlets() const override { return process.messageInlets(); }
    std::size_t midiInlets() const override { return process.midiInlets(); }

    std::size_t audioOutlets() const override { return process.audioOutlets(); }
    std::size_t messageOutlets() const override{ return process.messageOutlets(); }
    std::size_t midiOutlets() const override{ return process.midiOutlets(); }

    std::vector<Process::Port> inlets() const override { return process.inlets(); }
    std::vector<Process::Port> outlets() const override { return process.outlets(); }

    std::vector<Id<Process::Cable>> cables() const override { return process.cables; }
    void addCable(Id<Process::Cable> c) override { process.cables.push_back(c); }
    void removeCable(Id<Process::Cable> c) override {
      auto it = ossia::find(process.cables, c);
      if(it != process.cables.end())
        process.cables.erase(it);
    }


};

class ISCORE_ADDON_PD_EXPORT DataFlowProcessComponent : public ProcessComponent_T<Process::DataflowProcess>
{
  public:
    DataFlowProcessComponent(
        Process::DataflowProcess& process,
        DocumentPlugin& doc,
        const Id<iscore::Component>& id,
        const QString& name,
        QObject* parent);

    ~DataFlowProcessComponent();

    DataflowProcessNode& node() { return m_node; }
  private:
    DataflowProcessNode m_node;
};

class ConstraintBase :
    public Scenario::GenericConstraintComponent<Dataflow::DocumentPlugin>
{
    COMMON_COMPONENT_METADATA("eab98b28-5b0f-4754-aa3a-8d3622eedeea")
    public:
      using parent_t = Scenario::GenericConstraintComponent<Dataflow::DocumentPlugin>;
    using DocumentPlugin = Dataflow::DocumentPlugin;
    using model_t = Process::ProcessModel;
    using component_t = Dataflow::ProcessComponent;
    using component_factory_list_t = Dataflow::ProcessComponentFactoryList;

    ConstraintBase(
        const Id<iscore::Component>& id,
        Scenario::ConstraintModel& constraint,
        DocumentPlugin& doc,
        QObject* parent_comp);

    ProcessComponent* make(
        const Id<iscore::Component> & id,
        ProcessComponentFactory& factory,
        Process::ProcessModel &process);

    bool removing(const Process::ProcessModel& cst, const ProcessComponent& comp);

    template <typename... Args>
    void removed(Args&&...)
    {
    }
};

class Constraint final :
    public iscore::PolymorphicComponentHierarchy<ConstraintBase>
{
  public:
    using iscore::PolymorphicComponentHierarchy<ConstraintBase>::PolymorphicComponentHierarchyManager;

};

class ScenarioBase :
    public ProcessComponent_T<Scenario::ProcessModel>
{
    COMPONENT_METADATA("3ae53c95-fa10-47fa-a04f-eb766e3095f7")

    public:
      ScenarioBase(
        Scenario::ProcessModel& scenario,
        DocumentPlugin& doc,
        const Id<iscore::Component>& id,
        QObject* parent_obj):
    ProcessComponent_T<::Scenario::ProcessModel>{scenario, doc, id, "ScenarioComponent", parent_obj}
{
}

template<typename Component_T, typename Element>
Component_T* make(
    const Id<iscore::Component>& id,
    Element& elt)
{
  return new Component_T{id, elt, system(), this};
}

template<typename... Args>
bool removing(Args&&...) { return true; }
template<typename... Args>
void removed(Args&&...) { }

};

using ScenarioComponent = SimpleHierarchicalScenarioComponent<
ScenarioBase,
Scenario::ProcessModel,
Constraint>;

using ScenarioComponentFactory = ProcessComponentFactory_T<ScenarioComponent>;

///////////////////////////////////
class PdComponent :
    public DataFlowProcessComponent
{
    COMPONENT_METADATA("cbd653fc-2e29-4eb9-8f89-acf0074e8ec0")
    public:
      PdComponent(
        Process::DataflowProcess& process,
        DocumentPlugin& doc,
        const Id<iscore::Component>& id,
        QObject* parent);

  private:

};

using PdComponentFactory = ProcessComponentFactory_T<PdComponent>;
//////////////////////////////////

}
