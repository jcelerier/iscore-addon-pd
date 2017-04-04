#pragma once
#include <Pd/DataflowWindow.hpp>
#include <Pd/DataflowObjects.hpp>
#include <nodes/../../src/Connection.hpp>
#include <iscore/plugins/documentdelegate/plugin/DocumentPlugin.hpp>
#include <iscore/document/DocumentContext.hpp>
#include <iscore/command/Dispatchers/QuietOngoingCommandDispatcher.hpp>
#include <core/document/Document.hpp>
#include <core/document/DocumentModel.hpp>
#include <boost/bimap.hpp>
namespace Dataflow
{

class DocumentPlugin final :
    public iscore::DocumentPlugin
{
  Q_OBJECT

public:
  struct ConnectionImpl {
    QtNodes::Connection* gui{};
    Cable cable;
  };

  explicit DocumentPlugin(
      const iscore::DocumentContext& ctx,
      Id<iscore::DocumentPlugin> id,
      QObject* parent);

  virtual ~DocumentPlugin();

  void reload();

  // Model -> UI
  void createConnection(Cable c);
  void updateConnection(const Cable& before, Cable after);
  void removeConnection(const Cable& c);

  void quiet_createConnection(ConnectionImpl);
  void quiet_updateConnection(const Cable& before, Cable after);
  void quiet_removeConnection(const Cable& c);

  // UI -> Model
  void on_connectionCreated(QtNodes::Connection& c);
  void on_connectionUpdated(QtNodes::Connection& c);
  void on_connectionDeleted(QtNodes::Connection& c);
  void on_nodeMoved(QtNodes::Node& c, const QPointF& pos);
  void on_released(QPointF);

  DataflowWindow window;

  std::vector<ConnectionImpl> cables;

  iscore::QuietOngoingCommandDispatcher m_dispatcher;

  struct temp_bool {
    bool& b;
    temp_bool(bool& bo): b{bo} { b = true; }
    ~temp_bool() { b = false; }
  };
  auto start_command() { return temp_bool{m_applyingCommand}; }
  bool m_applyingCommand{false};
};
}
