#include "PdProcess.hpp"
#include <score/serialization/DataStreamVisitor.hpp>
#include <score/serialization/JSONValueVisitor.hpp>
#include <score/serialization/JSONVisitor.hpp>
#include <Process/Dataflow/Port.hpp>
#include <QRegularExpression>
#include <QFile>
#include <score/tools/File.hpp>

namespace Pd
{
ProcessModel::ProcessModel(
    const TimeVal& duration,
    const Id<Process::ProcessModel>& id,
    QObject* parent):
  Process::ProcessModel{duration, id, Metadata<ObjectKey_k, ProcessModel>::get(), parent}
{
  metadata().setInstanceName(*this);
}

ProcessModel::ProcessModel(
    const ProcessModel& source,
    const Id<Process::ProcessModel>& id,
    QObject* parent):
  Process::ProcessModel{source, id, Metadata<ObjectKey_k, ProcessModel>::get(), parent},
  m_script{source.m_script}
{
  metadata().setInstanceName(*this);
}

ProcessModel::~ProcessModel()
{
}

Process::Inlets ProcessModel::inlets() const { return m_inlets; }
Process::Outlets ProcessModel::outlets() const { return m_outlets; }

int ProcessModel::audioInputs() const
{
  return m_audioInputs;
}

int ProcessModel::audioOutputs() const
{
  return m_audioOutputs;
}

bool ProcessModel::midiInput() const
{
  return m_midiInput;
}

bool ProcessModel::midiOutput() const
{
  return m_midiOutput;
}

void ProcessModel::setAudioInputs(int audioInputs)
{
  if (m_audioInputs == audioInputs)
    return;

  m_audioInputs = audioInputs;
  emit audioInputsChanged(m_audioInputs);
}

void ProcessModel::setAudioOutputs(int audioOutputs)
{
  if (m_audioOutputs == audioOutputs)
    return;

  m_audioOutputs = audioOutputs;
  emit audioOutputsChanged(m_audioOutputs);
}

void ProcessModel::setMidiInput(bool midiInput)
{
  if (m_midiInput == midiInput)
    return;

  m_midiInput = midiInput;
  emit midiInputChanged(m_midiInput);
}

void ProcessModel::setMidiOutput(bool midiOutput)
{
  if (m_midiOutput == midiOutput)
    return;

  m_midiOutput = midiOutput;
  emit midiOutputChanged(m_midiOutput);
}

void ProcessModel::setScript(const QString& script)
{
  setMidiInput(false);
  setMidiOutput(false);
  for(auto p : m_inlets)
    delete p;
  m_inlets.clear();
  for(auto p : m_outlets)
    delete p;
  m_outlets.clear();

  m_script = score::locateFilePath(script, score::IDocument::documentContext(*this));
  QFile f(m_script);
  if(f.open(QIODevice::ReadOnly))
  {
    int i = 0;
    auto get_next_id = [&] {
      i++;
      return Id<Process::Port>(i);
    };

    auto patch = f.readAll();
    {
      static const QRegularExpression adc_regex{"adc~"};
      auto m = adc_regex.match(patch);
      if(m.hasMatch())
      {
        auto p = new Process::Inlet{get_next_id(), this};
        p->type = Process::PortType::Audio;
        p->setCustomData("Audio In");
        setAudioInputs(2);
        m_inlets.push_back(p);
      }
    }

    {
      static const QRegularExpression dac_regex{"dac~"};
      auto m = dac_regex.match(patch);
      if(m.hasMatch())
      {
        auto p = new Process::Outlet{get_next_id(), this};
        p->setPropagate(true);
        p->type = Process::PortType::Audio;
        p->setCustomData("Audio Out");
        setAudioOutputs(2);
        m_outlets.push_back(p);
      }
    }

    {
      static const QRegularExpression midi_regex{"(midiin|notein|controlin)"};
      auto m = midi_regex.match(patch);
      if(m.hasMatch())
      {
        auto p = new Process::Inlet{get_next_id(), this};
        p->type = Process::PortType::Midi;
        p->setCustomData("Midi In");
        m_inlets.push_back(p);

        setMidiInput(true);
      }
    }

    {
      static const QRegularExpression midi_regex{"(midiiout|noteout|controlout)"};
      auto m = midi_regex.match(patch);
      if(m.hasMatch())
      {
        auto p = new Process::Outlet{get_next_id(), this};
        p->type = Process::PortType::Midi;
        p->setCustomData("Midi Out");
        m_outlets.push_back(p);

        setMidiOutput(true);
      }
    }

    {
      static const QRegularExpression recv_regex{R"_(r \\\$0-(.*);)_"};
      auto it = recv_regex.globalMatch(patch);
      while(it.hasNext())
      {
        auto m = it.next();
        if(m.hasMatch())
        {
          auto var = m.capturedTexts()[1];

          auto p = new Process::Inlet{get_next_id(), this};
          p->type = Process::PortType::Message;
          p->setCustomData(var);
          m_inlets.push_back(p);
        }
      }
    }

    {
      static const QRegularExpression send_regex{R"_(s \\\$0-(.*);)_"};
      auto it = send_regex.globalMatch(patch);
      while(it.hasNext())
      {
        auto m = it.next();
        if(m.hasMatch())
        {
          auto var = m.capturedTexts()[1];

          auto p = new Process::Outlet{get_next_id(), this};
          p->type = Process::PortType::Message;
          p->setCustomData(var);
          m_outlets.push_back(p);
        }
      }
    }
  }

  emit inletsChanged();
  emit outletsChanged();
  emit scriptChanged(script);
}

const QString&ProcessModel::script() const
{ return m_script; }

}

