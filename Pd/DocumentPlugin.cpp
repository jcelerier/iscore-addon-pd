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
#include <Pd/UI/ScenarioNode.hpp>
#include <Scenario/Document/ScenarioDocument/ScenarioDocumentModel.hpp>
#include <iscore/document/DocumentInterface.hpp>
namespace Dataflow
{

DocumentPlugin::DocumentPlugin(
        const iscore::DocumentContext& ctx,
        Id<iscore::DocumentPlugin> id,
        QObject* parent):
    iscore::DocumentPlugin{ctx, std::move(id), "PdDocPlugin", parent},
    m_dispatcher{ctx.commandStack},
    audioproto{new Pd::audio_protocol},
    audio_dev{std::unique_ptr<ossia::net::protocol_base>(audioproto), "audio"},
    midi_dev{std::make_unique<ossia::net::multiplex_protocol>(), "midi"}
{
  auto& md = iscore::IDocument::modelDelegate<Scenario::ScenarioDocumentModel>(ctx.document);
  auto& scenar = dynamic_cast<Scenario::ProcessModel&>(*md.baseConstraint().processes.begin());
  auto plug = new Dataflow::ScenarioComponent(scenar, *this, Id<iscore::Component>{}, this);
  /*
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
      */
  midi_ins.push_back(create_address<ossia::midi_generic_address>(midi_dev.get_root_node(), "/0/in"));
  midi_outs.push_back(create_address<ossia::midi_generic_address>(midi_dev.get_root_node(), "/0/out"));

  execGraph = std::make_shared<ossia::graph>();
  audioproto->reload();
}

DocumentPlugin::~DocumentPlugin()
{
  audioproto->stop();

}

void DocumentPlugin::reload()
{
  /*
  window.scene.clearScene();

  for(Process::Cable& cable : cables) cable.gui = nullptr;


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

  for(Process::Cable& cable : cables)
  {
    auto src_p = dynamic_cast<Dataflow::ProcessModel*>(cable.m_source);
    auto snk_p = dynamic_cast<Dataflow::ProcessModel*>(cable.m_sink);

    if(src_p && snk_p && src_p->nodeModel && snk_p->nodeModel && cable.m_outlet && cable.m_inlet)
    {
      auto cmd = start_command(); // To prevent connection deletion signals
      cable.gui = window.scene.createConnection(
            *snk_p->node, *cable.m_inlet,
            *src_p->node, *cable.m_outlet).get();
      auto ct = std::make_unique<CustomConnection>(window.scene, *cable.gui);

      con(*ct, &CustomConnection::selectionChanged,
          this, [=,&cable] (bool b) {
          if(b) window.cableSelected(*cable.gui);
          else window.cableDeselected(*cable.gui);
      });

      cable.gui->setGraphicsObject(std::move(ct));
    }
    else
    {
      cable.gui = nullptr;
    }
  }
*/
}

void DocumentPlugin::createGuiConnection(Process::Cable& cable)
{
  /*
  auto cmd = start_command();

  auto src_p = dynamic_cast<Dataflow::ProcessModel*>(cable.m_source);
  auto snk_p = dynamic_cast<Dataflow::ProcessModel*>(cable.m_sink);

  if(src_p && snk_p && src_p->nodeModel && snk_p->nodeModel && cable.m_outlet && cable.m_inlet)
  {
    cable.gui = window.scene.createConnection(
          *snk_p->node, *cable.m_inlet,
          *src_p->node, *cable.m_outlet).get();
    cable.gui->setGraphicsObject(std::make_unique<CustomConnection>(window.scene, *cable.gui));
  }
  */
}

void DocumentPlugin::updateConnection(const Process::Cable& cable, Process::CableData)
{
  /*
  auto cmd = start_command();
  auto it = ossia::find_if(cables, [&] (const auto& c) { return c.cable == before; });
  if(it != cables.end())
  {
    auto new_source = after.source.find(ctx).node;
    auto new_sink = after.sink.find(ctx).node;

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

void DocumentPlugin::removeConnection(Id<Process::Cable> c)
{
  /*
  auto cmd = start_command();

  auto it = cables.find(c);
  if(it != cables.end())
  {
    if(it->gui)
      window.scene.deleteConnection(*it->gui);
    cables.remove(c);
  }
  */
}

void DocumentPlugin::quiet_createConnection(Process::Cable* i)
{
  ISCORE_ASSERT(cables.find(i->id()) == cables.end());
  cables.add(i);
}

void DocumentPlugin::quiet_updateConnection(const Process::Cable& before, Process::CableData after)
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

void DocumentPlugin::quiet_removeConnection(const Process::Cable& c)
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
  /*
  auto in_model = static_cast<CustomDataModel*>(in->nodeDataModel());
  auto out_model = static_cast<CustomDataModel*>(out->nodeDataModel());
  CommandDispatcher<SendStrategy::Quiet> disp{context().commandStack};
  auto cable = new Process::Cable{
               this->context(),
               getStrongId(cables),
               Process::CableData{
                 {},
                 *out_model->process,
                 *in_model->process,
                 (std::size_t)c.getPortIndex(QtNodes::PortType::Out),
                 (std::size_t)c.getPortIndex(QtNodes::PortType::In)
               }
  };
  cable->gui = &c;

  // The command is sent but not executed since the cable has already been created
  // by the framework
  disp.submitCommand<CreateCable>(*this, cable->id(), *cable);

  in_model->process->cables.push_back(cable->id());
  out_model->process->cables.push_back(cable->id());

  quiet_createConnection(cable);
  */
}

