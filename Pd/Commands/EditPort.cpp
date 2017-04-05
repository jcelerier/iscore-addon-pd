#include <Pd/Commands/EditPort.hpp>
#include <iscore/model/path/PathSerialization.hpp>
namespace Dataflow
{

AddPort::AddPort(
        const Dataflow::ProcessModel& model, bool inlet)
    : m_model{model}
    , m_inlet{inlet}
{

}

void AddPort::undo() const
{
    auto& m = m_model.find();
    std::vector<Dataflow::Port> vec = m_inlet ? m.inlets() : m.outlets();

    vec.pop_back();

    if(m_inlet) m.setInlets(std::move(vec));
    else m.setOutlets(std::move(vec));
}

void AddPort::redo() const
{
    auto& m = m_model.find();
    std::vector<Dataflow::Port> vec = m_inlet ? m.inlets() : m.outlets();

    vec.push_back(Dataflow::Port{Dataflow::PortType::Message, {}});

    if(m_inlet) m.setInlets(std::move(vec));
    else m.setOutlets(std::move(vec));
}

void AddPort::serializeImpl(DataStreamInput& s) const
{
    s << m_model << m_inlet;
}

void AddPort::deserializeImpl(DataStreamOutput& s)
{
    s >> m_model >> m_inlet;
}


EditPort::EditPort(
        const Dataflow::ProcessModel& model,
        Dataflow::Port next,
        std::size_t index, bool inlet)
    : m_model{model}
    , m_new{std::move(next)}
    , m_index{index}
    , m_inlet{inlet}
{
  m_old = inlet ? model.inlets()[index] : model.outlets()[index];

  auto isAudio = [] (const State::Address& addr) { return addr.device == "audio"; };
  auto isMidi = [] (const State::Address& addr) { return addr.device == "midi"; };

  if(m_new.type == PortType::Message)
  {
    if(isAudio(m_new.address.address))
      m_new.type = PortType::Audio;
    else if(isMidi(m_new.address.address))
      m_new.type = PortType::Midi;
  }
}

void EditPort::undo() const
{
    auto& m = m_model.find();
    std::vector<Dataflow::Port> vec = m_inlet ? m.inlets() : m.outlets();

    vec[m_index] = m_old;

    if(m_inlet) m.setInlets(std::move(vec));
    else m.setOutlets(std::move(vec));
}

void EditPort::redo() const
{
    auto& m = m_model.find();
    std::vector<Dataflow::Port> vec = m_inlet ? m.inlets() : m.outlets();

    vec[m_index] = m_new;

    if(m_inlet) m.setInlets(std::move(vec));
    else m.setOutlets(std::move(vec));
}

void EditPort::serializeImpl(DataStreamInput& s) const
{
    s << m_model << m_old << m_new << m_index << m_inlet;
}

void EditPort::deserializeImpl(DataStreamOutput& s)
{
    s >> m_model >> m_old >> m_new >> m_index >> m_inlet;
}



RemovePort::RemovePort(
        const Dataflow::ProcessModel& model,
        std::size_t index, bool inlet)
    : m_model{model}
    , m_index{index}
    , m_inlet{inlet}
{
    m_old = inlet ? model.inlets()[index] : model.outlets()[index];
}

void RemovePort::undo() const
{
    auto& m = m_model.find();
    std::vector<Dataflow::Port> vec = m_inlet ? m.inlets() : m.outlets();

    vec.insert(vec.begin() + m_index, m_old);

    if(m_inlet) m.setInlets(std::move(vec));
    else m.setOutlets(std::move(vec));
}

void RemovePort::redo() const
{
    auto& m = m_model.find();
    std::vector<Dataflow::Port> vec = m_inlet ? m.inlets() : m.outlets();

    vec.erase(vec.begin() + m_index);

    if(m_inlet) m.setInlets(std::move(vec));
    else m.setOutlets(std::move(vec));
}

void RemovePort::serializeImpl(DataStreamInput& s) const
{
    s << m_model << m_old << m_index << m_inlet;
}

void RemovePort::deserializeImpl(DataStreamOutput& s)
{
    s >> m_model >> m_old >> m_index >> m_inlet;
}



}
