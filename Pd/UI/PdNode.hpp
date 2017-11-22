#pragma once
#include <Process/Process.hpp>
#include <Process/Dataflow/Port.hpp>

#include <Engine/Executor/ProcessComponent.hpp>

#include <Pd/Commands/PdCommandFactory.hpp>
#include <Process/LayerView.hpp>
#include <Process/LayerPresenter.hpp>
#include <Process/ProcessMetadata.hpp>
#include <Process/ProcessFactory.hpp>
#include <Process/Inspector/ProcessInspectorWidgetDelegate.hpp>
#include <Process/Inspector/ProcessInspectorWidgetDelegateFactory.hpp>

#include <Inspector/InspectorWidgetFactoryInterface.hpp>
#include <Inspector/InspectorWidgetBase.hpp>

#include <Explorer/DocumentPlugin/DeviceDocumentPlugin.hpp>
#include <Device/Protocol/DeviceList.hpp>
#include <score/model/path/PathSerialization.hpp>

#include <Process/GenericProcessFactory.hpp>
#include <Process/WidgetLayer/WidgetProcessFactory.hpp>

#include <Engine/score2OSSIA.hpp>
#include <score/widgets/DoubleSlider.hpp>
#include <score/widgets/MarginLess.hpp>
#include <score/widgets/TextLabel.hpp>
#include <ossia/dataflow/graph.hpp>
#include <ossia/dataflow/node_process.hpp>
#include <ossia/network/domain/domain.hpp>
#include <brigand/algorithms.hpp>
#include <QFormLayout>
#include <type_traits>

namespace Pd
{

class SetControlValue final : public score::Command
{
    SCORE_COMMAND_DECL(
        Pd::CommandFactoryName(), SetControlValue, "Set a control")

    public:

    SetControlValue(const Process::ControlInlet& obj, ossia::value newval)
        : m_path{obj}
        , m_old{obj.value()}
        , m_new{newval}
    {
    }

    virtual ~SetControlValue() { }

    void undo(const score::DocumentContext& ctx) const final override
    {
      m_path.find(ctx).setValue(m_old);
    }

    void redo(const score::DocumentContext& ctx) const final override
    {
      m_path.find(ctx).setValue(m_new);
    }

    void update(unused_t, ossia::value newval)
    {
      m_new = std::move(newval);
    }

  protected:
    void serializeImpl(DataStreamInput& stream) const final override
    {
      stream << m_path << m_old << m_new;
    }
    void deserializeImpl(DataStreamOutput& stream) final override
    {
      stream >> m_path >> m_old >> m_new;
    }

  private:
    Path<Process::ControlInlet> m_path;
    ossia::value m_old, m_new;
};

}

#define COMPONENT_INFO_METADATA(BaseType, CompoType)                 \
public:                                                              \
  static Q_DECL_RELAXED_CONSTEXPR score::Component::Key static_key() \
  {                                                                  \
    return Info::CompoType ## _uuid;                                 \
  }                                                                  \
                                                                     \
  score::Component::Key key() const final override                   \
  {                                                                  \
    return static_key();                                             \
  }                                                                  \
                                                                     \
  bool key_match(score::Component::Key other) const final override   \
  {                                                                  \
    return static_key() == other                                     \
           || BaseType::base_key_match(other);                       \
  }                                                                  \
                                                                     \
private:

namespace Process {

struct AudioInInfo {

  QLatin1String name;
  template<std::size_t N>
  constexpr AudioInInfo(const char (&name)[N]): name{name, N} { }
};
struct AudioOutInfo {

  QLatin1String name;
  template<std::size_t N>
  constexpr AudioOutInfo(const char (&name)[N]): name{name, N} { }
};
struct ValueInInfo {

  QLatin1String name;
  template<std::size_t N>
  constexpr ValueInInfo(const char (&name)[N]): name{name, N} { }
};
struct ValueOutInfo {

  QLatin1String name;
  template<std::size_t N>
  constexpr ValueOutInfo(const char (&name)[N]): name{name, N} { }
};
struct MidiInInfo {

  QLatin1String name;
  template<std::size_t N>
  constexpr MidiInInfo(const char (&name)[N]): name{name, N} { }
};
struct MidiOutInfo {

  QLatin1String name;
  template<std::size_t N>
  constexpr MidiOutInfo(const char (&name)[N]): name{name, N} { }
};

enum ControlType: int8_t { Slider };
struct ControlInfo {
  QLatin1String name;
  ControlType type;
  double min;
  double max;
  double init;

