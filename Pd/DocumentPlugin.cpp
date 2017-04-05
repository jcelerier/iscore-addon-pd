#include <Pd/DocumentPlugin.hpp>
#include <nodes/../../src/Node.hpp>
#include <nodes/../../src/NodeGraphicsObject.hpp>
#include <iscore/command/Dispatchers/CommandDispatcher.hpp>
#include <iscore/command/Dispatchers/SendStrategy.hpp>
#include <Pd/Commands/EditConnection.hpp>
#include <Explorer/DocumentPlugin/DeviceDocumentPlugin.hpp>
#include <Engine/iscore2OSSIA.hpp>
namespace Dataflow
{
DocumentPlugin::DocumentPlugin(
        const iscore::DocumentContext& ctx,
        Id<iscore::DocumentPlugin> id,
        QObject* parent):
    iscore::DocumentPlugin{ctx, std::move(id), "PdDocPlugin", parent},
    m_dispatcher{ctx.commandStack},
    audiodev{std::make_unique<ossia::net::local_protocol>(), "audio"},
    mididev{std::make_unique<ossia::net::local_protocol>(), "midi"}
{
  con(window.scene, &QtNodes::FlowScene::connectionCreated,
      this, &DocumentPlugin::on_connectionCreated);
  con(window.scene, &QtNodes::FlowScene::connectionDeleted,
      this, &DocumentPlugin::on_connectionDeleted);
  con(window.scene, &QtNodes::FlowScene::nodeMoved,
      this, &DocumentPlugin::on_nodeMoved);
  con(window.scene, &ReleaseFlowScene::released,
      this, &DocumentPlugin::on_released);

  con(window, &DataflowWindow::typeChanged,
      this, &DocumentPlugin::on_connectionTypeChanged);

  audio_ins.push_back(ossia::net::create_node(audiodev.getRootNode(), "/in/0").createAddress());
  audio_ins.push_back(ossia::net::create_node(audiodev.getRootNode(), "/in/1").createAddress());
  audio_outs.push_back(ossia::net::create_node(audiodev.getRootNode(), "/out/0").createAddress());
  audio_outs.push_back(ossia::net::create_node(audiodev.getRootNode(), "/out/1").createAddress());
  midi_ins.push_back(ossia::net::create_node(mididev.getRootNode(), "/in/0").createAddress());
  midi_outs.push_back(ossia::net::create_node(mididev.getRootNode(), "/out/0").createAddress());

  currentExecutionContext = std::make_shared<ossia::graph>();
}

DocumentPlugin::~DocumentPlugin()
{

}

void DocumentPlugin::reload()
{
  window.scene.clearScene();

  auto processes = m_context.document.findChildren<Dataflow::ProcessModel*>();
  for(auto proc : processes)
  {
    auto model = std::make_unique<CustomDataModel>(*proc);
    proc->node = &window.scene.createNode(std::move(model));
    proc->node->nodeGraphicsObject().setPos(proc->pos());

    for(auto& cbl : proc->cables)
    {
      // Try to re-create all the registered cables in the process
      createConnection(cbl);
    }

    connect(proc, &Dataflow::ProcessModel::identified_object_destroying,
            this, [=] {
      if(proc->node)
      {
        auto cmd = start_command(); // To prevent connection deletion signals

        for(const auto& cable : proc->cables)
        {
          quiet_removeConnection(cable);
        }
        window.scene.removeNode(*proc->node);
      }
    });
  }

  for(auto& pair : cables)
  {
    auto& cable = pair.cable;
    auto& src = cable.source.find();
    auto& snk = cable.sink.find();
    if(src.nodeModel && snk.nodeModel && cable.outlet && cable.inlet)
    {
      pair.gui = window.scene.createConnection(
            *snk.node, *cable.inlet,
            *src.node, *cable.outlet).get();
      auto ct = std::make_unique<CustomConnection>(window.scene, *pair.gui);

      con(*ct, &CustomConnection::selectionChanged,
          this, [=] (bool b) {
          if(b) window.cableSelected(*pair.gui);
          else window.cableDeselected(*pair.gui);
      });
      pair.gui->setGraphicsObject(std::move(ct));
    }
    else
    {
      pair.gui = nullptr;
    }
  }
}

void DocumentPlugin::createConnection(Cable c)
{
  auto cmd = start_command();
  ConnectionImpl impl;
  impl.cable = std::move(c);
  auto& cable = impl.cable;
  auto& src = cable.source.find();
  auto& snk = cable.sink.find();

  if(src.nodeModel && snk.nodeModel && cable.outlet && cable.inlet)
  {
    impl.gui = window.scene.createConnection(
          *snk.node, *cable.inlet,
          *src.node, *cable.outlet).get();
    impl.gui->setGraphicsObject(std::make_unique<CustomConnection>(window.scene, *impl.gui));
  }

  quiet_createConnection(std::move(impl));
}

void DocumentPlugin::updateConnection(const Cable& before, Cable after)
{
  auto cmd = start_command();
  auto it = ossia::find_if(cables, [&] (const auto& c) { return c.cable == before; });
  if(it != cables.end())
  {
    auto new_source = after.source.find().node;
    auto new_sink = after.sink.find().node;

    it->gui->setNodeToPort(*new_source, QtNodes::PortType::Out, *after.outlet);
    it->gui->setNodeToPort(*new_sink, QtNodes::PortType::In, *after.inlet);
    it->cable = std::move(after);
  }
  else
  {
    qDebug() << "ERROR! cable not found";
  }
}

void DocumentPlugin::removeConnection(const Cable& c)
{
  auto cmd = start_command();
  auto it = ossia::find_if(cables, [&] (const auto& other) { return other.cable == c; });
  if(it != cables.end())
  {
    if(it->gui)
      window.scene.deleteConnection(*it->gui);
    cables.erase(it);
  }
}

void DocumentPlugin::quiet_createConnection(ConnectionImpl i)
{
  auto it = ossia::find_if(cables, [&] (const auto& other) { return other.cable == i.cable; });
  if(it == cables.end())
    cables.push_back(std::move(i));
}

void DocumentPlugin::quiet_updateConnection(const Cable& before, Cable after)
{
  auto it = ossia::find_if(cables, [&] (const auto& c) { return c.cable == before; });
  if(it != cables.end())
  {
    it->cable = std::move(after);
  }
  else
  {
    qDebug() << "ERROR! cable not found";
  }
}

void DocumentPlugin::quiet_removeConnection(const Cable& c)
{
  auto it = ossia::find_if(cables, [&] (const auto& other) { return other.cable == c; });
  if(it != cables.end())
  {
    cables.erase(it);
  }
}

void DocumentPlugin::on_connectionCreated(QtNodes::Connection& c)
{
  if(m_applyingCommand)
    return;

  // If input & output, create cable right now. Else create it in first update.

  auto in = c.getNode(QtNodes::PortType::In);
  auto out = c.getNode(QtNodes::PortType::Out);
  if(in && out)
  {
    auto in_model = static_cast<CustomDataModel*>(in->nodeDataModel());
    auto out_model = static_cast<CustomDataModel*>(out->nodeDataModel());
    CommandDispatcher<SendStrategy::Quiet> disp{context().commandStack};
    Cable cable{{},
            *out_model->process,
          *in_model->process,
          (std::size_t)c.getPortIndex(QtNodes::PortType::Out),
          (std::size_t)c.getPortIndex(QtNodes::PortType::In)};

    disp.submitCommand<CreateCable>(*this, cable);

    in_model->process->cables.push_back(cable);
    out_model->process->cables.push_back(cable);

    quiet_createConnection({&c, std::move(cable)});
  }

  con(c, &QtNodes::Connection::updated,
      this, &DocumentPlugin::on_connectionUpdated);
}

void DocumentPlugin::on_connectionUpdated(QtNodes::Connection& c)
{
  if(m_applyingCommand)
    return;
  auto in = c.getNode(QtNodes::PortType::In);
  auto out = c.getNode(QtNodes::PortType::Out);
  auto existing_it = ossia::find_if(cables, [&] (const auto& con) { return con.gui == &c; });

  if(existing_it == cables.end())
  {
    if(in && out)
    {
      auto in_model = static_cast<CustomDataModel*>(in->nodeDataModel());
      auto out_model = static_cast<CustomDataModel*>(out->nodeDataModel());
      CommandDispatcher<SendStrategy::Quiet> disp{context().commandStack};
      Cable cable{{},
            *out_model->process,
            *in_model->process,
            (std::size_t)c.getPortIndex(QtNodes::PortType::Out),
            (std::size_t)c.getPortIndex(QtNodes::PortType::In)};

      disp.submitCommand<CreateCable>(*this, cable);

      in_model->process->cables.push_back(cable);
      out_model->process->cables.push_back(cable);

      quiet_createConnection({&c, std::move(cable)});
    }
    else
    {
      // Do nothing ?
      ISCORE_TODO;
    }
  }
  else
  {
    if(in && out)
    {
      auto in_model = static_cast<CustomDataModel*>(in->nodeDataModel());
      auto out_model = static_cast<CustomDataModel*>(out->nodeDataModel());
      CommandDispatcher<SendStrategy::Quiet> disp{context().commandStack};
      Cable previous = existing_it->cable;
      Cable next{previous.type,
                 *out_model->process,
                 *in_model->process,
                 (std::size_t)c.getPortIndex(QtNodes::PortType::Out),
                 (std::size_t)c.getPortIndex(QtNodes::PortType::In)};

      disp.submitCommand<UpdateCable>(*this, previous, next);

      { // Remove cable in previous nodes
        auto& source = previous.source.find();
        source.cables.erase(ossia::find(source.cables, previous));
        auto& sink = previous.sink.find();
        sink.cables.erase(ossia::find(sink.cables, previous));
      }
      { // Add cable in new nodes
        in_model->process->cables.push_back(next);
        out_model->process->cables.push_back(next);
      }

      quiet_updateConnection(std::move(previous), std::move(next));
    }
    else
    {
      ISCORE_TODO;
      // Do nothing ?
      // remove ?
    }

  }
}

void DocumentPlugin::on_connectionDeleted(QtNodes::Connection& c)
{
  if(m_applyingCommand)
    return;
  auto existing_it = ossia::find_if(cables, [&] (const auto& con) { return con.gui == &c; });
  if(existing_it != cables.end())
  {
    CommandDispatcher<SendStrategy::Quiet> disp{context().commandStack};
    disp.submitCommand<RemoveCable>(*this, existing_it->cable);

    auto& source = existing_it->cable.source.find();
    source.cables.erase(ossia::find(source.cables, existing_it->cable));
    auto& sink = existing_it->cable.sink.find();
    sink.cables.erase(ossia::find(sink.cables, existing_it->cable));

    quiet_removeConnection(existing_it->cable);
  }
}

void DocumentPlugin::on_connectionTypeChanged(QList<QtNodes::Connection*> c, CableType t)
{
  ISCORE_TODO;
}

void DocumentPlugin::on_nodeMoved(QtNodes::Node& n, const QPointF& pos)
{
  if(m_applyingCommand)
    return;
  auto model = static_cast<CustomDataModel*>(n.nodeDataModel());

  m_dispatcher.submitCommand<MoveNode>(*model->process, pos);
}

void DocumentPlugin::on_released(QPointF)
{
  m_dispatcher.commit();
}

ossia::net::address_base* DocumentPlugin::resolve(const State::AddressAccessor& addr) const
{
  if(addr.address.device == QStringLiteral("audio"))
  {
    return Engine::iscore_to_ossia::findNodeFromPath(addr.address.path, audiodev)->getAddress();
  }
  else if(addr.address.device == QStringLiteral("midi"))
  {
    return Engine::iscore_to_ossia::findNodeFromPath(addr.address.path, mididev)->getAddress();
  }
  else
  {
    return Engine::iscore_to_ossia::address(addr.address, context().plugin<Explorer::DeviceDocumentPlugin>().list());
  }
}

}


