#include <Pd/DocumentPlugin.hpp>
#include <nodes/../../src/Node.hpp>
#include <nodes/../../src/NodeGraphicsObject.hpp>
#include <iscore/command/Dispatchers/CommandDispatcher.hpp>
#include <iscore/command/Dispatchers/SendStrategy.hpp>
#include <Pd/Commands/EditConnection.hpp>
#include <Explorer/DocumentPlugin/DeviceDocumentPlugin.hpp>
#include <Engine/iscore2OSSIA.hpp>
#include <ossia/dataflow/audio_address.hpp>
#include <iscore/tools/IdentifierGeneration.hpp>
namespace Dataflow
{
template<typename Address>
auto create_address(ossia::net::node_base& root, std::string name)
{
  auto& node = ossia::net::create_node(root, name);
  auto addr = new Address(node);
  node.setAddress(std::unique_ptr<Address>(addr));
  return addr;
}

DocumentPlugin::DocumentPlugin(
        const iscore::DocumentContext& ctx,
        Id<iscore::DocumentPlugin> id,
        QObject* parent):
    iscore::DocumentPlugin{ctx, std::move(id), "PdDocPlugin", parent},
    m_dispatcher{ctx.commandStack},
    audiodev{std::make_unique<ossia::net::local_protocol>(), "audio"},
    midi_dev{std::make_unique<ossia::net::local_protocol>(), "midi"}
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

  audio_ins.push_back(create_address<ossia::audio_address>(audiodev.getRootNode(), "/in/0"));
  audio_ins.push_back(create_address<ossia::audio_address>(audiodev.getRootNode(), "/in/1"));
  audio_outs.push_back(create_address<ossia::audio_address>(audiodev.getRootNode(), "/out/0"));
  audio_outs.push_back(create_address<ossia::audio_address>(audiodev.getRootNode(), "/out/1"));

  midi_ins.push_back(create_address<ossia::midi_generic_address>(midi_dev.getRootNode(), "/0/in"));
  midi_outs.push_back(create_address<ossia::midi_generic_address>(midi_dev.getRootNode(), "/0/out"));

  execGraph = std::make_shared<ossia::graph>();
}

DocumentPlugin::~DocumentPlugin()
{

}

