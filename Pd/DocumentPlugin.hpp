#pragma once
#include <iscore/plugins/documentdelegate/plugin/DocumentPlugin.hpp>
#include <Pd/DataflowWindow.hpp>
#include <Process/Process.hpp>
#include <ossia/detail/optional.hpp>
namespace Dataflow
{
struct Cable
{
  Path<Process::ProcessModel> source, sink;
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

  DataflowWindow window;

  std::vector<Cable> cables;
};
}
