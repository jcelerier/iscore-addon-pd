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
#include <score/widgets/SignalUtils.hpp>
#include <score/widgets/MarginLess.hpp>
#include <score/widgets/TextLabel.hpp>
#include <ossia/dataflow/graph.hpp>
#include <ossia/dataflow/node_process.hpp>
#include <ossia/network/domain/domain.hpp>
#include <brigand/algorithms.hpp>
#include <frozen/set.h>
#include <QCheckBox>
#include <QFormLayout>
#include <QLineEdit>
#include <type_traits>
#include <variant>
#define make_uuid(text) score::uuids::string_generator::compute((text))

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


template<typename T>
using timed_vec = std::vector<std::pair<ossia::time_value, T>>;

struct AudioInInfo {
  const QLatin1String name;

  template<std::size_t N>
  constexpr AudioInInfo(const char (&name)[N]): name{name, N} { }
};
struct AudioOutInfo {
  const QLatin1String name;

  template<std::size_t N>
  constexpr AudioOutInfo(const char (&name)[N]): name{name, N} { }
};
struct ValueInInfo {
  const QLatin1String name;

  template<std::size_t N>
  constexpr ValueInInfo(const char (&name)[N]): name{name, N} { }
};
struct ValueOutInfo {
  const QLatin1String name;

  template<std::size_t N>
  constexpr ValueOutInfo(const char (&name)[N]): name{name, N} { }
};
struct MidiInInfo {
  const QLatin1String name;

  template<std::size_t N>
  constexpr MidiInInfo(const char (&name)[N]): name{name, N} { }
};
struct MidiOutInfo {
  const QLatin1String name;

  template<std::size_t N>
  constexpr MidiOutInfo(const char (&name)[N]): name{name, N} { }
};


struct ControlInfo {
  const QLatin1String name;

  template<std::size_t N, typename... Args>
  constexpr ControlInfo(const char (&name)[N]):
    name{name, N}
  {
  }
};

struct FloatSlider : ControlInfo
{
  using type = float;
  const float min{};
  const float max{};
  const float init{};

  auto create_inlet(Id<Process::Port> id, QObject* parent) const
  {
    auto p = new Process::ControlInlet(id, parent);
    p->type = Process::PortType::Message;
    p->setValue(init);
    p->setDomain(ossia::make_domain(min, max));
    p->setCustomData(name);
    return p;
  }

  auto make_widget(ControlInlet& inlet, CommandDispatcher<>& m_dispatcher, QWidget* parent) const
  {
    auto sl = new score::DoubleSlider{parent};
    sl->setOrientation(Qt::Horizontal);

    QObject::connect(sl, &score::DoubleSlider::valueChanged,
            parent, [*this,&inlet,&m_dispatcher] (double val) {
      m_dispatcher.submitCommand<Pd::SetControlValue>(inlet, min + val * (max - min));
    });

    QObject::connect(&inlet, &ControlInlet::valueChanged,
            parent, [*this,sl] (ossia::value val) {
      sl->setValue((ossia::convert<double>(val) - min) / (max - min));
    });

    return sl;
  }
};
struct IntSlider: ControlInfo
{
  using type = int;
  const int min{};
  const int max{};
  const int init{};

  auto create_inlet(Id<Process::Port> id, QObject* parent) const
  {
    auto p = new Process::ControlInlet(id, parent);
    p->type = Process::PortType::Message;
    p->setValue(init);
    p->setDomain(ossia::make_domain(min, max));
    p->setCustomData(name);
    return p;
  }

  auto make_widget(ControlInlet& inlet, CommandDispatcher<>& m_dispatcher, QWidget* parent) const
  {
    auto sl = new QSlider{parent};
    sl->setOrientation(Qt::Horizontal);
    sl->setRange(min, max);
    sl->setValue(init);

    QObject::connect(sl, &QSlider::valueChanged,
            parent, [&inlet,&m_dispatcher] (int val) {
      m_dispatcher.submitCommand<Pd::SetControlValue>(inlet, val);
    });

    QObject::connect(&inlet, &ControlInlet::valueChanged,
            parent, [sl] (ossia::value val) {
      sl->setValue(ossia::convert<int>(val));
    });

    return sl;
  }
};
struct IntSpinBox: ControlInfo
{
  using type = int;
  const int min{};
  const int max{};
  const int init{};

