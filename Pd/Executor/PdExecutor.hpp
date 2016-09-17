#pragma once
#include <ossia/editor/scenario/time_process.hpp>
#include <QJSEngine>
#include <QJSValue>
#include <QString>
#include <memory>
#include <Engine/Executor/ProcessElement.hpp>
#include <Engine/Executor/ExecutorContext.hpp>
#include <iscore/document/DocumentContext.hpp>
#include <iscore/document/DocumentInterface.hpp>
#include <ossia/editor/scenario/time_value.hpp>

#include "z_libpd.h"
#include "m_imp.h"

namespace Explorer
{
class DeviceDocumentPlugin;
}
namespace Device
{
class DeviceList;
}
namespace Engine { namespace Execution
{
class ConstraintElement;
} }
namespace Pd
{
class ProcessModel;
class ProcessExecutor final :
        public ossia::time_process
{
    public:
        ProcessExecutor(
                const Explorer::DeviceDocumentPlugin& devices);

        ~ProcessExecutor();

        void setTickFun(const QString& val);

        ossia::state_element state(double);
        ossia::state_element state() override;
        ossia::state_element offset(ossia::time_value) override;

    private:
        const Device::DeviceList& m_devices;
        t_pdinstance * m_instance{};
        int m_dollarzero = 0;
};


class Component final :
        public ::Engine::Execution::ProcessComponent_T<Pd::ProcessModel, ProcessExecutor>
{
        COMPONENT_METADATA("78657f42-3a2a-4b80-8736-8736463442b4")
    public:
        Component(
                Engine::Execution::ConstraintElement& parentConstraint,
                Pd::ProcessModel& element,
                const Engine::Execution::Context& ctx,
                const Id<iscore::Component>& id,
                QObject* parent);
};

using ComponentFactory = ::Engine::Execution::ProcessComponentFactory_T<Component>;
}