  template<std::size_t N, typename... Args>
  constexpr ControlInfo(const char (&name)[N], ControlType t, double min, double max, double init)://:
    name{name, N}
  , type{t}
  , min{min}
  , max{max}
  , init{init}
  {
  }
};

template<typename... Args>
struct NodeInfo: Args...
{
  using types = brigand::list<Args...>;
};

template<typename... Args>
static constexpr auto make_ninfo(Args&&... args)
{
  return NodeInfo<Args...>{std::forward<Args>(args)...};
}

template<typename... Args>
static constexpr auto audio_ins(Args&&... args)
{
  return std::array<AudioInInfo, sizeof...(Args)>{{std::forward<Args>(args)...}};
}
template<typename... Args>
static constexpr auto audio_outs(Args&&... args)
{
  return std::array<AudioOutInfo, sizeof...(Args)>{{std::forward<Args>(args)...}};
}

template<typename... Args>
static constexpr auto midi_ins(Args&&... args)
{
  return std::array<MidiInInfo, sizeof...(Args)>{{std::forward<Args>(args)...}};
}
template<typename... Args>
static constexpr auto midi_outs(Args&&... args)
{
  return std::array<MidiOutInfo, sizeof...(Args)>{{std::forward<Args>(args)...}};
}

template<typename... Args>
static constexpr auto value_ins(Args&&... args)
{
  return std::array<ValueInInfo, sizeof...(Args)>{{std::forward<Args>(args)...}};
}
template<typename... Args>
static constexpr auto value_outs(Args&&... args)
{
  return std::array<ValueOutInfo, sizeof...(Args)>{{std::forward<Args>(args)...}};
}

template<typename... Args>
static constexpr auto control_ins(Args&&... args)
{
  return std::array<ControlInfo, sizeof...(Args)>{{std::forward<Args>(args)...}};
}

template<typename T, typename...>
struct is_port : std::false_type {};
template<typename T, std::size_t N>
struct is_port<T, std::array<T, N>> : std::true_type {};

template<typename PortType, typename T>
static constexpr auto get_ports(const T& t)
{
  using index = brigand::index_if<typename T::types, is_port<PortType, brigand::_1>>;

  if constexpr(!std::is_same<index, brigand::no_such_type_>::value)
  {
    using array_type = brigand::at<typename T::types, index>;
    return static_cast<const array_type&>(t);
  }
  else
  {
    return std::array<PortType, 0>{};
  }
}

template<typename Info>
struct InfoFunctions
{

  static constexpr bool same(QLatin1String s1, QLatin1String s2)
  {
    if(s1.size() != s2.size())
      return false;
    for(int i = 0; i < s1.size(); i++)
    {
      if(s1[i] != s2[i])
        return false;
    }
    return true;
  }

  template<std::size_t N>
  static constexpr std::size_t inlet(const char (&n)[N])
  {
    QLatin1String name(n, N);
    std::size_t i = 0;
    for(const auto& in : get_ports<AudioInInfo>(Info::info))
    {
      if (same(in.name, name))
        return i;
      i++;
    }
    for(const auto& in : get_ports<MidiInInfo>(Info::info))
    {
      if (same(in.name, name))
        return i;
      i++;
    }
    for(const auto& in : get_ports<ValueInInfo>(Info::info))
    {
      if (same(in.name, name))
        return i;
      i++;
    }
    for(const auto& in : get_ports<ControlInfo>(Info::info))
    {
      if (same(in.name, name))
        return i;
      i++;
    }
    throw;
  }


  template<std::size_t N>
  static constexpr std::size_t outlet(const char (&n)[N])
  {
    QLatin1String name(n, N);
    std::size_t i = 0;
    for(const auto& in : get_ports<AudioOutInfo>(Info::info))
      if (same(in.name, name))
        return i;
      else
        i++;

    for(const auto& in : get_ports<MidiOutInfo>(Info::info))
      if (same(in.name, name))
        return i;
      else
        i++;

    for(const auto& in : get_ports<ValueOutInfo>(Info::info))
      if (same(in.name, name))
        return i;
      else
        i++;

    throw;
  }

  static constexpr auto control_start()
  {
    constexpr auto audio_in = get_ports<AudioInInfo>(Info::info);
    constexpr auto midi_in = get_ports<MidiInInfo>(Info::info);
    constexpr auto value_in = get_ports<ValueInInfo>(Info::info);
    constexpr auto control_in = get_ports<ControlInfo>(Info::info);
    constexpr auto audio_size = audio_in.size();
    constexpr auto midi_size = midi_in.size();
    constexpr auto value_size = value_in.size();

    return audio_size + midi_size + value_size;
  }