  auto create_inlet(Id<Process::Port> id, QObject* parent) const
  {
    auto p = new Process::ControlInlet(id, parent);
    p->type = Process::PortType::Message;
    p->setValue(init);
    p->setDomain(ossia::make_domain(min, max));
    p->setCustomData(name);
    return p;
  }

  auto make_widget(ControlInlet& inlet, CommandDispatcher<>& m_dispatcher, QWidget* parent) const
  {
    auto sl = new QSpinBox{parent};
    sl->setRange(min, max);
    sl->setValue(init);

    QObject::connect(sl, SignalUtils::QSpinBox_valueChanged_int(),
            parent, [&inlet,&m_dispatcher] (int val) {
      m_dispatcher.submitCommand<Pd::SetControlValue>(inlet, val);
    });

    QObject::connect(&inlet, &ControlInlet::valueChanged,
            parent, [sl] (ossia::value val) {
      sl->setValue(ossia::convert<int>(val));
    });

    return sl;
  }
};
struct Toggle: ControlInfo
{
  using type = bool;
  const bool init{};
  auto create_inlet(Id<Process::Port> id, QObject* parent) const
  {
    auto p = new Process::ControlInlet(id, parent);
    p->type = Process::PortType::Message;
    p->setValue(init);
    p->setCustomData(name);
    return p;
  }

  auto make_widget(ControlInlet& inlet, CommandDispatcher<>& m_dispatcher, QWidget* parent) const
  {
    auto sl = new QCheckBox{parent};
    sl->setChecked(init);

    QObject::connect(sl, &QCheckBox::toggled,
            parent, [&inlet,&m_dispatcher] (bool val) {
      m_dispatcher.submitCommand<Pd::SetControlValue>(inlet, val);
    });

    QObject::connect(&inlet, &ControlInlet::valueChanged,
            parent, [sl] (ossia::value val) {
      sl->setChecked(ossia::convert<bool>(val));
    });

    return sl;
  }
};
struct LineEdit: ControlInfo
{
  using type = std::string;
  const QLatin1Literal init{};
  auto create_inlet(Id<Process::Port> id, QObject* parent) const
  {
    auto p = new Process::ControlInlet(id, parent);
    p->type = Process::PortType::Message;
    p->setValue(std::string(init.latin1(), init.size()));
    p->setCustomData(name);
    return p;
  }

  auto make_widget(ControlInlet& inlet, CommandDispatcher<>& m_dispatcher, QWidget* parent) const
  {
    auto sl = new QLineEdit{parent};
    sl->setText(init);

    QObject::connect(sl, &QLineEdit::editingFinished,
            parent, [sl,&inlet,&m_dispatcher] () {
      m_dispatcher.submitCommand<Pd::SetControlValue>(inlet, sl->text().toStdString());
    });

    QObject::connect(&inlet, &ControlInlet::valueChanged,
            parent, [sl] (ossia::value val) {
      sl->setText(QString::fromStdString(ossia::convert<std::string>(val)));
    });

    return sl;
  }
};
struct RGBAEdit: ControlInfo
{
  using type = std::array<float, 4>;
  std::array<float, 4> init{};
};
struct XYZEdit: ControlInfo
{
  using type = std::array<float, 3>;
  std::array<float, 3> init{};
};
template<typename T, std::size_t N>
struct ComboBox: ControlInfo
{
  using type = T;
  const std::size_t init{};
  const std::array<std::pair<QLatin1String, T>, N> values;

  auto create_inlet(Id<Process::Port> id, QObject* parent) const
  {
    auto p = new Process::ControlInlet(id, parent);
    p->type = Process::PortType::Message;
    p->setValue(values[init].second);
    p->setCustomData(name);
    return p;
  }

