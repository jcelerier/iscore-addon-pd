#include <Pd/DataflowProcess.hpp>
#include <iscore/model/path/PathSerialization.hpp>
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
  updateCounts();
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
    updateCounts();
    emit inletsChanged();
  }
}

void ProcessModel::setOutlets(const std::vector<Port>& outlets)
{
  if(outlets != m_outlets)
  {
    m_outlets = outlets;
    updateCounts();
    emit outletsChanged();
  }
}

const std::vector<Port>& ProcessModel::inlets() const { return m_inlets; }

const std::vector<Port>& ProcessModel::outlets() const { return m_outlets; }

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

void ProcessModel::updateCounts()
{
  m_portCount = {};
  for(auto& p : m_inlets)
  {
    switch(p.type)
    {
      case PortType::Midi:
        m_portCount.midiIn++; break;
      case PortType::Audio:
        m_portCount.audioIn++; break;
      case PortType::Message:
        m_portCount.messageIn++; break;
    }
  }

  for(auto& p : m_outlets)
  {
    switch(p.type)
    {
      case PortType::Midi:
        m_portCount.midiOut++; break;
      case PortType::Audio:
        m_portCount.audioOut++; break;
      case PortType::Message:
        m_portCount.messageOut++; break;
    }
  }
}

}

template<>
void DataStreamReader::read<Dataflow::Port>(const Dataflow::Port& p)
{
  m_stream << p.type << p.customData << p.address;
}
template<>
void DataStreamWriter::write<Dataflow::Port>(Dataflow::Port& p)
{
  m_stream >> p.type >> p.customData >> p.address;
}

template<>
void DataStreamReader::read<Dataflow::Cable>(const Dataflow::Cable& p)
{
  m_stream << p.type << p.source << p.sink << p.outlet << p.inlet;
}
template<>
void DataStreamWriter::write<Dataflow::Cable>(Dataflow::Cable& p)
{
  m_stream >> p.type >> p.source >> p.sink >> p.outlet >> p.inlet;
}
