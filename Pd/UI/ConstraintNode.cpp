#include "ConstraintNode.hpp"
#include <Pd/DocumentPlugin.hpp>
#include <Pd/PdProcess.hpp>
#include <Scenario/Document/Components/ConstraintComponent.hpp>
#include <Scenario/Document/Components/ProcessComponent.hpp>
#include <Process/Process.hpp>
#include <iscore/model/Component.hpp>
#include <iscore/model/ComponentHierarchy.hpp>
#include <iscore/plugins/customfactory/ModelFactory.hpp>
#include <iscore/model/ComponentFactory.hpp>
#include <Process/Dataflow/DataflowObjects.hpp>
#include <Pd/UI/Slider.hpp>
#include <Pd/UI/NodeItem.hpp>

namespace Dataflow
{
ConstraintNode::ConstraintNode(DocumentPlugin &doc, Id<Process::Node> c, QObject *parent)
    : Process::Node{c, parent}
{
    qDebug("constraint node created");
    auto item = new NodeItem{doc.context(), *this};
    ui = item;
    item->setParentItem(doc.window.view.contentItem());
    item->setPosition(QPointF(300, 300));
}

QString ConstraintNode::getText() const
{
    return tr("Constraint");
}

std::size_t ConstraintNode::audioInlets() const
{
    return 1;
}

std::size_t ConstraintNode::messageInlets() const
{
    return 1;
}

std::size_t ConstraintNode::midiInlets() const
{
    return 1;
}

std::size_t ConstraintNode::audioOutlets() const
{
    return 1;
}

std::size_t ConstraintNode::messageOutlets() const
{
    return 1;
}

std::size_t ConstraintNode::midiOutlets() const
{
    return 1;
}

std::vector<Process::Port> ConstraintNode::inlets() const
{
    std::vector<Process::Port> p(3);
    p[0].type = Process::PortType::Audio;
    p[1].type = Process::PortType::Message;
    p[2].type = Process::PortType::Midi;
    return p;
}

std::vector<Process::Port> ConstraintNode::outlets() const
{
  std::vector<Process::Port> p(3);
  p[0].type = Process::PortType::Audio;
  p[1].type = Process::PortType::Message;
  p[2].type = Process::PortType::Midi;
  for(auto& port : p)
    port.propagate = true;
  return p;
}

std::vector<Id<Process::Cable> > ConstraintNode::cables() const
{ return m_cables; }

void ConstraintNode::addCable(Id<Process::Cable> c)
{ m_cables.push_back(c); }

void ConstraintNode::removeCable(Id<Process::Cable> c)
{ m_cables.erase(ossia::find(m_cables, c)); }



ConstraintBase::ConstraintBase(
    const Id<iscore::Component>& id,
    Scenario::ConstraintModel& constraint,
    ConstraintBase::DocumentPlugin& doc,
    QObject* parent_comp):
  parent_t{constraint, doc, id, "ConstraintComponent", parent_comp}
, m_node{doc, Id<Process::Node>{}, this}
{
}

void ConstraintBase::setupProcess(
        Process::ProcessModel &c,
        Dataflow::ProcessComponent*comp)
{
  Slider* slider{};
  if(comp->mainNode().audioOutlets() > 0)
  {
    slider = new Slider{system(), getStrongId(m_sliders), this};
    {
      // Create a cable from the slider to the constraint
      auto cable = new Process::Cable{getStrongId(system().cables)};
      cable->setSource(slider);
      cable->setSink(&m_node);
      cable->setOutlet(0);
      cable->setInlet(0);
      system().cables.add(cable);
    }
    m_processes.insert({&c, {comp, slider}});
    m_sliders.push_back(slider);
  }

  // Connect process to slider
  // When the process's outputs change, create new cables
  auto& compNode = comp->mainNode();
  const auto& outlets = compNode.outlets();

  for(std::size_t i = 0; i < outlets.size(); i++)
  {
    const Process::Port& outlet = outlets[i];
    if(outlet.propagate)
    {
      auto cable = new Process::Cable{getStrongId(system().cables)};
      cable->setSource(&compNode);
      cable->setOutlet(i);
      switch(outlet.type)
      {
        case Process::PortType::Audio:
        {
          ISCORE_ASSERT(slider);
          {
            cable->setSink(slider);
            cable->setInlet(0);
          }
          break;
        }
        case Process::PortType::Message:
        {
          cable->setSink(&m_node);
          cable->setInlet(1);
          break;
        }
        case Process::PortType::Midi:
        {
          cable->setSink(&m_node);
          cable->setInlet(2);
          break;
        }
      }
      system().cables.add(cable);
    }
  }
}

void ConstraintBase::teardownProcess(
        const Process::ProcessModel& c, const Dataflow::ProcessComponent& comp)
{
    auto it = m_processes.find(&c);
    if(it != m_processes.end()) {
        m_sliders.removeAll(it->second.mix);
        delete it->second.mix;
        m_processes.erase(it);
    }
}
ProcessComponent*ConstraintBase::make(
    const Id<iscore::Component>& id,
    ProcessComponentFactory& factory,
    Process::ProcessModel& process)
{
  auto comp = factory.make(process, system(), id, &process);
  setupProcess(process, comp);
  return comp;
}

bool ConstraintBase::removing(
    const Process::ProcessModel& cst,
    const ProcessComponent& comp)
{
  teardownProcess(cst, comp);
  return true;
}

}