  static constexpr auto inlet_size()
  {
    constexpr auto audio_in = get_ports<AudioInInfo>(Info::info);
    constexpr auto midi_in = get_ports<MidiInInfo>(Info::info);
    constexpr auto value_in = get_ports<ValueInInfo>(Info::info);
    constexpr auto control_in = get_ports<ControlInfo>(Info::info);
    constexpr auto audio_size = audio_in.size();
    constexpr auto midi_size = midi_in.size();
    constexpr auto value_size = value_in.size();
    constexpr auto control_size = control_in.size();
    return audio_size + midi_size + value_size + control_size;
  }

  template<std::size_t N>
  static constexpr auto get_inlet_accessor()
  {
    constexpr auto audio_in = get_ports<AudioInInfo>(Info::info);
    constexpr auto midi_in = get_ports<MidiInInfo>(Info::info);
    constexpr auto value_in = get_ports<ValueInInfo>(Info::info);
    constexpr auto control_in = get_ports<ControlInfo>(Info::info);
    constexpr auto audio_size = audio_in.size();
    constexpr auto midi_size = midi_in.size();
    constexpr auto value_size = value_in.size();
    constexpr auto control_size = control_in.size();
    if constexpr(N < audio_size)
        return [] (const ossia::inlets& inl) { return *inl[N]->data.target<ossia::audio_port>(); };
    else if constexpr(N < (audio_size + midi_size))
        return [] (const ossia::inlets& inl) { return *inl[N]->data.target<ossia::midi_port>(); };
    else if constexpr(N < (audio_size + midi_size + value_size + control_size))
        return [] (const ossia::inlets& inl) { return *inl[N]->data.target<ossia::value_port>(); };
    else
        throw;
  }

  static constexpr auto outlet_size()
  {
    constexpr auto audio_out = get_ports<AudioOutInfo>(Info::info);
    constexpr auto midi_out = get_ports<MidiOutInfo>(Info::info);
    constexpr auto value_out = get_ports<ValueOutInfo>(Info::info);
    constexpr auto audio_size = audio_out.size();
    constexpr auto midi_size = midi_out.size();
    constexpr auto value_size = value_out.size();
    return audio_size + midi_size + value_size;
  }

  template<std::size_t N>
  static constexpr auto get_outlet_accessor()
  {
    constexpr auto audio_out = get_ports<AudioOutInfo>(Info::info);
    constexpr auto midi_out = get_ports<MidiOutInfo>(Info::info);
    constexpr auto value_out = get_ports<ValueOutInfo>(Info::info);
    constexpr auto audio_size = audio_out.size();
    constexpr auto midi_size = midi_out.size();
    constexpr auto value_size = value_out.size();
    if constexpr(N < audio_size)
        return [] (const ossia::outlets& outl) { return *outl[N]->data.target<ossia::audio_port>(); };
    else if constexpr(N < (audio_size + midi_size))
        return [] (const ossia::outlets& outl) { return *outl[N]->data.target<ossia::midi_port>(); };
    else if constexpr(N < (audio_size + midi_size + value_size))
        return [] (const ossia::outlets& outl) { return *outl[N]->data.target<ossia::value_port>(); };
    else
        throw;
  }

  template <class F, std::size_t... I>
  static constexpr void apply_inlet_impl(const F& f, const ossia::inlets& t, const std::index_sequence<I...>& )
  {
    f(get_inlet_accessor<I>()(t)...);
  }

  template <class F, std::size_t... I>
  static constexpr void apply_outlet_impl(const F& f, const ossia::outlets& t, const std::index_sequence<I...>& )
  {
    f(get_outlet_accessor<I>()(t)...);
  }

  template<typename F>
  static void run(
      const ossia::inlets& inlets,
      const ossia::outlets& outlets,
      const ossia::token_request& tk,
      ossia::execution_state& st,
      const F& f)
  {
    using inlets_indices = std::make_index_sequence<inlet_size()>;
    using outlets_indices = std::make_index_sequence<outlet_size()>;

    // from this, create tuples of functions
    // apply the functions to inlets and outlets
    apply_inlet_impl(
          [&] (auto&&... i) {
      apply_outlet_impl(
            [&] (auto&&... o) {
        return f(i..., o..., tk, st);
      }, outlets, outlets_indices{});
    }, inlets, inlets_indices{});
  }
};




template<typename Info>
class ControlProcess final: public Process::ProcessModel
{
    SCORE_SERIALIZE_FRIENDS
    PROCESS_METADATA_IMPL(ControlProcess<Info>)
  Process::Inlets m_inlets;
  Process::Outlets m_outlets;

