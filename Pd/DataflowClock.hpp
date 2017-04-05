#pragma once
#include <Engine/Executor/ClockManager/ClockManagerFactory.hpp>
#include <Engine/Executor/ClockManager/DefaultClockManager.hpp>

#include <portaudio.h>
namespace Dataflow
{
class DocumentPlugin;
class Clock final :
        public Engine::Execution::ClockManager
{
    public:
        Clock(const Engine::Execution::Context& ctx);

        ~Clock();
private:
        // Clock interface
        void play_impl(
                const TimeVal& t,
                Engine::Execution::BaseScenarioElement&) override;
        void pause_impl(Engine::Execution::BaseScenarioElement&) override;
        void resume_impl(Engine::Execution::BaseScenarioElement&) override;
        void stop_impl(Engine::Execution::BaseScenarioElement&) override;

        Engine::Execution::DefaultClockManager m_default;
        Dataflow::DocumentPlugin& m_plug;
        Engine::Execution::BaseScenarioElement* m_cur{};
        PaStream* stream{};
        static int PortAudioCallback(
            const void* input,
            void* output,
            unsigned long frameCount,
            const PaStreamCallbackTimeInfo* timeInfo,
            PaStreamCallbackFlags statusFlags,
            void* userData);
};

class ClockFactory final : public Engine::Execution::ClockManagerFactory
{
        ISCORE_CONCRETE("e9ae6dec-a10f-414f-9060-b21d15b5d58d")

        QString prettyName() const override;
        std::unique_ptr<Engine::Execution::ClockManager> make(
            const Engine::Execution::Context& ctx) override;
};
}
