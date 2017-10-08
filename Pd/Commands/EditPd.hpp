#pragma once
#include <Pd/PdProcess.hpp>
#include <Pd/Commands/PdCommandFactory.hpp>
#include <score/model/path/Path.hpp>
#include <score/command/PropertyCommand.hpp>

namespace Pd
{
class EditPdPath final : public score::Command
{
    SCORE_COMMAND_DECL(Pd::CommandFactoryName(), EditPdPath, "Edit path to Pd file")

    public:
        EditPdPath(const Pd::ProcessModel& model, QString newpath);

    void undo(const score::DocumentContext& ctx) const override;
    void redo(const score::DocumentContext& ctx) const override;

protected:
    void serializeImpl(DataStreamInput& s) const override;
    void deserializeImpl(DataStreamOutput& s) override;

private:
    Path<Pd::ProcessModel> m_model;
    QString m_old, m_new;

};

class SetAudioIns final
    : public score::PropertyCommand
{
  SCORE_COMMAND_DECL(Pd::CommandFactoryName(), SetAudioIns, "Set audio ins")
public:
  SetAudioIns(const ProcessModel& path, int newval)
      : score::PropertyCommand{std::move(path), "audioInputs", newval}
  {
  }
};
class SetAudioOuts final
    : public score::PropertyCommand
{
    SCORE_COMMAND_DECL(Pd::CommandFactoryName(), SetAudioOuts, "Set audio outs")
    public:
      SetAudioOuts(const ProcessModel& path, int newval)
    : score::PropertyCommand{std::move(path), "audioOutputs", newval}
{
}
};
class SetMidiIn final
    : public score::PropertyCommand
{
  SCORE_COMMAND_DECL(Pd::CommandFactoryName(), SetMidiIn, "Set midi in")
public:
  SetMidiIn(const ProcessModel& path, bool newval)
      : score::PropertyCommand{std::move(path), "midiInputs", newval}
  {
  }
};
class SetMidiOut final
    : public score::PropertyCommand
{
    SCORE_COMMAND_DECL(Pd::CommandFactoryName(), SetMidiOut, "Set midi out")
    public:
      SetMidiOut(const ProcessModel& path, bool newval)
    : score::PropertyCommand{std::move(path), "midiOutputs", newval}
{
}
};
}
