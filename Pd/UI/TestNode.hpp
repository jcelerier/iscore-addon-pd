#include "PdNode.hpp"
#define make_uuid(text) score::uuids::string_generator::compute((text))
namespace Pd
{

struct SomeInfo
{
  static const constexpr auto prettyName = "My Funny Process";
  static const constexpr auto objectKey = "FunnyProcess";
  static const constexpr auto Process_uuid = make_uuid("f6b88ec9-cd56-43e8-a568-33208d5a8fb7");
  static const constexpr auto Executor_uuid = make_uuid("7f655416-17ce-4ddd-aaad-3bb7476f03ab");
  static const constexpr auto Inspector_uuid = make_uuid("74950a6c-8d82-4441-857d-14dac005cade");

  static const constexpr auto info = [] {
    using namespace Process;
    return make_ninfo(
        audio_ins(AudioInInfo{"audio1"}, AudioInInfo{"audio2"}),
        midi_ins(),
        midi_outs(MidiOutInfo{"midi1"}),
        value_ins(),
        value_outs(),
        control_ins(ControlInfo{"foo", Slider, 1.0, 10., 5.})
        );
  }();


  static void fun(
      const ossia::audio_port& p1, const ossia::audio_port& p2, const ossia::value_port& o1,
      ossia::midi_port& p,
      ossia::token_request tk,
      ossia::execution_state& st)
  {
    std::cerr << "coucou " << o1.get_data().back().value << std::endl;
  }
};

struct evurithin
{
    using process = Process::ControlProcess<SomeInfo>;
    using process_factory = Process::GenericProcessModelFactory<process>;

    using executor = Process::Executor<SomeInfo>;
    using executor_factory = Engine::Execution::ProcessComponentFactory_T<executor>;

    using inspector = Process::InspectorWidget<SomeInfo>;
    using inspector_factory = Process::InspectorFactory<SomeInfo>;

    using layer_factory = WidgetLayer::LayerFactory<process, inspector>;
};


}
