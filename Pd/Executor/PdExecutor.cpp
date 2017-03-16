#include <Pd/Executor/PdExecutor.hpp>

#include <Explorer/DocumentPlugin/DeviceDocumentPlugin.hpp>
#include <Engine/OSSIA2iscore.hpp>
#include <Engine/iscore2OSSIA.hpp>
#include <Engine/Executor/DocumentPlugin.hpp>
#include <vector>

#include <ossia/editor/state/message.hpp>
#include <ossia/editor/state/state.hpp>
#include <ossia/editor/scenario/time_constraint.hpp>
#include <Pd/PdProcess.hpp>


namespace Pd
{


ProcessExecutor::ProcessExecutor(
        const Explorer::DeviceDocumentPlugin& devices):
    m_devices{devices.list()}
{
    m_instance = pdinstance_new();
    pd_setinstance(m_instance);

    // compute audio    [; pd dsp 1(
    libpd_start_message(1); // one entry in list
    libpd_add_float(1.0f);
    libpd_finish_message("pd", "dsp");
}

ProcessExecutor::~ProcessExecutor()
{
    pdinstance_free(m_instance);
}

void ProcessExecutor::setTickFun(const QString& val)
{
    pd_setinstance(m_instance);

    auto handle = libpd_openfile("ex.pd", "/tmp");
    m_dollarzero = libpd_getdollarzero(handle);
    auto mess = std::to_string(m_dollarzero) + "-out";
    libpd_bind(mess.c_str());
    libpd_set_floathook([] (const char *recv, float f) {
      qDebug() << "received float: " << recv <<  f;
    });

    libpd_set_printhook([] (const char *recv) {
      ossia::logger().info("Pd: {}", recv);
    });
}

ossia::state_element ProcessExecutor::state()
{
    return state(parent()->getDate() / parent()->getDurationNominal());
}

ossia::state_element ProcessExecutor::state(double t)
{
    pd_setinstance(m_instance);
    auto mess = std::to_string(m_dollarzero) + "-time";
/*
    libpd_start_message(1); // one entry in list
    libpd_add_float(t);
    libpd_finish_message(mess.c_str(), "float");
    */
    libpd_float(mess.c_str(), t);


    float inbuf[64], outbuf[128];  // one input channel, two output channels
                                   // block size 64, one tick per buffer
    libpd_process_float(1, inbuf, outbuf);
    ossia::state st;
    /*
    if(!m_tickFun.isCallable())
        return st;

    // 2. Get the value of the js fun
    auto messages = JS::convert::messages(m_tickFun.call({QJSValue{t}}));

    m_engine.collectGarbage();

    for(const auto& mess : messages)
    {
        st.add(Engine::iscore_to_ossia::message(mess, m_devices));
    }
    */

    // 3. Convert our value back
    if(unmuted())
      return st;
    return {};
}

ossia::state_element ProcessExecutor::offset(ossia::time_value off)
{
    return state(off / parent()->getDurationNominal());
}

Component::Component(
        ::Engine::Execution::ConstraintComponent& parentConstraint,
        Pd::ProcessModel& element,
        const ::Engine::Execution::Context& ctx,
        const Id<iscore::Component>& id,
        QObject* parent):
    ::Engine::Execution::ProcessComponent_T<Pd::ProcessModel, ProcessExecutor>{
          parentConstraint, element, ctx, id, "PdComponent", parent}
{
    m_ossia_process = std::make_shared<ProcessExecutor>(ctx.devices);
    OSSIAProcess().setTickFun(element.script());
}
}

