#include <Pd/DocumentPlugin.hpp>
#include <iscore/command/Dispatchers/CommandDispatcher.hpp>
#include <iscore/command/Dispatchers/SendStrategy.hpp>
#include <Pd/Commands/EditConnection.hpp>
#include <Explorer/DocumentPlugin/DeviceDocumentPlugin.hpp>
#include <Engine/iscore2OSSIA.hpp>
#include <ossia/dataflow/audio_address.hpp>
#include <iscore/tools/IdentifierGeneration.hpp>
#include <Pd/UI/ScenarioNode.hpp>
#include <Pd/UI/NodeItem.hpp>

#include <Scenario/Document/ScenarioDocument/ScenarioDocumentModel.hpp>
#include <iscore/document/DocumentInterface.hpp>
namespace Dataflow
{

IdContainer<Dataflow::CableItem, Process::Cable> cableItems;
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
  cables.mutable_added.connect<DocumentPlugin, &DocumentPlugin::on_cableAdded>(*this);
  cables.removing.connect<DocumentPlugin, &DocumentPlugin::on_cableRemoving>(*this);
}

void DocumentPlugin::init()
{
  auto& md = iscore::IDocument::modelDelegate<Scenario::ScenarioDocumentModel>(context().document);
  m_constraint = new Dataflow::Constraint(Id<iscore::Component>{}, md.baseConstraint(), *this, this);

  midi_ins.push_back(create_address<ossia::midi_generic_address>(midi_dev.get_root_node(), "/0/in"));
  midi_outs.push_back(create_address<ossia::midi_generic_address>(midi_dev.get_root_node(), "/0/out"));

  execGraph = std::make_shared<ossia::graph>();
  audioproto->reload();
}

DocumentPlugin::~DocumentPlugin()
{
  delete m_constraint;
  audioproto->stop();
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

void DocumentPlugin::on_cableAdded(Process::Cable& c)
{
  auto source = dynamic_cast<NodeItem*>(c.source()->ui);
  auto sink = dynamic_cast<NodeItem*>(c.sink()->ui);
  if(!source || !sink)
  {
    qDebug("Unexpected source / sink missing");
    return;
  }

  auto ci = new Dataflow::CableItem{c, source, sink};
  ci->setParentItem(window.view.contentItem());
  cableItems.insert(ci);
}

void DocumentPlugin::on_cableRemoving(const Process::Cable& c)
{
  auto& map = cableItems.get();
  auto it = map.find(c.id());
  if (it != map.end())
  {
    delete *it;
    map.erase(it);
  }
}

}