void DocumentPlugin::reload()
{
  window.scene.clearScene();

  for(Cable& cable : cables) cable.gui = nullptr;


  auto processes = m_context.document.findChildren<Dataflow::ProcessModel*>();
  for(auto proc : processes)
  {
    auto cmd = start_command(); // To prevent connection deletion signals
    auto model = std::make_unique<CustomDataModel>(*proc);
    proc->node = &window.scene.createNode(std::move(model));
    proc->node->nodeGraphicsObject().setPos(proc->pos());

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
  qDebug() << cables.size();

  for(Cable& cable : cables)
  {
    qDebug() << "1:" << cables.size();
    auto& src = cable.source.find();
    auto& snk = cable.sink.find();
    if(src.nodeModel && snk.nodeModel && cable.outlet && cable.inlet)
    {
      auto cmd = start_command(); // To prevent connection deletion signals
      qDebug() << "2:" << cables.size();
      cable.gui = window.scene.createConnection(
            *snk.node, *cable.inlet,
            *src.node, *cable.outlet).get();
      auto ct = std::make_unique<CustomConnection>(window.scene, *cable.gui);

      con(*ct, &CustomConnection::selectionChanged,
          this, [=,&cable] (bool b) {
          if(b) window.cableSelected(*cable.gui);
          else window.cableDeselected(*cable.gui);
      });

      qDebug() << "3:" << cables.size();
      cable.gui->setGraphicsObject(std::move(ct));

      qDebug() << "4:" << cables.size();
    }
    else
    {
      cable.gui = nullptr;
    }
  }

}

void DocumentPlugin::createGuiConnection(Cable& cable)
{
  auto cmd = start_command();

  auto& src = cable.source.find();
  auto& snk = cable.sink.find();

  if(src.nodeModel && snk.nodeModel && cable.outlet && cable.inlet)
  {
    cable.gui = window.scene.createConnection(
          *snk.node, *cable.inlet,
          *src.node, *cable.outlet).get();
    cable.gui->setGraphicsObject(std::make_unique<CustomConnection>(window.scene, *cable.gui));
  }
}

void DocumentPlugin::updateConnection(const Cable& cable, CableData)
{
  /*
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
  */
}

void DocumentPlugin::removeConnection(Id<Cable> c)
{
  auto cmd = start_command();

  auto it = cables.find(c);
  if(it != cables.end())
  {
    if(it->gui)
      window.scene.deleteConnection(*it->gui);
    cables.remove(c);
  }
}

void DocumentPlugin::quiet_createConnection(Cable* i)
{
  ISCORE_ASSERT(cables.find(i->id()) == cables.end());
  cables.add(i);
}

void DocumentPlugin::quiet_updateConnection(const Cable& before, CableData after)
{
  /*
  auto it = ossia::find_if(cables, [&] (const auto& c) { return c.cable == before; });
  if(it != cables.end())
  {
    it->cable = std::move(after);
  }
  else
  {
    qDebug() << "ERROR! cable not found";
  }
  */
}

void DocumentPlugin::quiet_removeConnection(const Cable& c)
{
  /*
  auto it = ossia::find_if(cables, [&] (const auto& other) { return other.cable == c; });
  if(it != cables.end())
  {
    cables.erase(it);
  }
  */
}

void DocumentPlugin::createCableFromGuiImpl(QtNodes::Connection& c, QtNodes::Node* in, QtNodes::Node* out)
{
  auto in_model = static_cast<CustomDataModel*>(in->nodeDataModel());
  auto out_model = static_cast<CustomDataModel*>(out->nodeDataModel());
  CommandDispatcher<SendStrategy::Quiet> disp{context().commandStack};
  auto cable = new Cable{getStrongId(cables),
      CableData{{},
      *out_model->process,
      *in_model->process,
      (std::size_t)c.getPortIndex(QtNodes::PortType::Out),
      (std::size_t)c.getPortIndex(QtNodes::PortType::In)}};
  cable->gui = &c;

  // The command is sent but not executed since the cable has already been created
  // by the framework
  disp.submitCommand<CreateCable>(*this, cable->id(), (CableData) *cable);

  in_model->process->cables.push_back(cable->id());
  out_model->process->cables.push_back(cable->id());

  quiet_createConnection(cable);
}

void DocumentPlugin::updateCableFromGuiImpl(QtNodes::Connection& c, QtNodes::Node* in, QtNodes::Node* out, Cable& cur)
{
  auto in_model = static_cast<CustomDataModel*>(in->nodeDataModel());
  auto out_model = static_cast<CustomDataModel*>(out->nodeDataModel());
  CommandDispatcher<SendStrategy::Quiet> disp{context().commandStack};

  CableData next{cur.type,
             *out_model->process,
             *in_model->process,
             (std::size_t)c.getPortIndex(QtNodes::PortType::Out),
             (std::size_t)c.getPortIndex(QtNodes::PortType::In)};

  { // Remove cable in previous nodes
    auto& source = cur.source.find();
    source.cables.erase(ossia::find(source.cables, cur.id()));
    auto& sink = cur.sink.find();
    sink.cables.erase(ossia::find(sink.cables, cur.id()));
  }

  { // Add cable in new nodes
    in_model->process->cables.push_back(cur.id());
    out_model->process->cables.push_back(cur.id());
  }

  (CableData&)cur = next;
  disp.submitCommand<UpdateCable>(*this, cur, std::move(next));

  //quiet_updateConnection(std::move(previous), std::move(next));
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
    createCableFromGuiImpl(c, in, out);
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
      createCableFromGuiImpl(c, in, out);
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
      updateCableFromGuiImpl(c, in, out, *existing_it);
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
  /*
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
  */
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
    return Engine::iscore_to_ossia::findNodeFromPath(addr.address.path, midi_dev)->getAddress();
  }
  else
  {
    return Engine::iscore_to_ossia::address(addr.address, context().plugin<Explorer::DeviceDocumentPlugin>().list());
  }
}

}