  auto make_widget(ControlInlet& inlet, CommandDispatcher<>& m_dispatcher, QWidget* parent) const
  {
    auto sl = new QComboBox{parent};
    for(auto& e : values)
    {
      sl->addItem(e.first);
    }
    sl->setCurrentIndex(init);

    QObject::connect(sl, SignalUtils::QComboBox_currentIndexChanged_int(),
            parent, [*this,&inlet,&m_dispatcher] (int idx) {
      m_dispatcher.submitCommand<Pd::SetControlValue>(inlet, values[idx].second);
    });

    QObject::connect(&inlet, &ControlInlet::valueChanged,
            parent, [*this,sl] (ossia::value val) {
      auto v = ossia::convert<T>(val);
      auto idx = ossia::find_if(values, [&] (const auto& pair) { return pair.second == v; });
      if(idx != values.end())
      {
        sl->setCurrentIndex(std::distance(values.begin(), idx));
      }
    });

    return sl;
  }
};

template<std::size_t N>
struct Enum: ControlInfo
{
  using type = std::string;
  const std::size_t init{};
  const std::array<const char*, N> values;

  auto create_inlet(Id<Process::Port> id, QObject* parent) const
  {
    auto p = new Process::ControlInlet(id, parent);
    p->type = Process::PortType::Message;
    p->setValue(std::string(values[init]));
    p->setCustomData(name);
    return p;
  }

  auto make_widget(ControlInlet& inlet, CommandDispatcher<>& m_dispatcher, QWidget* parent) const
  {
    auto sl = new QComboBox{parent};
    for(const auto& e : values)
    {
      sl->addItem(e);
    }
    sl->setCurrentIndex(init);

    QObject::connect(sl, SignalUtils::QComboBox_currentIndexChanged_int(),
            parent, [*this,&inlet,&m_dispatcher] (int idx) {
      m_dispatcher.submitCommand<Pd::SetControlValue>(inlet, values[idx]);
    });

    QObject::connect(&inlet, &ControlInlet::valueChanged,
            parent, [*this,sl] (ossia::value val) {
      auto v = QString::fromStdString(ossia::convert<std::string>(val));
      auto idx = ossia::find(values, v);
      if(idx != values.end())
      {
        sl->setCurrentIndex(std::distance(values.begin(), idx));
      }
    });

    return sl;
  }
};

template<typename... Args>
struct NodeInfo: Args...
{
  using types = brigand::list<Args...>;
};

template<typename... Args>
static constexpr auto make_node(Args&&... args)
{
  return NodeInfo<Args...>{std::forward<Args>(args)...};
}

template<std::size_t N>
using AudioIns = std::array<AudioInInfo, N>;
template<std::size_t N>
using AudioOuts = std::array<AudioOutInfo, N>;
template<std::size_t N>
using MidiIns = std::array<MidiInInfo, N>;
template<std::size_t N>
using MidiOuts = std::array<MidiOutInfo, N>;
template<std::size_t N>
using ValueIns = std::array<ValueInInfo, N>;
template<std::size_t N>
using ValueOuts = std::array<ValueOutInfo, N>;
template<std::size_t N>
using Controls = std::array<ControlInfo, N>;

namespace detail {
template <class T, std::size_t N, std::size_t... I>
constexpr std::array<std::remove_cv_t<T>, N>
    to_array_impl(T (&a)[N], std::index_sequence<I...>)
{
    return { {a[I]...} };
}
}

template <class T, std::size_t N>
constexpr std::array<std::remove_cv_t<T>, N> to_array(T (&a)[N])
{
    return detail::to_array_impl(a, std::make_index_sequence<N>{});
}


template<typename... Args>
struct NodeBuilder: Args...
{
  constexpr auto audio_ins() const { return *this; }
  constexpr auto audio_outs() const { return *this; }
  constexpr auto midi_ins() const { return *this; }
  constexpr auto midi_outs() const { return *this; }
  constexpr auto value_ins() const { return *this; }
  constexpr auto value_outs() const { return *this; }
  constexpr auto controls() const { return *this; }

