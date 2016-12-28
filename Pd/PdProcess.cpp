#include "PdProcess.hpp"
#include <iscore/serialization/DataStreamVisitor.hpp>
#include <iscore/serialization/JSONValueVisitor.hpp>
#include <iscore/serialization/JSONVisitor.hpp>


namespace Pd
{
ProcessModel::ProcessModel(
        const TimeValue& duration,
        const Id<Process::ProcessModel>& id,
        QObject* parent):
    Process::ProcessModel{duration, id, Metadata<ObjectKey_k, ProcessModel>::get(), parent}
{
    m_script = "(function(t) { \n"
               "     var obj = new Object; \n"
               "     obj[\"address\"] = 'OSCdevice:/millumin/layer/x/instance'; \n"
               "     obj[\"value\"] = t + iscore.value('OSCdevice:/millumin/layer/y/instance'); \n"
               "     return [ obj ]; \n"
               "});";

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

void ProcessModel::setScript(const QString& script)
{
    m_script = script;
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

