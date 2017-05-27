#include <Pd/DataflowProcess.hpp>

namespace Dataflow
{
QPointF ProcessModel::pos() const
{
  return m_pos;
}

void ProcessModel::setPos(QPointF pos)
{
  if (m_pos == pos)
    return;

  m_pos = pos;
  emit posChanged(pos);
}
}