  Process::Inlets inlets() const final override
  {
    return m_inlets;
  }

  Process::Outlets outlets() const final override
  {
    return m_outlets;
  }

  public:

  const Process::Inlets& inlets_ref() const { return m_inlets; }
  const Process::Outlets& outlets_ref() const { return m_outlets; }

  ossia::value control(int i)
  {
    static_assert(get_ports<ControlInfo>(Info::info).size() != 0);
    constexpr int start = InfoFunctions<Info>::control_start();

    return static_cast<ControlInlet*>(m_inlets[start + i])->value();
  }

  void setControl(int i, ossia::value v)
  {
    static_assert(get_ports<ControlInfo>(Info::info).size() != 0);
    constexpr int start = InfoFunctions<Info>::control_start();

    static_cast<ControlInlet*>(m_inlets[start + i])->setValue(std::move(v));
  }

  ControlProcess(
      const TimeVal& duration,
      const Id<Process::ProcessModel>& id,
      QObject* parent):
    Process::ProcessModel{duration, id, Metadata<ObjectKey_k, ProcessModel>::get(), parent}
  {
    metadata().setInstanceName(*this);

    int inlet = 0;
    for(const auto& in : get_ports<AudioInInfo>(Info::info))
    {
      auto p = new Process::Inlet(Id<Process::Port>(inlet++), this);
      p->type = Process::PortType::Audio;
      p->setCustomData(in.name);
      m_inlets.push_back(p);
    }
    for(const auto& in : get_ports<MidiInInfo>(Info::info))
    {
      auto p = new Process::Inlet(Id<Process::Port>(inlet++), this);
      p->type = Process::PortType::Midi;
      p->setCustomData(in.name);
      m_inlets.push_back(p);
    }
    for(const auto& in : get_ports<ValueInInfo>(Info::info))
    {
      auto p = new Process::Inlet(Id<Process::Port>(inlet++), this);
      p->type = Process::PortType::Midi;
      p->setCustomData(in.name);
      m_inlets.push_back(p);
    }
    for(const auto& in : get_ports<ControlInfo>(Info::info))
    {
      auto p = new Process::ControlInlet(Id<Process::Port>(inlet++), this);
      p->type = Process::PortType::Midi;
      p->setDomain(ossia::make_domain(in.min, in.max));
      p->setCustomData(in.name);
      m_inlets.push_back(p);
    }


    int outlet = 0;
    for(const auto& out : get_ports<AudioOutInfo>(Info::info))
    {
      auto p = new Process::Outlet(Id<Process::Port>(outlet++), this);
      p->type = Process::PortType::Audio;
      p->setCustomData(out.name);
      if(outlet == 0)
        p->setPropagate(true);
      m_outlets.push_back(p);
    }
    for(const auto& out : get_ports<MidiOutInfo>(Info::info))
    {
      auto p = new Process::Outlet(Id<Process::Port>(outlet++), this);
      p->type = Process::PortType::Midi;
      p->setCustomData(out.name);
      m_outlets.push_back(p);
    }
    for(const auto& out : get_ports<ValueOutInfo>(Info::info))
    {
      auto p = new Process::Outlet(Id<Process::Port>(outlet++), this);
      p->type = Process::PortType::Midi;
      p->setCustomData(out.name);
      m_outlets.push_back(p);
    }
  }

  ControlProcess(
      const ControlProcess& source,
      const Id<Process::ProcessModel>& id,
      QObject* parent):
    Process::ProcessModel{
      source,
      id,
      Metadata<ObjectKey_k, ProcessModel>::get(),
      parent}
  {
    // Process::clone_outlet(*source.outlet, this);
    metadata().setInstanceName(*this);

  }


  template<typename Impl>
  explicit ControlProcess(
      Impl& vis,
      QObject* parent) :
    Process::ProcessModel{vis, parent}
  {
    vis.writeTo(*this);
  }

