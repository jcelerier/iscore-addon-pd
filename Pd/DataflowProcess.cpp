#include <Pd/DataflowProcess.hpp>

namespace Dataflow
{

ProcessModel::ProcessModel(
        const ProcessModel& source,
        const Id<Process::ProcessModel>& id,
        const QString& name,
        QObject* parent):
  Process::ProcessModel{source, id, name, parent}
, m_inlets{source.m_inlets}
, m_outlets{source.m_outlets}
{
}

ProcessModel::~ProcessModel()
{
  emit identified_object_destroying(this);
}

void ProcessModel::setInlets(const std::vector<Port>& inlets)
{
  if(inlets != m_inlets)
  {
    m_inlets = inlets;
    emit inletsChanged();
  }
}

void ProcessModel::setOutlets(const std::vector<Port>& outlets)
{
  if(outlets != m_outlets)
  {
    m_outlets = outlets;
    emit outletsChanged();
  }
}

const std::vector<Port>&ProcessModel::inlets() const { return m_inlets; }

const std::vector<Port>&ProcessModel::outlets() const { return m_outlets; }

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

template<>
void DataStreamReader::read<Dataflow::Port>(const Dataflow::Port& p)
{

}
template<>
void DataStreamWriter::write<Dataflow::Port>(Dataflow::Port& p)
{

}

template<>
void DataStreamReader::read<Dataflow::Cable>(const Dataflow::Cable& p)
{

}
template<>
void DataStreamWriter::write<Dataflow::Cable>(Dataflow::Cable& p)
{

}
