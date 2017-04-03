#pragma once
#include <iscore/plugins/documentdelegate/plugin/DocumentPlugin.hpp>
#include <Pd/DataflowWindow.hpp>
#include <Pd/DataflowProcess.hpp>
#include <iscore/document/DocumentContext.hpp>
#include <core/document/Document.hpp>
#include <core/document/DocumentModel.hpp>
#include <ossia/detail/optional.hpp>
namespace Dataflow
{
struct Cable
{
  Path<Dataflow::ProcessModel> source, sink;
  ossia::optional<int> outlet, inlet;
};

class DocumentPlugin final :
    public iscore::DocumentPlugin
{
  Q_OBJECT

public:
  explicit DocumentPlugin(
      const iscore::DocumentContext& ctx,
      Id<iscore::DocumentPlugin> id,
      QObject* parent);

  virtual ~DocumentPlugin();


  void reload()
  {
    window.scene.clearScene();
    auto processes = m_context.document.findChildren<Dataflow::ProcessModel*>();
    for(auto proc : processes)
    {
      auto model = std::make_unique<CustomDataModel>(*proc);
      proc->node = &window.scene.createNode(std::move(model));

    }

    for(auto& cable : cables)
    {
      auto src = cable.source.try_find();
      auto snk = cable.sink.try_find();
      if(src && snk && src->nodeModel && snk->nodeModel && cable.outlet && cable.inlet)
      {
        window.scene.createConnection(
              *snk->node, *cable.inlet,
              *src->node, *cable.outlet);
      }
    }

  }
  DataflowWindow window;

  std::vector<Cable> cables;
};
}
