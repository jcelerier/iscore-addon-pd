#include "PdNode.hpp"
#include <array>
#include <tuple>
#include <utility>
#include <algorithm>
#include <map>
#include <bitset>

namespace Pd
{



template<std::size_t N>
struct num { static const constexpr auto value = N; };

/////////////
template <class F, class... Ts, std::size_t... Is>
void for_each_in_tuple(
    const std::tuple<Ts...>& tuple, F func, std::index_sequence<Is...>)
{
  using expander = int[];
  (void)expander{0, ((void)func(std::get<Is>(tuple)), 0)...};
}

template <class F, class... Ts>
void for_each_in_tuple(const std::tuple<Ts...>& tuple, F func)
{
  for_each_in_tuple(tuple, func, std::make_index_sequence<sizeof...(Ts)>());
}

///////////
template <class F, std::size_t... Is>
void for_each_in_range(
    F func, std::index_sequence<Is...>)
{
  using expander = int[];
  (void)expander{0, ((void)func(num<Is>{}), 0)...};
}

template <std::size_t N, typename F>
void for_each_in_range(F func)
{
  for_each_in_range(func, std::make_index_sequence<N>());
}

///////////

template <class F, class... Ts, class... T2s, std::size_t... Is>
void for_each_in_tuples(
    const std::tuple<Ts...>& tuple, const std::tuple<T2s...>& tuple2, F func, std::index_sequence<Is...>)
{
  using expander = int[];
  (void)expander{0, ((void)func(std::get<Is>(tuple), std::get<Is>(tuple2), num<Is>{}), 0)...};
}

template <class F, class... Ts, class... T2s>
void for_each_in_tuples(const std::tuple<Ts...>& tuple, const std::tuple<T2s...>& tuple2, F func)
{
  for_each_in_tuples(tuple, tuple2, func, std::make_index_sequence<sizeof...(Ts)>());
}

/////////////

template <class F, class... Ts, class... T2s,  class... T3s, std::size_t... Is>
void for_each_in_tuples3(
    const std::tuple<Ts...>& tuple, const std::tuple<T2s...>& tuple2, std::tuple<T3s...>& tuple3, F func, std::index_sequence<Is...>)
{
  using expander = int[];
  (void)expander{0, ((void)func(std::get<Is>(tuple), std::get<Is>(tuple2), std::get<Is>(tuple3), num<Is>{}), 0)...};
}

template <class F, class... Ts, class... T2s, class... T3s>
void for_each_in_tuples3(const std::tuple<Ts...>& tuple, const std::tuple<T2s...>& tuple2, std::tuple<T3s...>& tuple3, F func)
{
  for_each_in_tuples3(tuple, tuple2, tuple3, func, std::make_index_sequence<sizeof...(Ts)>());
}

///////////////

template <class F, class... Ts, class... T2s, std::size_t... Is>
bool all_of_in_tuple(
    const std::tuple<Ts...>& tuple, const std::tuple<T2s...>& tuple2, F func, std::index_sequence<Is...>)
{
  std::array res{func(std::get<Is>(tuple), std::get<Is>(tuple2))...};
  return std::all_of(res.begin(), res.end(), [] (bool b) { return b; });
}

template <class F, class... Ts, class... T2s>
bool all_of_in_tuple(
    const std::tuple<Ts...>& tuple, const std::tuple<T2s...>& tuple2, F func)
{
  return all_of_in_tuple(tuple, tuple2, func, std::make_index_sequence<sizeof...(Ts)>());
}
/////////////////
template<typename... Args>
auto timestamp(const std::pair<Args...>& p)
{
  return p.first;
}
template<typename T>
auto timestamp(const T& p)
{
  return p.timestamp;
}
template<typename TickFun, typename... Args>
auto sub_tick(TickFun f, ossia::time_value prev_date, ossia::token_request req, const Process::timed_vec<Args>&... arg)
{
  constexpr const std::size_t N = sizeof...(arg);
  auto iterators = std::make_tuple(arg.begin()...);
  const auto last_iterators = std::make_tuple(--arg.end()...);

  // while all the it are != arg.rbegin(),
  //  increment the smallest one
  //  call a tick with this at the new date

  auto reached_end = [&] {
    return all_of_in_tuple(iterators, last_iterators,
                           [] (const auto& it, const auto& rbit) { return it == rbit; });
  };

  const auto parent_dur = req.date / req.position;
  auto call_f = [&] (ossia::time_value cur) {
    ossia::token_request r = req;
    //r.date +=
    std::apply([&] (const auto&... it) { f(it->second...); }, iterators);
  };

  ossia::time_value current_time = 0;
  while(!reached_end())
  {
    // run a tick with the current values (TODO pass the current time too)
    call_f();

    std::bitset<sizeof...(Args)> to_increment;
    to_increment.reset();
    auto min = ossia::Infinite;
    for_each_in_tuples(iterators, last_iterators,
                       [&] (const auto& it, const auto& last, auto idx_t) {
      constexpr auto idx = decltype(idx_t)::value;
      if(it != last)
      {
        auto next = it; ++next;
        const auto next_ts = timestamp(*next);
        const auto diff = next_ts - current_time;
        if(diff < min)
        {
          min = diff;
          to_increment.reset();
          to_increment.set(idx);
        }
        else if(diff == min)
        {
          to_increment.set(idx);
        }
      }
    });

    current_time += min;
    for_each_in_range<N>([&] (auto idx_t)
    {
      constexpr auto idx = decltype(idx_t)::value;
      if(to_increment.test(idx))
      {
        ++std::get<idx>(iterators);
      }
    });
  }

  call_f();
}

namespace LFO
{

struct Node
{
  struct Metadata
  {
    static const constexpr auto prettyName = "My Funny Process";
    static const constexpr auto objectKey = "FunnyProcess";
    static const constexpr auto uuid = make_uuid("f6b88ec9-cd56-43e8-a568-33208d5a8fb7");
  };

