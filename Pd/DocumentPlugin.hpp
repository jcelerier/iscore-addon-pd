#pragma once
#include <Pd/DataflowWindow.hpp>
#include <Pd/DataflowObjects.hpp>
#include <nodes/../../src/Connection.hpp>
#include <iscore/plugins/documentdelegate/plugin/DocumentPlugin.hpp>
#include <iscore/document/DocumentContext.hpp>
#include <iscore/command/Dispatchers/QuietOngoingCommandDispatcher.hpp>
#include <core/document/Document.hpp>
#include <core/document/DocumentModel.hpp>
#include <ossia/network/generic/generic_device.hpp>
#include <ossia/network/local/local.hpp>
#include <ossia/dataflow/graph.hpp>
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

  std::shared_ptr<ossia::graph> currentExecutionContext;

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
  void on_connectionTypeChanged(QList<QtNodes::Connection*> c, CableType t);
  void on_nodeMoved(QtNodes::Node& c, const QPointF& pos);
  void on_released(QPointF);

  ossia::net::address_base* resolve(const State::AddressAccessor& ) const;

  DataflowWindow window;

  std::vector<ConnectionImpl> cables;

  iscore::QuietOngoingCommandDispatcher m_dispatcher;

  mutable ossia::net::generic_device audiodev;
  mutable ossia::net::generic_device mididev;

  std::vector<ossia::net::address_base*> audio_ins;
  std::vector<ossia::net::address_base*> audio_outs;
  std::vector<ossia::net::address_base*> midi_ins;
  std::vector<ossia::net::address_base*> midi_outs;
  struct temp_bool {
    bool& b;
    temp_bool(bool& bo): b{bo} { b = true; }
    ~temp_bool() { b = false; }
  };
  auto start_command() { return temp_bool{m_applyingCommand}; }
  bool m_applyingCommand{false};
};
}
