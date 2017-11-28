#include "PdNode.hpp"
#include <random>
namespace Pd
{
namespace PulseToNote
{
struct ratio
{
  int num{};
  int denom{};

  friend constexpr ratio operator+(const ratio& lhs, const ratio& rhs)
  {
    return {
      lhs.num * rhs.denom + rhs.num * lhs.denom,
               lhs.denom * rhs.denom
    };
  }
};

struct Node
{
  struct Metadata
  {
    static const constexpr auto prettyName = "Pulse to Note";
    static const constexpr auto objectKey = "VelToNote";
    static const constexpr auto uuid = make_uuid("2c6493c3-5449-4e52-ae04-9aee3be5fb6a");
  };

  struct NoteIn
  {
    int note{};
    ossia::time_value end_date{};
  };
  struct State
  {
    std::vector<NoteIn> to_start;
    std::vector<NoteIn> running_notes;
  };


  static const constexpr std::array<std::pair<const char*, float>, 13> notes
  {{
      {"None",  0.},
      {"Whole", 1.},
      {"Half",  1./2.},
      {"4th",   1./4.},
      {"8th",   1./8.},
      {"16th",  1./16.},
      {"32th",  1./32.},
      {"64th",  1./64.},
      {"Dotted Half",  3./4.},
      {"Dotted 4th",   3./8.},
      {"Dotted 8th",   3./16.},
      {"Dotted 16th",  3./32.},
      {"Dotted 32th",  3./64.}
    }};
  static const constexpr auto info =
      Process::create_node()
      .value_ins({{"in"}})
      .midi_outs({{"out"}})
      .controls(
                Process::ComboBox<float, std::size(notes)>{"Quantification", 2, notes},
                Process::ComboBox<float, std::size(notes)>{"Duration", 2, notes},
                Process::IntSpinBox{"Default pitch", 0, 127, 64},
                Process::IntSpinBox{"Default vel.", 0, 127, 64},
                Process::IntSlider{"Pitch random", 0, 24, 0},
                Process::IntSlider{"Vel. random", 0, 24, 0}
                )
      .state<State>()
      .build();

  struct val_visitor
  {
    State& st;
    int base_note{};
    int base_vel{};

    struct note { int pitch{}; int vel{}; };
    template<typename... T>
    note operator()(T&&... v)
    {
      return {base_note, base_vel};
    }
    note operator()(int vel)
    {
      return {base_note, vel};
    }
    note operator()(int note, int vel)
    {
      return {note, vel};
    }
    note operator()(const std::vector<ossia::value>& v)
    {
      switch(v.size())
      {
        case 0: return operator()();
        case 1: return operator()(ossia::convert<int>(v[0]));
        default: return operator()(ossia::convert<int>(v[0]), ossia::convert<int>(v[1]));
      }
    }
    template<std::size_t N>
    note operator()(const std::array<float, N>& v)
    {
      return operator()(v[0], v[1]);
    }
  };
  static void run(
      const ossia::value_port& p1,
      const Process::timed_vec<float>& startq,
      const Process::timed_vec<float>& dur,
      const Process::timed_vec<int>& basenote,
      const Process::timed_vec<int>& basevel,
      const Process::timed_vec<int>& note_random,
      const Process::timed_vec<int>& vel_random,
      ossia::midi_port& p2,
      State& self,
      ossia::time_value prev_date,
      ossia::token_request tk,
      ossia::execution_state& st)
  {
    static std::mt19937 m;

    // When a message is received, we have three cases :
    // 1. Just an impulse: use base note & base vel
    // 2. Just an int: it's the velocity, use base note
    // 3. A tuple [int, int]: it's note and velocity

    // Then once we have a pair [int, int] we randomize and we output a note on.

    // At the end, scan running_notes: if any is going to end in this buffer, end it too.

    auto start = startq.rbegin()->second;
    auto duration = dur.rbegin()->second;
    auto base_note = basenote.rbegin()->second;
    auto base_vel = basevel.rbegin()->second;
    auto rand_note = note_random.rbegin()->second;
    auto rand_vel = vel_random.rbegin()->second;


    for(auto& in : p1.get_data())
    {
      auto note = in.value.apply(val_visitor{self, base_note, base_vel});

      if(rand_note != 0)
        note.pitch += std::uniform_int_distribution(-rand_note, rand_note)(m);
      if(rand_vel != 0)
        note.vel += std::uniform_int_distribution(-rand_vel, rand_vel)(m);

      note.pitch = ossia::clamp(note.pitch, 0, 127);
      note.vel = ossia::clamp(note.vel, 0, 127);

      auto no = mm::MakeNoteOn(1, note.pitch, note.vel);
      no.timestamp = in.timestamp;


      p2.messages.push_back(no);
      self.running_notes.push_back({note.pitch, tk.date + (int64_t)no.timestamp + (int64_t)duration});
    }

    for(auto it = self.running_notes.begin(); it != self.running_notes.end(); )
    {
      auto& note = *it;
      if(note.end_date > prev_date && note.end_date < tk.date)
      {

      }
      else
      {
        ++it;
      }
    }

  }
};

using Factories = Process::Factories<Node>;
}

}