  template<std::size_t N>
  constexpr auto audio_ins(const AudioInInfo (&arg)[N]) const {
    return NodeBuilder<std::array<AudioInInfo, N>, Args...>{to_array(arg), static_cast<Args>(*this)...};
  }
  template<std::size_t N>
  constexpr auto audio_outs(const AudioOutInfo (&arg)[N]) const {
    return NodeBuilder<std::array<AudioOutInfo, N>, Args...>{to_array(arg), static_cast<Args>(*this)...};
  }

  template<std::size_t N>
  constexpr auto midi_ins(const MidiInInfo (&arg)[N]) const {
    return NodeBuilder<std::array<MidiInInfo, N>, Args...>{to_array(arg), static_cast<Args>(*this)...};
  }
  template<std::size_t N>
  constexpr auto midi_outs(const MidiOutInfo (&arg)[N]) const {
    return NodeBuilder<std::array<MidiOutInfo, N>, Args...>{to_array(arg), static_cast<Args>(*this)...};
  }

  template<std::size_t N>
  constexpr auto value_ins(const ValueInInfo (&arg)[N]) const {
    return NodeBuilder<std::array<ValueInInfo, N>, Args...>{to_array(arg), static_cast<Args>(*this)...};
  }
  template<std::size_t N>
  constexpr auto value_outs(const ValueOutInfo (&arg)[N]) const {
    return NodeBuilder<std::array<ValueOutInfo, N>, Args...>{to_array(arg), static_cast<Args>(*this)...};
  }
  template<typename... Controls>
  constexpr auto controls(const Controls&&... ctrls) const {
    return NodeBuilder<std::tuple<Controls...>, Args...>{
        std::make_tuple(ctrls...), static_cast<Args>(*this)...
    };
  }
  constexpr auto build() const {
    return NodeInfo<Args...>{static_cast<Args>(*this)...};
  }
};

