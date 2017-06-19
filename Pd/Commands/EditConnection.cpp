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

void MoveNode::undo(const iscore::DocumentContext& ctx) const
{
  m_model.find(ctx).setPos(m_old);
}

void MoveNode::redo(const iscore::DocumentContext& ctx) const
{
  m_model.find(ctx).setPos(m_new);
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

CreateCable::CreateCable(
    const Dataflow::DocumentPlugin& dp,
    Id<Process::Cable> theCable, const Process::Cable& cable)
  : m_model{dp}
  , m_cable{std::move(theCable)}
{
  m_dat.inlet = cable.inlet;
  m_dat.outlet = cable.outlet;
  m_dat.source = *cable.source;
  m_dat.sink = *cable.sink;
  m_dat.type = cable.type;
}

void CreateCable::undo(const iscore::DocumentContext& ctx) const
{
  auto& src = m_dat.source.find(ctx).cables;
  src.erase(ossia::find(src, m_cable));

  auto& sink = m_dat.sink.find(ctx).cables;
  sink.erase(ossia::find(sink, m_cable));

  m_model.find(ctx).removeConnection(m_cable);
}

void CreateCable::redo(const iscore::DocumentContext& ctx) const
{
  auto& model = m_model.find(ctx);
  auto c = new Process::Cable{ctx, m_cable, m_dat};
  model.quiet_createConnection(c);
  model.createGuiConnection(*c);

  m_dat.source.find(ctx).cables.push_back(m_cable);
  m_dat.sink.find(ctx).cables.push_back(m_cable);
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
  , m_new{std::move(newDat)}
{
  m_old.inlet = cable.inlet;
  m_old.outlet = cable.outlet;
  m_old.source = *cable.source;
  m_old.sink = *cable.sink;
  m_old.type = cable.type;
}

void UpdateCable::undo(const iscore::DocumentContext& ctx) const
{
  auto& dp = m_model.find(ctx);
  dp.updateConnection(dp.cables.at(m_cable), m_old);
}

void UpdateCable::redo(const iscore::DocumentContext& ctx) const
{
  auto& dp = m_model.find(ctx);
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

void RemoveCable::undo(const iscore::DocumentContext& ctx) const
{
//  m_model.find(ctx).createConnection(m_cable);
}

void RemoveCable::redo(const iscore::DocumentContext& ctx) const
{
//  m_model.find(ctx).removeConnection(m_cable);
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
