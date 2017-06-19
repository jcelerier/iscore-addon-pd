#pragma once
#include <Pd/PdProcess.hpp>
#include <Pd/Commands/PdCommandFactory.hpp>
#include <iscore/model/path/Path.hpp>

namespace Pd
{
class EditPdPath final : public iscore::Command
{
    ISCORE_COMMAND_DECL(Pd::CommandFactoryName(), EditPdPath, "Edit path to Pd file")

    public:
        EditPdPath(const Pd::ProcessModel& model, QString newpath);

    void undo(const iscore::DocumentContext& ctx) const override;
    void redo(const iscore::DocumentContext& ctx) const override;

protected:
    void serializeImpl(DataStreamInput& s) const override;
    void deserializeImpl(DataStreamOutput& s) override;

private:
    Path<Pd::ProcessModel> m_model;
    QString m_old, m_new;

};
}
