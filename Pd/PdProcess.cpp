#include "PdProcess.hpp"
#include <iscore/serialization/DataStreamVisitor.hpp>
#include <iscore/serialization/JSONValueVisitor.hpp>
#include <iscore/serialization/JSONVisitor.hpp>
#include <QRegularExpression>
#include <QFile>

namespace Pd
{
ProcessModel::ProcessModel(
        const TimeVal& duration,
        const Id<Process::ProcessModel>& id,
        QObject* parent):
    Dataflow::ProcessModel{duration, id, Metadata<ObjectKey_k, ProcessModel>::get(), parent}
{
    metadata().setInstanceName(*this);
}

ProcessModel::ProcessModel(
        const ProcessModel& source,
        const Id<Process::ProcessModel>& id,
        QObject* parent):
    Dataflow::ProcessModel{source, id, Metadata<ObjectKey_k, ProcessModel>::get(), parent},
    m_script{source.m_script}
{
    metadata().setInstanceName(*this);
}

ProcessModel::~ProcessModel()
{
}

void ProcessModel::setScript(const QString& script)
{
    m_script = script;
    QFile f(m_script);
    if(f.open(QIODevice::ReadOnly))
    {
      std::vector<Process::Port> inlets;
      std::vector<Process::Port> outlets;
      auto patch = f.readAll();
      {
        QRegularExpression adc_regex{"adc~;"};
        auto m = adc_regex.match(patch);
        if(m.hasMatch())
        {
          inlets.push_back({Process::PortType::Audio, "l", {}});
          inlets.push_back({Process::PortType::Audio, "r", {}});
        }
      }

      {
        QRegularExpression dac_regex{"dac~;"};
        auto m = dac_regex.match(patch);
        if(m.hasMatch())
        {
          outlets.push_back({Process::PortType::Audio, "l", {}});
          outlets.push_back({Process::PortType::Audio, "r", {}});
        }
      }

      {
        QRegularExpression recv_regex{R"_(r \\\$0-(.*);)_"};
        auto it = recv_regex.globalMatch(patch);
        while(it.hasNext())
        {
          auto m = it.next();
          if(m.hasMatch())
          {
            auto var = m.capturedTexts()[1];
            inlets.push_back({Process::PortType::Message, var, {}});
          }
        }
      }

      {
        QRegularExpression send_regex{R"_(s \\\$0-(.*);)_"};
        auto it = send_regex.globalMatch(patch);
        while(it.hasNext())
        {
          auto m = it.next();
          if(m.hasMatch())
          {
            auto var = m.capturedTexts()[1];
            outlets.push_back({Process::PortType::Message, var, {}});
          }
        }
      }
      setInlets(inlets);
      setOutlets(outlets);
    }
    emit scriptChanged(script);
}

}

template <>
void DataStreamReader::read(const Pd::ProcessModel& proc)
{
    m_stream << proc.m_script;

    insertDelimiter();
}

template <>
void DataStreamWriter::write(Pd::ProcessModel& proc)
{
    QString str;
    m_stream >> str;
    proc.setScript(str);

    checkDelimiter();
}

template <>
void JSONObjectReader::read(const Pd::ProcessModel& proc)
{
    obj["Script"] = proc.script();
}

template <>
void JSONObjectWriter::write(Pd::ProcessModel& proc)
{
    proc.setScript(obj["Script"].toString());
}

