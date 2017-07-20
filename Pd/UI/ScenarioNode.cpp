#include "ScenarioNode.hpp"
#include "NodeItem.hpp"
#include "View.hpp"

namespace Dataflow
{

ProcessComponent::ProcessComponent(
    Process::ProcessModel& process,
    DocumentPlugin& doc,
    const Id<iscore::Component>& id,
    const QString& name,
    QObject* parent):
  Scenario::GenericProcessComponent<DocumentPlugin>{process, doc, id, name, parent}

{

}

ProcessComponent::~ProcessComponent()
{
  delete ui;
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


  ui = new NodeItem{*this};
  ui->setParentItem(doc.window.view.contentItem());
  ui->setPosition(QPointF(50, 50));
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
  return factory.make(process, system(), id, this);
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
