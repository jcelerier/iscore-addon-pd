#include <Pd/DataflowProcess.hpp>

namespace Dataflow
{
ProcessModel::ProcessModel(JSONObject::Deserializer& vis, QObject* parent):
  Process::DataflowProcess{vis, parent}
{
  vis.writeTo(*this);
}

ProcessModel::ProcessModel(DataStream::Deserializer& vis, QObject* parent):
  Process::DataflowProcess{vis, parent}
{
  vis.writeTo(*this);
}

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
ISCORE_LIB_PROCESS_EXPORT void DataStreamReader::read<Dataflow::ProcessModel>(const Dataflow::ProcessModel& p)
{
  m_stream << p.m_pos;
}
template<>
ISCORE_LIB_PROCESS_EXPORT void DataStreamWriter::write<Dataflow::ProcessModel>(Dataflow::ProcessModel& p)
{
  m_stream >> p.m_pos;
  p.updateCounts();
}

template<>
ISCORE_LIB_PROCESS_EXPORT void JSONObjectReader::read<Dataflow::ProcessModel>(const Dataflow::ProcessModel& p)
{
  obj["Pos"] = toJsonValue(p.m_pos);
}
template<>
ISCORE_LIB_PROCESS_EXPORT void JSONObjectWriter::write<Dataflow::ProcessModel>(Dataflow::ProcessModel& p)
{
  p.m_pos = fromJsonValue<QPointF>(obj["Pos"]);
}
