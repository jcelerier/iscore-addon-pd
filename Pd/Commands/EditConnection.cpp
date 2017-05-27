#include <Pd/Commands/EditConnection.hpp>
#include <iscore/model/path/PathSerialization.hpp>

namespace Dataflow
{

MoveNode::MoveNode(
    const Dataflow::ProcessModel& model,
    QPointF pos)
  : m_model{model}
  , m_old{model.pos()}
  , m_new{pos}
{

}

void MoveNode::undo() const
{
  m_model.find().setPos(m_old);
}

void MoveNode::redo() const
{
  m_model.find().setPos(m_new);
}

void MoveNode::serializeImpl(DataStreamInput& s) const
{
  s << m_model << m_old << m_new;
}

void MoveNode::deserializeImpl(DataStreamOutput& s)
{
  s >> m_model >> m_old >> m_new;
}



CreateCable::CreateCable(
    const Dataflow::DocumentPlugin& dp,
    Id<Process::Cable> theCable, Process::CableData dat)
  : m_model{dp}
  , m_cable{std::move(theCable)}
  , m_dat{std::move(dat)}
{

}

void CreateCable::undo() const
{
  auto& src = m_dat.source.find().cables;
  src.erase(ossia::find(src, m_cable));

  auto& sink = m_dat.sink.find().cables;
  sink.erase(ossia::find(sink, m_cable));

  m_model.find().removeConnection(m_cable);
}

void CreateCable::redo() const
{
  auto& model = m_model.find();
  auto c = new Process::Cable{m_cable, m_dat};
  model.quiet_createConnection(c);
  model.createGuiConnection(*c);

  m_dat.source.find().cables.push_back(m_cable);
  m_dat.sink.find().cables.push_back(m_cable);
}

void CreateCable::serializeImpl(DataStreamInput& s) const
{
  s << m_model << m_cable << m_dat;
}

void CreateCable::deserializeImpl(DataStreamOutput& s)
{
  s >> m_model >> m_cable >> m_dat;
}



UpdateCable::UpdateCable(
    const Dataflow::DocumentPlugin& dp, Process::Cable& cable, Process::CableData newDat)
  : m_model{dp}
  , m_cable{cable.id()}
  , m_old{cable}
  , m_new{std::move(newDat)}
{

}

void UpdateCable::undo() const
{
  auto& dp = m_model.find();
  dp.updateConnection(dp.cables.at(m_cable), m_old);
}

void UpdateCable::redo() const
{
  auto& dp = m_model.find();
  dp.updateConnection(dp.cables.at(m_cable), m_new);
}

void UpdateCable::serializeImpl(DataStreamInput& s) const
{
  s << m_model << m_cable << m_old << m_new;
}

void UpdateCable::deserializeImpl(DataStreamOutput& s)
{
  s >> m_model >> m_cable >> m_old >> m_new;
}



RemoveCable::RemoveCable(
    const Dataflow::DocumentPlugin& dp,
    const Process::Cable& theCable)
  : m_model{dp}
 // , m_cable{std::move(cable)}
{

}

void RemoveCable::undo() const
{
//  m_model.find().createConnection(m_cable);
}

void RemoveCable::redo() const
{
//  m_model.find().removeConnection(m_cable);
}

void RemoveCable::serializeImpl(DataStreamInput& s) const
{
  s << m_model << m_cable;
}

void RemoveCable::deserializeImpl(DataStreamOutput& s)
{
  s >> m_model >> m_cable;
}

}
