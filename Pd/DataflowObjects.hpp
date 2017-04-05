#pragma once
#include <iscore/model/path/Path.hpp>
#include <State/Address.hpp>
#include <ossia/detail/optional.hpp>

namespace Dataflow
{
class ProcessModel;
enum class CableType { ImmediateGlutton, ImmediateStrict, DelayedGlutton, DelayedStrict };
struct Cable
{
  CableType type;
  Path<Dataflow::ProcessModel> source, sink;
  ossia::optional<int> outlet, inlet;

  friend bool operator==(const Cable& lhs, const Cable& rhs)
  {
    return lhs.type == rhs.type
        && lhs.source == rhs.source
        && lhs.sink == rhs.sink
        && lhs.outlet == rhs.outlet
        && lhs.inlet == rhs.inlet;
  }
};

enum class PortType { Message, Audio, Midi };
struct Port
{
  PortType type;
  QString customData;
  State::AddressAccessor address;

  friend bool operator==(const Port& lhs, const Port& rhs)
  {
    return lhs.type == rhs.type && lhs.customData == rhs.customData && lhs.address == rhs.address;
  }
};

}
