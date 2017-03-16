#pragma once
#include <ossia/editor/scenario/time_process.hpp>
#include <QJSEngine>
#include <QJSValue>
#include <QString>
#include <memory>
#include <Engine/Executor/ProcessComponent.hpp>
#include <Engine/Executor/ExecutorContext.hpp>
#include <iscore/document/DocumentContext.hpp>
#include <iscore/document/DocumentInterface.hpp>
#include <ossia/editor/scenario/time_value.hpp>

#include "z_libpd.h"
#include "m_imp.h"

namespace ossia
{
struct glutton_connection { };
struct needful_connection { };

struct delayed_glutton_connection {
  // delayed at the source or at the target
};
struct delayed_needful_connection {
  // same
};
struct parallel_connection {
};
struct replacing_connection {
};
using graph_edge = eggs::variant<
  glutton_connection,
  needful_connection,
  delayed_glutton_connection,
  delayed_needful_connection,
  parallel_connection,
  replacing_connection>;

struct audio_port {};
struct midi_port {};
struct value_port {};
using port = eggs::variant<audio_port, midi_port, value_port>;
class graph_node
{
  // Note : pour QtQuick : Faire View et Model qui hérite de View, puis faire binding automatique entre propriétés de la vue et du modèle
  // Utiliser... DSPatch ? Pd ?
  // Ports : midi, audio, value

  std::vector<port> in_ports;
  std::vector<port> out_ports;
};


class graph_process
{
  std::shared_ptr<ossia::graph_node> subProcess;
};
}

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
class ConstraintComponent;
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
                Engine::Execution::ConstraintComponent& parentConstraint,
                Pd::ProcessModel& element,
                const Engine::Execution::Context& ctx,
                const Id<iscore::Component>& id,
                QObject* parent);
};

using ComponentFactory = ::Engine::Execution::ProcessComponentFactory_T<Component>;
}
