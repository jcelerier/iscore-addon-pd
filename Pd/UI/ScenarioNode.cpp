#include "ScenarioNode.hpp"
#include "NodeItem.hpp"
#include "View.hpp"

namespace Dataflow
{

void ProcessComponent::cleanup()
{
  qDebug("deleting");
  DocumentPlugin& p = this->system();
  for(const auto& cable : cables())
  {
    p.cables.remove(cable);
  }
}
ProcessComponent::ProcessComponent(
    Process::ProcessModel& process,
    DocumentPlugin& doc,
    const Id<iscore::Component>& id,
    const QString& name,
    QObject* parent):
  Process::GenericProcessComponent<DocumentPlugin>{process, doc, id, name, parent}

{

}

DataFlowProcessComponent::DataFlowProcessComponent(
    Process::DataflowProcess& process,
    DocumentPlugin& doc,
    const Id<iscore::Component>& id,
    const QString& name,
    QObject* parent):
  ProcessComponent_T<Process::DataflowProcess>{process, doc, id, name, parent}
{
  connect(&process, &Process::DataflowProcess::inletsChanged, this, &ProcessComponent::inletsChanged);
  connect(&process, &Process::DataflowProcess::outletsChanged, this, &ProcessComponent::outletsChanged);


  auto itm = new NodeItem{*this};
  itm->setParentItem(doc.window.view.contentItem());
  itm->setPosition(QPointF(50, 50));
  ui = itm;
}

DataFlowProcessComponent::~DataFlowProcessComponent()
{
  cleanup();
}

ConstraintBase::ConstraintBase(
    const Id<iscore::Component>& id,
    Scenario::ConstraintModel& constraint,
    ConstraintBase::DocumentPlugin& doc,
    QObject* parent_comp):
  parent_t{constraint, doc, id, "ConstraintComponent", parent_comp}
{
}

ProcessComponent*ConstraintBase::make(
    const Id<iscore::Component>& id,
    ProcessComponentFactory& factory,
    Process::ProcessModel& process)
{
  return factory.make(process, system(), id, &process);
}

bool ConstraintBase::removing(
    const Process::ProcessModel& cst,
    const ProcessComponent& comp)
{

  return true;
}

PdComponent::PdComponent(
    Process::DataflowProcess& process,
    DocumentPlugin& doc,
    const Id<iscore::Component>& id,
    QObject* parent):
  DataFlowProcessComponent{process, doc, id, "Pd", parent}
{
}
}
