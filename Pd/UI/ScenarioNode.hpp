#pragma once
#include <Pd/DocumentPlugin.hpp>
#include <Scenario/Document/Components/ConstraintComponent.hpp>
#include <Scenario/Document/Components/ProcessComponent.hpp>
#include <Scenario/Document/Components/ScenarioComponent.hpp>
#include <Process/Process.hpp>
#include <iscore/model/Component.hpp>
#include <iscore/model/ComponentHierarchy.hpp>
#include <iscore/plugins/customfactory/ModelFactory.hpp>
#include <iscore/model/ComponentFactory.hpp>
#include <iscore_addon_pd_export.h>
#include <Process/Dataflow/DataflowObjects.hpp>
namespace Dataflow
{

class ISCORE_ADDON_PD_EXPORT ProcessComponent :
    public Scenario::GenericProcessComponent<DocumentPlugin>
{
    Q_OBJECT
       ABSTRACT_COMPONENT_METADATA(Dataflow::ProcessComponent, "44f68a30-4bb1-4d94-940f-074f5b5b78fe")
  public:
    ProcessComponent(
        Process::ProcessModel& process,
        DocumentPlugin& doc,
        const Id<iscore::Component>& id,
        const QString& name,
        QObject* parent):
      Scenario::GenericProcessComponent<DocumentPlugin>{process, doc, id, name, parent}

    {

    }

    virtual std::size_t audioInlets() const = 0;
    virtual std::size_t messageInlets() const = 0;
    virtual std::size_t midiInlets() const = 0;

    virtual std::size_t audioOutlets() const = 0;
    virtual std::size_t messageOutlets() const = 0;
    virtual std::size_t midiOutlets() const = 0;

    virtual std::vector<Process::Port> inlets() const = 0;
    virtual std::vector<Process::Port> outlets() const = 0;

    virtual std::vector<Id<Process::Cable>> cables() const = 0;
    virtual void addCable(Id<Process::Cable> c) = 0;
    virtual ~ProcessComponent()
    {

    }

  signals:
    void inletsChanged();
    void outletsChanged();
};

template<typename Process_T>
using ProcessComponent_T = Scenario::GenericProcessComponent_T<ProcessComponent, Process_T>;

class ISCORE_ADDON_PD_EXPORT DataFlowProcessComponent : public ProcessComponent_T<Process::DataflowProcess>
{
  public:
    DataFlowProcessComponent(
        Process::DataflowProcess& process,
        DocumentPlugin& doc,
        const Id<iscore::Component>& id,
        const QString& name,
        QObject* parent):
      ProcessComponent_T<Process::DataflowProcess>{process, doc, id, name, parent}
    {
      connect(&process, &Process::DataflowProcess::inletsChanged, this, &ProcessComponent::inletsChanged);
      connect(&process, &Process::DataflowProcess::outletsChanged, this, &ProcessComponent::outletsChanged);

    }

    std::size_t audioInlets() const { return process().audioInlets(); }
    std::size_t messageInlets() const { return process().messageInlets(); }
    std::size_t midiInlets() const { return process().midiInlets(); }

    std::size_t audioOutlets() const override { return process().audioOutlets(); }
    std::size_t messageOutlets() const override{ return process().messageOutlets(); }
    std::size_t midiOutlets() const override{ return process().midiOutlets(); }

    std::vector<Process::Port> inlets() const override { return process().inlets(); }
    std::vector<Process::Port> outlets() const override { return process().outlets(); }

    std::vector<Id<Process::Cable>> cables() const override { return process().cables; }
    void addCable(Id<Process::Cable> c) override { process().cables.push_back(c); }
};

class ISCORE_ADDON_PD_EXPORT ProcessComponentFactory :
    public iscore::GenericComponentFactory<
    Process::ProcessModel,
    DocumentPlugin,
    ProcessComponentFactory>
{
    ISCORE_ABSTRACT_COMPONENT_FACTORY(Dataflow::ProcessComponent)
    public:
      virtual ~ProcessComponentFactory()
    {

    }

    virtual ProcessComponent* make(
        Process::ProcessModel& proc,
        DocumentPlugin& doc,
        const Id<iscore::Component>&,
        QObject* paren_objt) const = 0;
};

template<
    typename ProcessComponent_T>
class ProcessComponentFactory_T :
    public iscore::GenericComponentFactoryImpl<ProcessComponent_T, ProcessComponentFactory>
{
  public:
    using model_type = typename ProcessComponent_T::model_type;
    ProcessComponent* make(
        Process::ProcessModel& proc,
        DocumentPlugin& doc,
        const Id<iscore::Component>& id,
        QObject* paren_objt) const final override
    {
      return new ProcessComponent_T{static_cast<model_type&>(proc), doc, id, paren_objt};
    }
};

using ProcessComponentFactoryList =
iscore::GenericComponentFactoryList<
Process::ProcessModel,
DocumentPlugin,
ProcessComponentFactory>;

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
        QObject* parent_comp):
      parent_t{constraint, doc, id, "ConstraintComponent", parent_comp}
    {
      qDebug("party hard");

    }

    ProcessComponent* make(
        const Id<iscore::Component> & id,
        ProcessComponentFactory& factory,
        Process::ProcessModel &process)
    {
      return factory.make(process, system(), id, this);
    }

    bool removing(const Process::ProcessModel& cst, const ProcessComponent& comp)
    {

      return true;
    }

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


    std::size_t audioInlets() const { return 0; }
    std::size_t messageInlets() const { return 0; }
    std::size_t midiInlets() const { return 0; }

    std::size_t audioOutlets() const override { return 0; }
    std::size_t messageOutlets() const override{ return 0; }
    std::size_t midiOutlets() const override{ return 0; }

    std::vector<Process::Port> inlets() const override { return {}; }
    std::vector<Process::Port> outlets() const override { return {}; }

    std::vector<Id<Process::Cable>> cables() const override { return m_cables; }
    void addCable(Id<Process::Cable> c) override { m_cables.push_back(c); }
    private:
    std::vector<Id<Process::Cable>> m_cables;
};

using ScenarioComponent = SimpleHierarchicalScenarioComponent<
ScenarioBase,
Scenario::ProcessModel,
Constraint>;


using ScenarioComponentFactory = ProcessComponentFactory_T<ScenarioComponent>;
}