template <>
void DataStreamReader::read(const Pd::ProcessModel& proc)
{
  insertDelimiter();

  m_stream << (int32_t)proc.m_inlets.size();
  for(auto v : proc.m_inlets)
    m_stream << *v;
  m_stream << (int32_t)proc.m_outlets.size();
  for(auto v : proc.m_outlets)
    m_stream << *v;

  m_stream << proc.m_script
           << proc.m_audioInputs
           << proc.m_audioOutputs
           << proc.m_midiInput
           << proc.m_midiOutput;

  insertDelimiter();
}

template <>
void DataStreamWriter::write(Pd::ProcessModel& proc)
{
  checkDelimiter();

  {
    int32_t ports;
    m_stream >> ports;
    for(auto i = ports; i-->0;) {
      proc.m_inlets.push_back(new Process::Inlet{*this, &proc});
    }
  }
  {
    int32_t ports;
    m_stream >> ports;
    for(auto i = ports; i-->0;) {
      proc.m_outlets.push_back(new Process::Outlet{*this, &proc});
    }
  }

  m_stream
      >> proc.m_script
      >> proc.m_audioInputs
      >> proc.m_audioOutputs
      >> proc.m_midiInput
      >> proc.m_midiOutput;

  checkDelimiter();
}

template <>
void JSONObjectReader::read(const Pd::ProcessModel& proc)
{
  obj["Inlets"] = toJsonArray(proc.m_inlets);
  obj["Outlets"] = toJsonArray(proc.m_outlets);
  obj["Script"] = proc.script();
  obj["AudioInputs"] = proc.audioInputs();
  obj["AudioOutputs"] = proc.audioOutputs();
  obj["MidiInput"] = proc.midiInput();
  obj["MidiOutput"] = proc.midiOutput();
}

template <>
void JSONObjectWriter::write(Pd::ProcessModel& proc)
{
  fromJsonArray(obj["Inlets"].toArray(), proc.m_inlets, &proc);
  fromJsonArray(obj["Outlets"].toArray(), proc.m_outlets, &proc);
  proc.m_script = obj["Script"].toString();
  proc.m_audioInputs = obj["AudioInputs"].toInt();
  proc.m_audioOutputs = obj["AudioOutputs"].toInt();
  proc.m_midiInput = obj["MidiInput"].toBool();
  proc.m_midiOutput = obj["MidiOutput"].toBool();
}

