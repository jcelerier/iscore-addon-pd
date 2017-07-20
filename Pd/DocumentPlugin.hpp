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

#include <Pd/Devices/AudioDevice.hpp>
namespace ossia { class audio_address; class midi_generic_address; }
namespace Dataflow
{

class DocumentPlugin final :
    public iscore::DocumentPlugin
{
  Q_OBJECT

  void createCableFromGuiImpl(QtNodes::Connection& c, QtNodes::Node* in, QtNodes::Node* out);
  void updateCableFromGuiImpl(QtNodes::Connection& c, QtNodes::Node* in, QtNodes::Node* out, Process::Cable&);
public:
  std::shared_ptr<ossia::graph> execGraph;
  ossia::execution_state execState;

  explicit DocumentPlugin(
      const iscore::DocumentContext& ctx,
      Id<iscore::DocumentPlugin> id,
      QObject* parent);

  virtual ~DocumentPlugin();

  void reload();

  // Model -> UI
  void createGuiConnection(Process::Cable& c);
  void updateConnection(const Process::Cable& cable, Process::CableData);
  void removeConnection(Id<Process::Cable> c);

  void quiet_createConnection(Process::Cable*);
  void quiet_updateConnection(const Process::Cable& cable, Process::CableData);
  void quiet_removeConnection(const Process::Cable& c);

  // UI -> Model
  void on_connectionCreated(QtNodes::Connection& c);
  void on_connectionUpdated(QtNodes::Connection& c);
  void on_connectionDeleted(QtNodes::Connection& c);
  void on_connectionTypeChanged(QList<QQuickItem*> c, Process::CableType t);
  void on_nodeMoved(QtNodes::Node& c, const QPointF& pos);
  void on_released(QPointF);

  Pd::audio_protocol& audioProto() { return *audioproto; }
  ossia::net::address_base* resolve(const State::AddressAccessor& ) const;

  DataflowWindow window;

  iscore::EntityMap<Process::Cable> cables;

  iscore::QuietOngoingCommandDispatcher m_dispatcher;

  Pd::audio_protocol* audioproto{};
  mutable ossia::net::generic_device audio_dev;
  mutable ossia::net::generic_device midi_dev;

  std::vector<ossia::midi_generic_address*> midi_ins;
  std::vector<ossia::midi_generic_address*> midi_outs;
  struct temp_bool {
    bool& b;
    temp_bool(bool& bo): b{bo} { b = true; }
    ~temp_bool() { b = false; }
  };
  auto start_command() { return temp_bool{m_applyingCommand}; }
  bool m_applyingCommand{false};
};
}