  ~ControlProcess() override
  {

  }
};


////////// EXECUTION ////////////


template<typename Info>
class ControlNode : public ossia::graph_node
{
public:
  std::array<ossia::value, get_ports<ControlInfo>(Info::info).size()> controls;
  ControlNode()
  {
    for(const auto& in : get_ports<AudioInInfo>(Info::info))
    {
      m_inlets.push_back(ossia::make_inlet<ossia::audio_port>());
    }
    for(const auto& in : get_ports<MidiInInfo>(Info::info))
    {
      m_inlets.push_back(ossia::make_inlet<ossia::midi_port>());
    }
    for(const auto& in : get_ports<ValueInInfo>(Info::info))
    {
      m_inlets.push_back(ossia::make_inlet<ossia::value_port>());
    }
    for(const auto& in : get_ports<ControlInfo>(Info::info))
    {
      m_inlets.push_back(ossia::make_inlet<ossia::value_port>());
    }

    for(const auto& out : get_ports<AudioOutInfo>(Info::info))
    {
      m_outlets.push_back(ossia::make_outlet<ossia::audio_port>());
    }
    for(const auto& out : get_ports<MidiOutInfo>(Info::info))
    {
      m_outlets.push_back(ossia::make_outlet<ossia::midi_port>());
    }
    for(const auto& out : get_ports<ValueOutInfo>(Info::info))
    {
      m_outlets.push_back(ossia::make_outlet<ossia::value_port>());
    }
  }

  void run(ossia::token_request tk, ossia::execution_state& st) override
  {
    constexpr int start = InfoFunctions<Info>::control_start();
    for(int i = 0; i < get_ports<ControlInfo>(Info::info).size(); i++)
    {
      ossia::inlet& inlet = *m_inlets[start + i];
      auto& port = *inlet.data.target<ossia::value_port>();
      if(port.get_data().empty())
        port.add_value(controls[i]);
      else
        controls[i] = port.get_data().back().value;
    }
    InfoFunctions<Info>::run(this->inputs(), this->outputs(), tk, st, Info::fun);
  }
};

template<typename Info>
class Executor: public Engine::Execution::
    ProcessComponent_T<ControlProcess<Info>, ossia::node_process>
{
    COMPONENT_INFO_METADATA(Engine::Execution::ProcessComponent, Executor)
  public:
    Executor(
        ControlProcess<Info>& element,
        const ::Engine::Execution::Context& ctx,
        const Id<score::Component>& id,
        QObject* parent):
      Engine::Execution::ProcessComponent_T<ControlProcess<Info>, ossia::node_process>{
                            element,
                            ctx,
                            id, "Executor::ControlProcess<Info>", parent}
    {
      auto node = std::make_shared<ControlNode<Info>>();
      auto proc = std::make_shared<ossia::node_process>(node);
      this->m_node = node;
      this->m_ossia_process = proc;
      const auto& dl = ctx.devices.list();

      for(int i = 0; i < InfoFunctions<Info>::inlet_size(); i++)
      {
        auto dest = Engine::score_to_ossia::makeDestination(dl, element.inlets_ref()[i]->address());
        if(dest)
        {
          node->inputs()[i]->address = &dest->address();
        }
      }

      for(int i = 0; i < InfoFunctions<Info>::outlet_size(); i++)
      {
        auto dest = Engine::score_to_ossia::makeDestination(dl, element.outlets_ref()[i]->address());
        if(dest)
        {
          node->outputs()[i]->address = &dest->address();
        }
      }


      int i = 0;
      for(const ControlInfo& in : get_ports<ControlInfo>(Info::info))
      {
        constexpr int start = InfoFunctions<Info>::control_start();
        auto inlet = static_cast<ControlInlet*>(element.inlets_ref()[start + i]);

        QObject::connect(inlet, &ControlInlet::valueChanged,
                this, [=] (ossia::value val) {
          this->system().executionQueue.enqueue(
                [node,val,start,i]
          {
            node->inputs()[start + i]->data.template target<ossia::value_port>()->add_value(val);
          });
        });

        i++;
      }

      ctx.plugin.register_node(element, node);
    }


    ~Executor()
    {
      this->system().plugin.unregister_node(this->process(), m_node);
    }

  private:
    ossia::node_ptr m_node;
};





////////// INSPECTOR ////////////
template<typename Info>
class ControlCommand final : public score::Command
{
    using Command::Command;
    ControlCommand() = default;

    ControlCommand(const ControlProcess<Info>& obj, int control, ossia::value newval)
        : m_path{obj}
        , m_control{control}
        , m_old{obj.control(control)}
        , m_new{newval}
    {
    }

    virtual ~ControlCommand() { }

    void undo(const score::DocumentContext& ctx) const final override
    {
      m_path.find(ctx).setControl(m_control, m_old);
    }

