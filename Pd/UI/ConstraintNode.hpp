#pragma once
#include <Process/Dataflow/DataflowObjects.hpp>
#include <Scenario/Document/Components/ConstraintComponent.hpp>
#include <iscore_addon_pd_export.h>
namespace Dataflow
{
class Slider;
class ConstraintNode : public Process::Node
{
public:
    ConstraintNode(DocumentPlugin& ctx, Id<Process::Node> c, QObject *parent);

    QString getText() const override;
    std::size_t audioInlets() const override;
    std::size_t messageInlets() const override;
    std::size_t midiInlets() const override;
    std::size_t audioOutlets() const override;
    std::size_t messageOutlets() const override;
    std::size_t midiOutlets() const override;
    std::vector<Process::Port> inlets() const override;
    std::vector<Process::Port> outlets() const override;

    std::vector<Id<Process::Cable> > cables() const override;
    void addCable(Id<Process::Cable> c) override;
    void removeCable(Id<Process::Cable> c) override;

    std::vector<Id<Process::Cable>> m_cables;
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

    struct ProcessData
    {
        Dataflow::ProcessComponent* component{};
        Slider* mix{};
    };
    ProcessComponent* make(
            const Id<iscore::Component> & id,
            ProcessComponentFactory& factory,
            Process::ProcessModel &process);

    bool removing(const Process::ProcessModel& cst, const ProcessComponent& comp);

    template <typename... Args>
    void removed(Args&&...)
    {
    }

    ConstraintNode& mainNode() { return m_node; }

private:
    void setupProcess(Process::ProcessModel &c, ProcessComponent *comp);
    void teardownProcess(const Process::ProcessModel &c, const ProcessComponent&);

    std::unordered_map<const Process::ProcessModel*, ProcessData> m_processes;
    QVector<Process::Node*> m_sliders;
    ConstraintNode m_node;
};

class Constraint final :
        public iscore::PolymorphicComponentHierarchy<ConstraintBase>
{
public:
    using iscore::PolymorphicComponentHierarchy<ConstraintBase>::PolymorphicComponentHierarchyManager;
};

}
