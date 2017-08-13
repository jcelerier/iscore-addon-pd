#pragma once
#include <Pd/DocumentPlugin.hpp>
#include <Pd/PdProcess.hpp>
#include <Pd/UI/ConstraintNode.hpp>
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
#include <Pd/UI/Slider.hpp>
namespace Dataflow
{

class ScenarioNode : public Process::Node
{
public:
    ScenarioNode(const DocumentPlugin& ctx, Id<Process::Node> c, QObject *parent);

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


// For each constraint list the max num. of channels
// and create volume sliders and mix them
// For each constraint mix all the messages
// For each constraint mix all the midi
// For each state mix it.
// -> we have to add a link to the ossia::scenario
// -> we have to add data in events to indicate what states are to be executed
class ScenarioBase :
    public ProcessComponent_T<Scenario::ProcessModel>
{
    COMPONENT_METADATA("3ae53c95-fa10-47fa-a04f-eb766e3095f7")

public:
    ScenarioBase(
            Scenario::ProcessModel& scenario,
            DocumentPlugin& doc,
            const Id<iscore::Component>& id,
            QObject* parent_obj);

    struct ConstraintData
    {
        Constraint* component{};
        Slider* mix{};
    };

    Constraint* make(
            const Id<iscore::Component>& id,
            Scenario::ConstraintModel& elt);

    void setupConstraint(Scenario::ConstraintModel& c, Constraint* comp);
    void teardownConstraint(const Scenario::ConstraintModel& c, const Constraint& comp);

    template<typename... Args>
    bool removing(Args&&... args) { teardownConstraint(std::forward<Args>(args)...); return true; }
    template<typename... Args>
    void removed(Args&&...) { }

    ScenarioNode& mainNode() override { return m_node; }

private:
    std::unordered_map<const Scenario::ConstraintModel*, ConstraintData> m_constraints;
    QVector<Process::Node*> m_sliders;
    ScenarioNode m_node;
};

using ScenarioComponent = SimpleHierarchicalScenarioComponent<
ScenarioBase,
Scenario::ProcessModel,
Constraint>;

using ScenarioComponentFactory = ProcessComponentFactory_T<ScenarioComponent>;
}
