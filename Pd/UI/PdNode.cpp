#include "PdNode.hpp"

namespace Dataflow
{
PdComponent::PdComponent(
    Process::DataflowProcess& process,
    DocumentPlugin& doc,
    const Id<iscore::Component>& id,
    QObject* parent):
  DataFlowProcessComponent{process, doc, id, "Pd", parent}
{
}
}
