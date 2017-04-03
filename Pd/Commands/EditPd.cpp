#include <Pd/Commands/EditPd.hpp>
#include <iscore/model/path/PathSerialization.hpp>

namespace Pd
{

EditPdPath::EditPdPath(
        const Pd::ProcessModel& model,
        QString newpath)
    : m_model{model}
    , m_old{model.script()}
    , m_new{std::move(newpath)}
{

}

void EditPdPath::undo() const
{
    m_model.find().setScript(m_old);
}

void EditPdPath::redo() const
{
    m_model.find().setScript(m_new);
}

void EditPdPath::serializeImpl(DataStreamInput& s) const
{
    s << m_model << m_old << m_new;
}

void EditPdPath::deserializeImpl(DataStreamOutput& s)
{
    s >> m_model >> m_old >> m_new;
}

}
