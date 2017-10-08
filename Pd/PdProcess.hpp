#pragma once
#include <Pd/PdMetadata.hpp>
#include <Dataflow/UI/DataflowProcessNode.hpp>
#include <Process/Process.hpp>
#include <Process/WidgetLayer/WidgetProcessFactory.hpp>
#include <Process/WidgetLayer/WidgetLayerPresenter.hpp>
#include <Process/WidgetLayer/WidgetLayerView.hpp>
#include <Process/TimeValue.hpp>
#include <score/model/Identifier.hpp>
#include <score/selection/Selection.hpp>
#include <score/serialization/VisitorInterface.hpp>
class DataStream;
class JSONObject;

namespace Pd
{
class ProcessModel final : public Process::ProcessModel
{
    SCORE_SERIALIZE_FRIENDS
    PROCESS_METADATA_IMPL(Pd::ProcessModel)
    Q_OBJECT
    Q_PROPERTY(int audioInputs READ audioInputs WRITE setAudioInputs NOTIFY audioInputsChanged)
    Q_PROPERTY(int audioOutputs READ audioOutputs WRITE setAudioOutputs NOTIFY audioOutputsChanged)
    Q_PROPERTY(bool midiInput READ midiInput WRITE setMidiInput NOTIFY midiInputChanged)
    Q_PROPERTY(bool midiOutput READ midiOutput WRITE setMidiOutput NOTIFY midiOutputChanged)

    public:
      using base_type = Process::ProcessModel;
    explicit ProcessModel(
        const TimeVal& duration,
        const Id<Process::ProcessModel>& id,
        QObject* parent);

    explicit ProcessModel(
        const ProcessModel& source,
        const Id<Process::ProcessModel>& id,
        QObject* parent);

    template<typename Impl>
    explicit ProcessModel(
        Impl& vis,
        QObject* parent) :
      Process::ProcessModel{vis, parent}
    {
      vis.writeTo(*this);
    }

    void setScript(const QString& script);
    const QString& script() const;

    ~ProcessModel() override;

    std::vector<Process::Port*> inlets() const override;
    std::vector<Process::Port*> outlets() const override;

    int audioInputs() const;
    int audioOutputs() const;

    bool midiInput() const
    {
      return m_midiInput;
    }

    bool midiOutput() const
    {
      return m_midiOutput;
    }

  public slots:
    void setAudioInputs(int audioInputs);
    void setAudioOutputs(int audioOutputs);

    void setMidiInput(bool midiInput)
    {
      if (m_midiInput == midiInput)
        return;

      m_midiInput = midiInput;
      emit midiInputChanged(m_midiInput);
    }

    void setMidiOutput(bool midiOutput)
    {
      if (m_midiOutput == midiOutput)
        return;

      m_midiOutput = midiOutput;
      emit midiOutputChanged(m_midiOutput);
    }

  signals:
    void scriptChanged(QString);
    void audioInputsChanged(int audioInputs);
    void audioOutputsChanged(int audioOutputs);

    void midiInputChanged(bool midiInput);

    void midiOutputChanged(bool midiOutput);

  private:
    std::vector<Process::Port*> m_inlets;
    std::vector<Process::Port*> m_outlets;
    QString m_script;
    int m_audioInputs{0};
    int m_audioOutputs{0};
    bool m_midiInput{};
    bool m_midiOutput{};
};

}
