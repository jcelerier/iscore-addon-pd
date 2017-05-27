#pragma once
#include <Pd/DataflowProcess.hpp>
#include <Pd/Commands/PdCommandFactory.hpp>
#include <Process/Process.hpp>
#include <iscore/command/Command.hpp>
#include <iscore/model/path/Path.hpp>

namespace Dataflow
{
class AddPort final : public iscore::Command
{
    ISCORE_COMMAND_DECL(Pd::CommandFactoryName(), AddPort, "Add a node port")

    public:
        AddPort(const Dataflow::ProcessModel& model, bool inlet);

    void undo() const override;
    void redo() const override;

protected:
    void serializeImpl(DataStreamInput& s) const override;
    void deserializeImpl(DataStreamOutput& s) override;

private:
    Path<Dataflow::ProcessModel> m_model;
    bool m_inlet{}; // true : inlet ; false : outlet

};

class EditPort final : public iscore::Command
{
    ISCORE_COMMAND_DECL(Pd::CommandFactoryName(), EditPort, "Edit a node port")
    public:
        EditPort(const Dataflow::ProcessModel& model,
                 Process::Port next,
                 std::size_t index, bool inlet);

    void undo() const override;
    void redo() const override;

protected:
    void serializeImpl(DataStreamInput& s) const override;
    void deserializeImpl(DataStreamOutput& s) override;

private:
    Path<Dataflow::ProcessModel> m_model;

    Process::Port m_old, m_new;
    quint64 m_index{};
    bool m_inlet{}; // true : inlet ; false : outlet
};


class RemovePort final : public iscore::Command
{
    ISCORE_COMMAND_DECL(Pd::CommandFactoryName(), RemovePort, "Remove a node port")

    public:
        RemovePort(const Dataflow::ProcessModel& model,
                   std::size_t index, bool inlet);

    void undo() const override;
    void redo() const override;

protected:
    void serializeImpl(DataStreamInput& s) const override;
    void deserializeImpl(DataStreamOutput& s) override;

private:
    Path<Dataflow::ProcessModel> m_model;
    Process::Port m_old;
    quint64 m_index{};
    bool m_inlet{}; // true : inlet ; false : outlet
};

}