  struct State
  {

  };

  static const constexpr auto info =
      Process::create_node()
      .value_outs({"out"})
      .controls(Process::FloatSlider{"Freq.", 0., 50., 1.}
              , Process::FloatSlider{"Coarse intens.", 0., 1000., 1.}
              , Process::FloatSlider{"Fine intens.", 0., 1., 1.}
              , Process::FloatSlider{"Jitter", 0., 1., 0.}
              , Process::FloatSlider{"Phase", -1., 1., 0.}
              , Process::Enum<7>{"Function", 0,
                  {"Sin", "Triangle", "Square", "Tan", "Noise 1", "Noise 2", "Noise 3"}
                }
                )
      .state<State>()
      .build();

  static void run(
      const Process::timed_vec<float>& freq_vec,
      const Process::timed_vec<float>& coarse_vec,
      const Process::timed_vec<float>& fine_vec,
      const Process::timed_vec<float>& jitter_vec,
      const Process::timed_vec<float>& phase_vec,
      const Process::timed_vec<int>& type_vec,
      ossia::value_port& out,
      State&,
      ossia::time_value prev_date,
      ossia::token_request tk,
      ossia::execution_state& st)
  {
    sub_tick([] (ossia::token_request req, auto&&... args) {
      run_impl(args..., out, st, prev_date, req, st);
    }, tk, freq_vec, coarse_vec, fine_vec, jitter_vec, phase_vec, type_vec);
  }

  static void run_impl(
      float freq, float coarse, float fine, float jitter, float phase, int type,
      ossia::value_port& out,
      State&,
      ossia::time_value prev_date,
      ossia::token_request tk,
      ossia::execution_state& st)
  {/*
    sub_tick([] (float, float, float, float, float, int, ) {

    }, freq_vec, coarse_vec, fine_vec, jitter_vec, phase_vec, type_vec);
    */
  }
};
}

}
