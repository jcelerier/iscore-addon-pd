#pragma once
#include <Pd/PdMetadata.hpp>
#include <Pd/DataflowProcess.hpp>
#include <Process/Process.hpp>
#include <Process/WidgetLayer/WidgetProcessFactory.hpp>
#include <Process/WidgetLayer/WidgetLayerPresenter.hpp>
#include <Process/WidgetLayer/WidgetLayerView.hpp>
#include <Process/TimeValue.hpp>
#include <iscore/model/Identifier.hpp>
#include <iscore/selection/Selection.hpp>
#include <iscore/serialization/VisitorInterface.hpp>
class DataStream;
class JSONObject;

namespace Pd
{
class ProcessModel final : public Dataflow::ProcessModel
{
        ISCORE_SERIALIZE_FRIENDS
        PROCESS_METADATA_IMPL(Pd::ProcessModel)
    Q_OBJECT
    public:
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
            Dataflow::ProcessModel{vis, parent}
        {
            vis.writeTo(*this);
        }

        void setScript(const QString& script);
        const QString& script() const
        { return m_script; }

        ~ProcessModel();

    signals:
        void scriptChanged(QString);

    private:
        QString m_script;
};

}
