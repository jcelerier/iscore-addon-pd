#include <Pd/DocumentPlugin.hpp>

namespace Dataflow
{

DocumentPlugin::DocumentPlugin(
        const iscore::DocumentContext& ctx,
        Id<iscore::DocumentPlugin> id,
        QObject* parent):
    iscore::DocumentPlugin{ctx, std::move(id), "PdDocPlugin", parent}
{
}

DocumentPlugin::~DocumentPlugin()
{

}

}
