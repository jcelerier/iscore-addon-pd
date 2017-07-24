#include "ScenarioNode.hpp"
#include "NodeItem.hpp"
#include "View.hpp"

namespace Process
{
void Node::cleanup()
{
  qDebug("deleting");
  auto& p = iscore::IDocument::documentContext(*this).plugin<Dataflow::DocumentPlugin>();
  for(const auto& cable : cables())
  {
    // TODO undo-redo fails with this
    p.cables.remove(cable);
  }
}
}
namespace Dataflow
{

DataflowProcessNode::DataflowProcessNode(
    DocumentPlugin& doc,
    Process::DataflowProcess& proc,
    Id<Process::Node> c,
    QObject* parent)
  : Process::Node{c, parent}
  , process{proc}
{
  connect(&process, &Process::DataflowProcess::inletsChanged,
          this, &DataflowProcessNode::inletsChanged);
  connect(&process, &Process::DataflowProcess::outletsChanged,
          this, &DataflowProcessNode::outletsChanged);

  auto itm = new NodeItem{doc.context(), *this};
  itm->setParentItem(doc.window.view.contentItem());
  itm->setPosition(QPointF(50, 50));
  ui = itm;
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
    QObject* parent)
: ProcessComponent_T<Process::DataflowProcess>{process, doc, id, name, parent}
, m_node{doc, process, Id<Process::Node>{}, this}
{
}

DataFlowProcessComponent::~DataFlowProcessComponent()
{
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
