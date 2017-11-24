#include "PdNode.hpp"
#define make_uuid(text) score::uuids::string_generator::compute((text))

namespace Process
{


void test()
{
  constexpr NodeBuilder<> n;
  constexpr auto res =
  n.audio_ins({{"foo"}, {"bar"}})
   .audio_outs({{"mimi"}});

  static_assert(get_ports<AudioInInfo>(res.build()).size() == 2);
  static_assert(get_ports<AudioOutInfo>(res.build()).size() == 1);
  static_assert(get_ports<AudioInInfo>(res.build())[0].name[0] == 'f');
  static_assert(get_ports<AudioInInfo>(res.build())[1].name[0] == 'b');
  static_assert(get_ports<AudioOutInfo>(res.build())[0].name[0] == 'm');
}
}

namespace Pd
{

struct SomeInfo
{
  static const constexpr auto prettyName = "My Funny Process";
  static const constexpr auto objectKey = "FunnyProcess";
  static const constexpr auto Process_uuid = make_uuid("f6b88ec9-cd56-43e8-a568-33208d5a8fb7");
  static const constexpr auto Executor_uuid = make_uuid("7f655416-17ce-4ddd-aaad-3bb7476f03ab");
  static const constexpr auto Inspector_uuid = make_uuid("74950a6c-8d82-4441-857d-14dac005cade");

  static const constexpr auto info =
    Process::create_node()
      .audio_ins({{"audio1"}, {"audio2"}})
      .midi_ins()
      .midi_outs({{"midi1"}})
      .value_ins()
      .value_outs()
      .controls({{"foo", Process::Slider, 1.0, 10., 5.}})
      .build();


  static void fun(
      const ossia::audio_port& p1,
      const ossia::audio_port& p2,
      const ossia::value_port& o1,
      ossia::midi_port& p,
      ossia::token_request tk,
      ossia::execution_state& st)
  {
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
