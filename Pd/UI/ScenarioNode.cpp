#include "ScenarioNode.hpp"
#include "NodeItem.hpp"
#include "View.hpp"
#include <Pd/UI/ConstraintNode.hpp>
namespace Dataflow
{
ScenarioBase::ScenarioBase(
        Scenario::ProcessModel &scenario,
        DocumentPlugin &doc,
        const Id<iscore::Component> &id,
        QObject *parent_obj):
    ProcessComponent_T<Scenario::ProcessModel>{scenario, doc, id, "ScenarioComponent", parent_obj}
  , m_node{doc, Id<Process::Node>{}, this}
{
}

Constraint *ScenarioBase::make(
        const Id<iscore::Component> &id,
        Scenario::ConstraintModel &elt)
{
    auto comp = new Constraint{id, elt, system(), this};
    setupConstraint(elt, comp);
    return comp;
}

void ScenarioBase::setupConstraint(
        Scenario::ConstraintModel &c,
        Constraint *comp)
{
    auto slider = new Slider{system(), getStrongId(m_sliders), this};

    {
      // Create a cable from the slider to the scenario
      auto cable = new Process::Cable{getStrongId(system().cables)};
      cable->setSource(slider);
      cable->setSink(&m_node);
      cable->setOutlet(0);
      cable->setInlet(0);
      system().cables.add(cable);
    }

    m_constraints.insert({&c, {comp, slider}});
    m_sliders.push_back(slider);

    // Connect constraint to slider and slider to this
    {
      auto cable = new Process::Cable{getStrongId(system().cables)};
      cable->setSource(&comp->mainNode());
      cable->setSink(slider);
      cable->setOutlet(0);
      cable->setInlet(0);
      system().cables.add(cable);
    }
    { // Messages
      auto cable = new Process::Cable{getStrongId(system().cables)};
      cable->setSource(&comp->mainNode());
      cable->setSink(&m_node);
      cable->setOutlet(1);
      cable->setInlet(1);
      system().cables.add(cable);
    }
    { // MIDI
      auto cable = new Process::Cable{getStrongId(system().cables)};
      cable->setSource(&comp->mainNode());
      cable->setSink(&m_node);
      cable->setOutlet(2);
      cable->setInlet(2);
      system().cables.add(cable);
    }

}

void ScenarioBase::teardownConstraint(
        const Scenario::ConstraintModel& c, const Constraint& comp)
{
    auto it = m_constraints.find(&c);
    if(it != m_constraints.end()) {
        m_sliders.removeAll(it->second.mix);
        delete it->second.mix;
        m_constraints.erase(it);
    }
}

void ScenarioBase::preparePlay()
{
  m_node.exec = std::make_shared<constraint_node>();
  for(auto& cst : m_constraints)
  {
    cst.second.component->preparePlay();
    if(cst.second.mix)
      cst.second.mix->preparePlay();
  }
}

ScenarioNode::ScenarioNode(
        const DocumentPlugin& doc,
        Id<Process::Node> c,
        QObject *parent)
    : Process::Node{c, parent}
{
    auto item = new NodeItem{doc.context(), *this};
    ui = item;

    item->setParentItem(doc.window.view.contentItem());
    item->setPosition(QPointF(200, 200));
}

QString ScenarioNode::getText() const
{
  return tr("Scenario");
}

std::size_t ScenarioNode::audioInlets() const
{
  return 1;
}

std::size_t ScenarioNode::messageInlets() const
{
  return 1;
}

std::size_t ScenarioNode::midiInlets() const
{
  return 1;
}

std::size_t ScenarioNode::audioOutlets() const
{
  return 1;
}

std::size_t ScenarioNode::messageOutlets() const
{
  return 1;
}

std::size_t ScenarioNode::midiOutlets() const
{
  return 1;
}

std::vector<Process::Port> ScenarioNode::inlets() const
{
    std::vector<Process::Port> p(3);
    p[0].type = Process::PortType::Audio;
    p[1].type = Process::PortType::Message;
    p[2].type = Process::PortType::Midi;
    return p;
}

std::vector<Process::Port> ScenarioNode::outlets() const
{
  std::vector<Process::Port> p(3);
  p[0].type = Process::PortType::Audio;
  p[1].type = Process::PortType::Message;
  p[2].type = Process::PortType::Midi;
  for(auto& port : p)
    port.propagate = true;
  return p;
}

std::vector<Id<Process::Cable> > ScenarioNode::cables() const
{ return m_cables; }

void ScenarioNode::addCable(Id<Process::Cable> c)
{ m_cables.push_back(c); }

void ScenarioNode::removeCable(Id<Process::Cable> c)
{ m_cables.erase(ossia::find(m_cables, c)); }

}