constexpr auto create_node() {
  return NodeBuilder<>{};
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

template<typename T, typename...>
struct is_controls : std::false_type {};
template<typename T, typename... Args>
struct is_controls <T, std::tuple<Args...>> : std::true_type {};

template<typename T>
static constexpr auto get_controls(const T& t)
{
  using index = brigand::index_if<typename T::types, is_controls<PortType, brigand::_1>>;

  if constexpr(!std::is_same<index, brigand::no_such_type_>::value)
  {
    using tuple_type = brigand::at<typename T::types, index>;
    return static_cast<const tuple_type&>(t);
  }
  else
  {
    return std::tuple<>{};
  }

}
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
template<typename Info>
struct InfoFunctions
{



  /*
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
  */

  static constexpr auto audio_in_count =
      get_ports<AudioInInfo>(Info::info).size();
  static constexpr auto audio_out_count =
      get_ports<AudioOutInfo>(Info::info).size();
  static constexpr auto midi_in_count =
      get_ports<MidiInInfo>(Info::info).size();
  static constexpr auto midi_out_count =
      get_ports<MidiOutInfo>(Info::info).size();
  static constexpr auto value_in_count =
      get_ports<ValueInInfo>(Info::info).size();
  static constexpr auto value_out_count =
      get_ports<ValueOutInfo>(Info::info).size();
  static constexpr auto control_count =
      std::tuple_size<decltype(get_controls(Info::info))>::value;


  static constexpr auto control_start =
      audio_in_count + midi_in_count + value_in_count;

  static constexpr auto inlet_size =
      audio_in_count + midi_in_count + value_in_count + control_count;

  static constexpr auto outlet_size =
      audio_out_count + midi_out_count + value_out_count;

  template<std::size_t N>
  static constexpr auto get_inlet_accessor()
  {
    if constexpr(N < audio_in_count)
        return [] (const ossia::inlets& inl) -> const ossia::audio_port& { return *inl[N]->data.target<ossia::audio_port>(); };
    else if constexpr(N < (audio_in_count + midi_in_count))
        return [] (const ossia::inlets& inl) -> const ossia::midi_port& { return *inl[N]->data.target<ossia::midi_port>(); };
    else if constexpr(N < (audio_in_count + midi_in_count + value_in_count + control_count))
        return [] (const ossia::inlets& inl) -> const ossia::value_port& { return *inl[N]->data.target<ossia::value_port>(); };
    else
        throw;
  }
  template<std::size_t N>
  static constexpr auto get_control_accessor()
  {
    constexpr const auto idx = control_start + N;
    static_assert(control_count > 0);
    static_assert(N < control_count);

    return [=] (const ossia::inlets& inl) {
      constexpr const auto controls = get_controls(Info::info);
      constexpr const auto control = std::get<N>(controls);
      using val_type = typename decltype(control)::type;

      timed_vec<val_type> vec;
      const auto& vp = inl[idx]->data.template target<ossia::value_port>()->get_data();
      vec.reserve(vp.size());
      for(auto& v : vp)
      {
        vec.emplace_back(v.timestamp, ossia::convert<val_type>(v.value));
      }
      return vec;
    };
  }

  template<std::size_t N>
  static constexpr auto get_outlet_accessor()
  {
    if constexpr(N < audio_out_count)
        return [] (const ossia::outlets& inl) -> ossia::audio_port& { return *inl[N]->data.target<ossia::audio_port>(); };
    else if constexpr(N < (audio_out_count + midi_out_count))
        return [] (const ossia::outlets& inl) -> ossia::midi_port& { return *inl[N]->data.target<ossia::midi_port>(); };
    else if constexpr(N < (audio_out_count + midi_out_count + value_out_count))
        return [] (const ossia::outlets& inl) -> ossia::value_port& { return *inl[N]->data.target<ossia::value_port>(); };
    else
        throw;
  }

  template <class F, std::size_t... I>
  static constexpr void apply_inlet_impl(const F& f, const std::index_sequence<I...>& )
  {
    f(get_inlet_accessor<I>()...);
  }

  template <class F, std::size_t... I>
  static constexpr void apply_outlet_impl(const F& f, const std::index_sequence<I...>& )
  {
    f(get_outlet_accessor<I>()...);
  }

  template <class F, std::size_t... I>
  static constexpr void apply_control_impl(const F& f, const std::index_sequence<I...>& )
  {
    f(get_control_accessor<I>()...);
  }
  template<typename F>
  static void run(
      const ossia::inlets& inlets,
      const ossia::outlets& outlets,
      const ossia::token_request& tk,
      ossia::execution_state& st,
      const F& f)
  {
    using inlets_indices = std::make_index_sequence<audio_in_count + midi_in_count + value_in_count>;
    using controls_indices = std::make_index_sequence<control_count>;
    using outlets_indices = std::make_index_sequence<outlet_size>;

    if constexpr(control_count > 0) {
      auto control_span = gsl::span(&inlets[control_start], control_count);
      // from this, create tuples of functions
      // apply the functions to inlets and outlets
      apply_inlet_impl(
            [&] (auto&&... i) {
        apply_control_impl(
              [&] (auto&&... c) {
          apply_outlet_impl(
                [&] (auto&&... o) {
            f(i(inlets)..., c(inlets)..., o(outlets)..., tk, st);
          }, outlets_indices{});
        }, controls_indices{});
      }, inlets_indices{});
    }
    else
    {
      apply_inlet_impl(
            [&] (auto&&... i) {
        apply_outlet_impl(
              [&] (auto&&... o) {
          f(i(inlets)..., o(outlets)..., tk, st);
        }, outlets_indices{});
      }, inlets_indices{});

    }
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

  ossia::value control(std::size_t i)
  {
    static_assert(InfoFunctions<Info>::control_count != 0);
    constexpr auto start = InfoFunctions<Info>::control_start;

    return static_cast<ControlInlet*>(m_inlets[start + i])->value();
  }

  void setControl(std::size_t i, ossia::value v)
  {
    static_assert(InfoFunctions<Info>::control_count != 0);
    constexpr auto start = InfoFunctions<Info>::control_start;

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
      p->type = Process::PortType::Message;
      p->setCustomData(in.name);
      m_inlets.push_back(p);
    }
    ossia::for_each_in_tuple(get_controls(Info::info),
                             [&] (const auto& ctrl) {
      if(auto p = ctrl.create_inlet(Id<Process::Port>(inlet++), this))
      {
        m_inlets.push_back(p);
      }
    });

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
      p->type = Process::PortType::Message;
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
  std::array<ossia::value, InfoFunctions<Info>::control_count> controls;
  ControlNode()
  {
    m_inlets.reserve(InfoFunctions<Info>::inlet_size);
    m_outlets.reserve(InfoFunctions<Info>::outlet_size);
    for(std::size_t i = 0; i < InfoFunctions<Info>::audio_in_count; i++)
    {
      m_inlets.push_back(ossia::make_inlet<ossia::audio_port>());
    }
    for(std::size_t i = 0; i < InfoFunctions<Info>::midi_in_count; i++)
    {
      m_inlets.push_back(ossia::make_inlet<ossia::midi_port>());
    }
    for(std::size_t i = 0; i < InfoFunctions<Info>::value_in_count; i++)
    {
      m_inlets.push_back(ossia::make_inlet<ossia::value_port>());
    }
    for(std::size_t i = 0; i < InfoFunctions<Info>::control_count; i++)
    {
      m_inlets.push_back(ossia::make_inlet<ossia::value_port>());
    }

    for(std::size_t i = 0; i < InfoFunctions<Info>::audio_out_count; i++)
    {
      m_outlets.push_back(ossia::make_outlet<ossia::audio_port>());
    }
    for(std::size_t i = 0; i < InfoFunctions<Info>::midi_out_count; i++)
    {
      m_outlets.push_back(ossia::make_outlet<ossia::midi_port>());
    }
    for(std::size_t i = 0; i < InfoFunctions<Info>::value_out_count; i++)
    {
      m_outlets.push_back(ossia::make_outlet<ossia::value_port>());
    }
  }

  void run(ossia::token_request tk, ossia::execution_state& st) override
  {
    constexpr auto start = InfoFunctions<Info>::control_start;
    for(std::size_t i = 0; i < InfoFunctions<Info>::control_count; i++)
    {
      ossia::inlet& inlet = *m_inlets[start + i];
      auto& port = *inlet.data.target<ossia::value_port>();
      if(port.get_data().empty())
      {
        port.add_value(controls[i]);
      }
      else
      {
        controls[i] = port.get_data().back().value;
      }
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

      for(std::size_t i = 0; i < InfoFunctions<Info>::inlet_size; i++)
      {
        auto dest = Engine::score_to_ossia::makeDestination(dl, element.inlets_ref()[i]->address());
        if(dest)
        {
          node->inputs()[i]->address = &dest->address();
        }
      }
      constexpr const auto control_start = InfoFunctions<Info>::control_start;
      for(std::size_t i = 0; i < InfoFunctions<Info>::control_count; i++)
      {
        node->controls[i] = element.control(i);
      }


      for(std::size_t i = 0; i < InfoFunctions<Info>::outlet_size; i++)
      {
        auto dest = Engine::score_to_ossia::makeDestination(dl, element.outlets_ref()[i]->address());
        if(dest)
        {
          node->outputs()[i]->address = &dest->address();
        }
      }


      for(std::size_t idx = InfoFunctions<Info>::control_start;
          idx < InfoFunctions<Info>::control_start + InfoFunctions<Info>::control_count;
          idx++)
      {
        auto inlet = static_cast<ControlInlet*>(element.inlets_ref()[idx]);
        auto port = node->inputs()[idx]->data.template target<ossia::value_port>();

        QObject::connect(inlet, &ControlInlet::valueChanged,
                this, [=] (ossia::value val) {
          this->system().executionQueue.enqueue(
                [port, val]
          {
            port->add_value(std::move(val));
          });
        });
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

      std::size_t i = 0;
      constexpr auto start = InfoFunctions<Info>::control_start;
      ossia::for_each_in_tuple(
            get_controls(Info::info),
            [&] (const auto& ctrl) {
        auto inlet = static_cast<ControlInlet*>(object.inlets_ref()[start + i]);

        auto lab = new TextLabel{ctrl.name, this};
        vlay->addWidget(lab);

        auto widg = ctrl.make_widget(*inlet, m_dispatcher, this);
        vlay->addWidget(widg);

        i++;
      });

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
