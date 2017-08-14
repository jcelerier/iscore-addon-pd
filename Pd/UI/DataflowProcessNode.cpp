#include "DataflowProcessNode.hpp"
#include <Pd/UI/NodeItem.hpp>

namespace Process
{
void Node::cleanup()
{
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

}
