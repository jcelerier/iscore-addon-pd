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
    Id<Cable> theCable, CableData dat)
  : m_model{dp}
  , m_cable{std::move(theCable)}
{

}

void CreateCable::undo() const
{
  /*
  m_model.find().removeConnection(m_cable);

  auto& src = m_cable.source.find().cables;
  src.erase(ossia::find(src, m_cable));

  auto& sink = m_cable.sink.find().cables;
  sink.erase(ossia::find(sink, m_cable));
  */
}

void CreateCable::redo() const
{
//  m_model.find().createConnection(m_cable);
//  m_cable.source.find().cables.push_back(m_cable);
//  m_cable.sink.find().cables.push_back(m_cable);
}

void CreateCable::serializeImpl(DataStreamInput& s) const
{
  s << m_model << m_cable;
}

void CreateCable::deserializeImpl(DataStreamOutput& s)
{
  s >> m_model >> m_cable;
}



UpdateCable::UpdateCable(
    const Dataflow::DocumentPlugin& dp,
    Id<Cable> theCable, CableData oldDat, CableData newDat)
  : m_model{dp}
 // , m_old{std::move(oldCable)}
 // , m_new{std::move(newCable)}
{

}

void UpdateCable::undo() const
{
//  m_model.find().updateConnection(m_new, m_old);
}

void UpdateCable::redo() const
{
//  m_model.find().updateConnection(m_old, m_new);
}

void UpdateCable::serializeImpl(DataStreamInput& s) const
{
  s << m_model << m_old << m_new;
}

void UpdateCable::deserializeImpl(DataStreamOutput& s)
{
  s >> m_model >> m_old >> m_new;
}



RemoveCable::RemoveCable(
    const Dataflow::DocumentPlugin& dp,
    const Cable& theCable)
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
