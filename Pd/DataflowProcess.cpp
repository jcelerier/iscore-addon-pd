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

}

template<>
void DataStreamReader::read<Dataflow::Port>(const Dataflow::Port& p)
{

}
template<>
void DataStreamWriter::write<Dataflow::Port>(Dataflow::Port& p)
{

}
