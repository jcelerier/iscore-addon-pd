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
#include <portaudio.h>
namespace Dataflow
{
Clock::Clock(
    const Engine::Execution::Context& ctx):
  ClockManager{ctx},
  m_default{ctx},
  m_plug{context.doc.plugin<Dataflow::DocumentPlugin>()}
{
  Pa_Initialize();
  auto bs = context.sys.baseScenario();
  if(!bs)
    return;

  ossia::time_constraint& ossia_cst = *bs->baseConstraint().OSSIAConstraint();

  ossia_cst.setDriveMode(ossia::clock::DriveMode::EXTERNAL);
  // Number of milliseconds in each step -> we tick once per buffer
  ossia_cst.setGranularity(ossia::time_value(1000. * 64. / 44100.) );
}

Clock::~Clock()
{
  for(auto& cbl : m_plug.cables)
    cbl.exec.reset();
  m_plug.currentExecutionContext = std::make_shared<ossia::graph>();
  Pa_Terminate();
}

int Clock::PortAudioCallback(
    const void *input, void *output,
    unsigned long frameCount,
    const PaStreamCallbackTimeInfo* timeInfo,
    PaStreamCallbackFlags statusFlags,
    void *userData)
{
  auto& clock = *static_cast<Clock*>(userData);

  const ossia::time_value rate{1000000. * 64. / 44100.};
  clock.m_cur->baseConstraint().OSSIAConstraint()->tick(rate);

  auto st = clock.m_plug.currentExecutionContext->state();

  auto float_output = ((float **) output);
  for(int i = 0; i < 2; i++)
  {
    for(int j = 0; j < frameCount; j++)
    {
      float_output[i][j] = 0;
    }
  }

  auto& ao = clock.m_plug.audio_outs;
  for(int i = 0; i < std::min((int)ao.size(), 2); i++)
  {
    auto it = st.localState.find(ao[i]);
    if(it != st.localState.end())
    {
      if(ossia::audio_port* audio = it->second.target<ossia::audio_port>())
      {
        for(const auto& vec : audio->samples)
        {
          for(int j = 0; j < std::min(frameCount, vec.size()); j++)
            float_output[i][j] += vec[j];
        }
      }
    }
  }


  return 0;

}

void Clock::play_impl(
    const TimeVal& t,
    Engine::Execution::BaseScenarioElement& bs)
{
  m_cur = &bs;

  m_default.play(t);

  PaStreamParameters outputParameters;

  outputParameters.device = Pa_GetDefaultOutputDevice();
  outputParameters.channelCount = 2;
  outputParameters.sampleFormat = paFloat32 | paNonInterleaved;
  outputParameters.suggestedLatency = 0.01;
  outputParameters.hostApiSpecificStreamInfo = 0;

  auto ec = Pa_OpenStream(&stream,
                            nullptr,
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
