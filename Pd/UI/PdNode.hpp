#pragma once
#include <Pd/UI/DataflowProcessNode.hpp>

namespace Dataflow
{
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
};

using PdComponentFactory = ProcessComponentFactory_T<PdComponent>;
}
