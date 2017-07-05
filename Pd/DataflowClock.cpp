#include "DataflowClock.hpp"
#include <Engine/Executor/ExecutorContext.hpp>
#include <Engine/Executor/DocumentPlugin.hpp>
#include <Engine/Executor/BaseScenarioComponent.hpp>
#include <Engine/Executor/ConstraintComponent.hpp>
#include <Engine/Executor/Settings/ExecutorModel.hpp>
#include <Engine/Executor/BaseScenarioComponent.hpp>
#include <Engine/Executor/ConstraintComponent.hpp>
#include <Pd/DocumentPlugin.hpp>
#include <Pd/Executor/PdExecutor.hpp>
#include <boost/graph/graphviz.hpp>
#include <portaudio.h>
#include <ossia/dataflow/audio_address.hpp>
namespace Dataflow
{
Clock::Clock(
    const Engine::Execution::Context& ctx):
  ClockManager{ctx},
  m_default{ctx},
  m_plug{context.doc.plugin<Dataflow::DocumentPlugin>()}
{
  Pa_Initialize();
  auto& bs = context.scenario;
  if(!bs.active())
    return;
}

Clock::~Clock()
{
  for(Process::Cable& cbl : m_plug.cables)
  {
    cbl.source_node.reset();
    cbl.sink_node.reset();
    cbl.exec.reset();
  }
  m_plug.execGraph = std::make_shared<ossia::graph>();
  Pa_Terminate();
}

void Clock::play_impl(
    const TimeVal& t,
    Engine::Execution::BaseScenarioElement& bs)
{
  m_paused = false;

  std::stringstream s;
  boost::write_graphviz(s, m_plug.execGraph->m_graph, [&] (auto& out, const auto& v) {
      out << "[label=\"" << (void*)m_plug.execGraph->m_graph[v].get() << "\"]";
  },
  [] (auto&&...) {});

  std::cerr << s.str() << std::endl;
  m_cur = &bs;

  m_default.play(t);

  m_plug.audioProto().ui_tick = [this] (unsigned long frameCount) {
    m_plug.execState.clear();
    m_cur->baseConstraint().OSSIAConstraint()->tick(ossia::time_value(frameCount));
    m_plug.execGraph->state(m_plug.execState);
    m_plug.execState.commit();
  };

  m_plug.audioProto().replace_tick = true;
}

void Clock::pause_impl(
    Engine::Execution::BaseScenarioElement& bs)
{
  m_paused = true;
  m_plug.audioProto().ui_tick = {};
  m_plug.audioProto().replace_tick = true;
  qDebug("pause");
  m_default.pause();
}

void Clock::resume_impl(
    Engine::Execution::BaseScenarioElement& bs)
{
  m_paused = false;
  m_default.resume();
  m_plug.audioProto().ui_tick = [this] (unsigned long frameCount) {
    m_plug.execState.clear();
    m_cur->baseConstraint().OSSIAConstraint()->tick(ossia::time_value(frameCount));
    m_plug.execGraph->state(m_plug.execState);
    m_plug.execState.commit();
  };

  m_plug.audioProto().replace_tick = true;
  qDebug("resume");
}

void Clock::stop_impl(
    Engine::Execution::BaseScenarioElement& bs)
{
  m_paused = false;
  m_plug.audioProto().ui_tick = {};
  m_plug.audioProto().replace_tick = true;
  m_default.stop();
}

bool Clock::paused() const
{
  return m_paused;
}

std::unique_ptr<Engine::Execution::ClockManager> ClockFactory::make(
    const Engine::Execution::Context& ctx)
{
  return std::make_unique<Clock>(ctx);
}

std::function<ossia::time_value (const TimeVal&)>
ClockFactory::makeTimeFunction(const iscore::DocumentContext& ctx) const
{
  auto rate = ctx.plugin<Dataflow::DocumentPlugin>().audioProto().rate;
  return [=] (const TimeVal& v) -> ossia::time_value {
    // Go from milliseconds to samples
    // 1000 ms = sr samples
    // x ms    = k samples
    return v.isInfinite()
        ? ossia::Infinite
        : ossia::time_value(std::llround(rate * v.msec() / 1000.));
  };
}

std::function<TimeVal(const ossia::time_value&)>
ClockFactory::makeReverseTimeFunction(const iscore::DocumentContext& ctx) const
{
  auto rate = ctx.plugin<Dataflow::DocumentPlugin>().audioProto().rate;
  return [=] (const ossia::time_value& v) -> TimeVal {
    return v.infinite()
        ? TimeVal{PositiveInfinity{}}
        : TimeVal::fromMsecs(1000. * v.impl / rate);
  };
}

QString ClockFactory::prettyName() const
{
  return QObject::tr("Dataflow");
}

}