    void redo(const score::DocumentContext& ctx) const final override
    {
      m_path.find(ctx).setControl(m_control, m_new);
    }

    void update(unused_t, ossia::value newval)
    {
      m_new = std::move(newval);
    }

  protected:
    void serializeImpl(DataStreamInput& stream) const final override
    {
      stream << m_path << m_control << m_old << m_new;
    }
    void deserializeImpl(DataStreamOutput& stream) final override
    {
      stream >> m_path >> m_control >> m_old >> m_new;
    }

  private:
    Path<ControlProcess<Info>> m_path;
    int m_control{};
    ossia::value m_old, m_new;

};


template<typename Info>
class InspectorWidget final
    : public Process::InspectorWidgetDelegate_T<ControlProcess<Info>>
{
public:
  explicit InspectorWidget(
      const ControlProcess<Info>& object,
      const score::DocumentContext& doc,
      QWidget* parent)
      : InspectorWidgetDelegate_T<ControlProcess<Info>>{object, parent}
      , m_dispatcher{doc.commandStack}
    {
      auto vlay = new QVBoxLayout{this};
      vlay->setSpacing(2);
      vlay->setMargin(2);
      vlay->setContentsMargins(0, 0, 0, 0);

      int i = 0;
      for(const ControlInfo& in : get_ports<ControlInfo>(Info::info))
      {
        constexpr int start = InfoFunctions<Info>::control_start();
        auto inlet = static_cast<ControlInlet*>(object.inlets_ref()[start + i]);

        auto lab = new TextLabel{in.name, this};
        auto sl = new score::DoubleSlider{this};
        vlay->addWidget(lab);
        vlay->addWidget(sl);

        QObject::connect(sl, &score::DoubleSlider::valueChanged,
                this, [=] (double val) {
          m_dispatcher.submitCommand<Pd::SetControlValue>(*inlet, in.min + val * (in.max - in.min));
        });

        QObject::connect(inlet, &ControlInlet::valueChanged,
                this, [=] (ossia::value val) {
          sl->setValue((ossia::convert<double>(val) - in.min) / (in.max - in.min));
        });

        i++;
      }

      vlay->addStretch();
    }

private:
  CommandDispatcher<> m_dispatcher;
};

template<typename Info>
class InspectorFactory final
    : public Process::InspectorWidgetDelegateFactory_T<ControlProcess<Info>, InspectorWidget<Info>>
{
  public:
    static Q_DECL_RELAXED_CONSTEXPR Process::InspectorWidgetDelegateFactory::ConcreteKey
    static_concreteKey() noexcept
    {
      return Info::Inspector_uuid;
    }

    Process::InspectorWidgetDelegateFactory::ConcreteKey concreteKey() const noexcept final override
    {
      return static_concreteKey();
    }
};

}




////////// METADATA ////////////

template <typename Info>
struct Metadata<PrettyName_k, Process::ControlProcess<Info>>
{
  static Q_DECL_RELAXED_CONSTEXPR auto get()
  {
    return Info::prettyName;
  }
};
template <typename Info>
struct Metadata<ObjectKey_k, Process::ControlProcess<Info>>
{
    static Q_DECL_RELAXED_CONSTEXPR auto get()
    {
      return Info::objectKey;
    }
};
template <typename Info>
struct Metadata<ConcreteKey_k, Process::ControlProcess<Info>>
{
    static Q_DECL_RELAXED_CONSTEXPR UuidKey<Process::ProcessModel> get()
    {
      return Info::Process_uuid;
    }
};

namespace score
{

template<typename Vis, typename Info>
void serialize_dyn_impl(Vis& v, const Process::ControlProcess<Info>& t)
{
}
}

template<typename Info>
struct is_custom_serialized<Process::ControlProcess<Info>>: std::true_type { };

template <typename T>
struct TSerializer<DataStream, Process::ControlProcess<T>>
{
  template<typename U>
  static void
  readFrom(DataStream::Serializer& s, const Process::ControlProcess<U>& obj)
  {
  }

  template<typename U>
  static void writeTo(DataStream::Deserializer& s, Process::ControlProcess<U>& obj)
  {
  }
};

template <typename T>
struct TSerializer<JSONObject, Process::ControlProcess<T>>
{
  template<typename U>
  static void
  readFrom(JSONObject::Serializer& s, const Process::ControlProcess<U>& obj)
  {
  }

  template<typename U>
  static void writeTo(JSONObject::Deserializer& s, Process::ControlProcess<U>& obj)
  {
  }
};