void DocumentPlugin::updateCableFromGuiImpl(QtNodes::Connection& c, QtNodes::Node* in, QtNodes::Node* out, Process::Cable& cur)
{
  /*
  auto in_model = static_cast<CustomDataModel*>(in->nodeDataModel());
  auto out_model = static_cast<CustomDataModel*>(out->nodeDataModel());
  CommandDispatcher<SendStrategy::Quiet> disp{context().commandStack};

  Process::CableData next{cur.m_type,
             *out_model->process,
             *in_model->process,
             (std::size_t)c.getPortIndex(QtNodes::PortType::Out),
             (std::size_t)c.getPortIndex(QtNodes::PortType::In)};

  { // Remove cable in previous nodes
    auto& source = *cur.m_source;
    source.cables.erase(ossia::find(source.cables, cur.id()));
    auto& sink = *cur.m_sink;
    sink.cables.erase(ossia::find(sink.cables, cur.id()));
  }

  { // Add cable in new nodes
    in_model->process->cables.push_back(cur.id());
    out_model->process->cables.push_back(cur.id());
  }

  (Process::CableData&)cur = next;
  disp.submitCommand<UpdateCable>(*this, cur, std::move(next));

  //quiet_updateConnection(std::move(previous), std::move(next));
  */
}

void DocumentPlugin::on_connectionCreated(QtNodes::Connection& c)
{
  /*
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
      */
}

void DocumentPlugin::on_connectionUpdated(QtNodes::Connection& c)
{
  /*
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
  */
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

    auto& source = existing_it->cable.source.find(ctx);
    source.cables.erase(ossia::find(source.cables, existing_it->cable));
    auto& sink = existing_it->cable.sink.find(ctx);
    sink.cables.erase(ossia::find(sink.cables, existing_it->cable));

    quiet_removeConnection(existing_it->cable);
  }
  */
}

void DocumentPlugin::on_connectionTypeChanged(QList<QQuickItem*> c, Process::CableType t)
{
  ISCORE_TODO;
}

void DocumentPlugin::on_nodeMoved(QtNodes::Node& n, const QPointF& pos)
{
  /*
  if(m_applyingCommand)
    return;
  auto model = static_cast<CustomDataModel*>(n.nodeDataModel());

  m_dispatcher.submitCommand<MoveNode>(*model->process, pos);
  */
}

void DocumentPlugin::on_released(QPointF)
{
  m_dispatcher.commit();
}

ossia::net::address_base* DocumentPlugin::resolve(const State::AddressAccessor& addr) const
{
  if(addr.address.device == QStringLiteral("audio"))
  {
    return Engine::iscore_to_ossia::findNodeFromPath(addr.address.path, audio_dev)->get_address();
  }
  else if(addr.address.device == QStringLiteral("midi"))
  {
    return Engine::iscore_to_ossia::findNodeFromPath(addr.address.path, midi_dev)->get_address();
  }
  else
  {
    return Engine::iscore_to_ossia::address(addr.address, context().plugin<Explorer::DeviceDocumentPlugin>().list());
  }
}

}


