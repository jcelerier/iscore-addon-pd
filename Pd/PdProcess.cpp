#include "PdProcess.hpp"
#include <score/serialization/DataStreamVisitor.hpp>
#include <score/serialization/JSONValueVisitor.hpp>
#include <score/serialization/JSONVisitor.hpp>
#include <QRegularExpression>
#include <QFile>

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

std::vector<Process::Port*> ProcessModel::inlets() const { return m_inlets; }
std::vector<Process::Port*> ProcessModel::outlets() const { return m_outlets; }

int ProcessModel::audioInputs() const
{
  return m_audioInputs;
}

int ProcessModel::audioOutputs() const
{
  return m_audioOutputs;
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

  m_script = script;
  QFile f(m_script);
  if(f.open(QIODevice::ReadOnly))
  {
    int i = 0;
    int inl = 0;
    int outl = 0;
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
        auto p = new Process::Port{get_next_id(), this};
        p->outlet = false;
        p->type = Process::PortType::Audio;
        p->num = inl++;
        m_inlets.push_back(p);
      }
    }

    {
      static const QRegularExpression dac_regex{"dac~"};
      auto m = dac_regex.match(patch);
      if(m.hasMatch())
      {
        auto p = new Process::Port{get_next_id(), this};
        p->outlet = true;
        p->propagate = true;
        p->type = Process::PortType::Audio;
        p->num = outl++;
        m_outlets.push_back(p);
      }
    }

    {
      static const QRegularExpression midi_regex{"(midiin|notein|controlin)"};
      auto m = midi_regex.match(patch);
      if(m.hasMatch())
      {
        auto p = new Process::Port{get_next_id(), this};
        p->outlet = false;
        p->type = Process::PortType::Midi;
        p->num = inl++;
        m_inlets.push_back(p);

        setMidiInput(true);
      }
    }

    {
      static const QRegularExpression midi_regex{"(midiiout|noteout|controlout)"};
      auto m = midi_regex.match(patch);
      if(m.hasMatch())
      {
        auto p = new Process::Port{get_next_id(), this};
        p->outlet = true;
        p->type = Process::PortType::Midi;
        p->num = outl++;
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

          auto p = new Process::Port{get_next_id(), this};
          p->outlet = false;
          p->type = Process::PortType::Message;
          p->num = inl++;
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

          auto p = new Process::Port{get_next_id(), this};
          p->outlet = true;
          p->type = Process::PortType::Message;
          p->num = outl++;
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
  {
    int32_t ports;
    m_stream >> ports;
    for(auto i = ports; i-->0;) {
      proc.m_inlets.push_back(new Process::Port{*this, &proc});
    }
  }
  {
    int32_t ports;
    m_stream >> ports;
    for(auto i = ports; i-->0;) {
      proc.m_outlets.push_back(new Process::Port{*this, &proc});
    }
  }
  QString str;
  m_stream
      >> str
      >> proc.m_audioInputs
      >> proc.m_audioOutputs
      >> proc.m_midiInput
      >> proc.m_midiOutput;
  proc.setScript(str);

  checkDelimiter();
}

template<typename T>
QJsonArray toJsonArray(const std::vector<T*>& array)
{
  QJsonArray arr;
  for (auto& v : array)
    arr.push_back(toJsonObject(*v));
  return arr;
}

template<typename T>
void fromJsonArray(const QJsonArray& arr, std::vector<T*>& array, QObject* parent)
{
  for (const auto& v : arr)
  {
    auto obj = v.toObject();
    array.push_back(new T{JSONObjectWriter{obj}, parent});
  }
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
  proc.setScript(obj["Script"].toString());
  proc.m_audioInputs = obj["AudioInputs"].toInt();
  proc.m_audioOutputs = obj["AudioOutputs"].toInt();
  proc.m_midiInput = obj["MidiInput"].toBool();
  proc.m_midiOutput = obj["MidiOutput"].toBool();
}

