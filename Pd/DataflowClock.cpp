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

int Clock::PortAudioCallback(
    const void *input, void *output,
    unsigned long frameCount,
    const PaStreamCallbackTimeInfo* timeInfo,
    PaStreamCallbackFlags statusFlags,
    void *userData)
{
  using idx_t = gsl::span<float>::index_type;
  const idx_t fc = frameCount;
  auto& clock = *static_cast<Clock*>(userData);

  auto float_input = ((float **) input);
  auto float_output = ((float **) output);

  // Prepare audio inputs
  const int n_in_channels = clock.m_plug.audio_ins.size();
  for(int i = 0; i < n_in_channels; i++)
  {
    clock.m_plug.audio_ins[i]->audio = {float_input[i], fc};
  }

  // Prepare audio outputs
  const int n_out_channels = clock.m_plug.audio_outs.size();
  for(int i = 0; i < n_out_channels; i++)
  {
    clock.m_plug.audio_outs[i]->audio = {float_output[i], fc};

    for(int j = 0; j < frameCount; j++)
    {
      float_output[i][j] = 0;
   }
  }

  // Run a tick
  const ossia::time_value rate{1000000. * 64. / 44100.};
  clock.m_plug.execState.clear();

  clock.m_cur->baseConstraint().OSSIAConstraint()->tick(rate);
  clock.m_plug.execGraph->state(clock.m_plug.execState);
  clock.m_plug.execState.commit();
  return 0;
}

void Clock::play_impl(
    const TimeVal& t,
    Engine::Execution::BaseScenarioElement& bs)
{
  std::stringstream s;
  boost::write_graphviz(s, m_plug.execGraph->m_graph, [&] (auto& out, const auto& v) {
      out << "[label=\"" << (void*)m_plug.execGraph->m_graph[v].get() << "\"]";
  },
  [] (auto&&...) {});

  std::cerr << s.str() << std::endl;
  m_cur = &bs;

  m_default.play(t);

  PaStreamParameters inputParameters;
  inputParameters.device = Pa_GetDefaultInputDevice();
  inputParameters.channelCount = m_plug.audio_ins.size();
  inputParameters.sampleFormat = paFloat32 | paNonInterleaved;
  inputParameters.suggestedLatency = 0.01;
  inputParameters.hostApiSpecificStreamInfo = nullptr;

  PaStreamParameters outputParameters;
  outputParameters.device = Pa_GetDefaultOutputDevice();
  outputParameters.channelCount = m_plug.audio_outs.size();
  outputParameters.sampleFormat = paFloat32 | paNonInterleaved;
  outputParameters.suggestedLatency = 0.01;
  outputParameters.hostApiSpecificStreamInfo = nullptr;

  auto ec = Pa_OpenStream(&stream,
                          &inputParameters,
                          &outputParameters,
                          44100,
                          64,
                          paNoFlag,
                          &PortAudioCallback,
                          this);
  Pa_StartStream( stream );
}

void Clock::pause_impl(
    Engine::Execution::BaseScenarioElement& bs)
{
  Pa_StopStream( stream );
  m_default.pause();
}

void Clock::resume_impl(
    Engine::Execution::BaseScenarioElement& bs)
{
  Pa_StartStream( stream );
  m_default.resume();
}

void Clock::stop_impl(
    Engine::Execution::BaseScenarioElement& bs)
{
  Pa_StopStream( stream );
  m_default.stop();
}

std::unique_ptr<Engine::Execution::ClockManager> ClockFactory::make(
    const Engine::Execution::Context& ctx)
{
  return std::make_unique<Clock>(ctx);
}

QString ClockFactory::prettyName() const
{
  return QObject::tr("Dataflow");
}

}
