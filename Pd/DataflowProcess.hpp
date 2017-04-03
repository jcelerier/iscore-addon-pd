#pragma once
#include <Process/Process.hpp>
#include <Process/TimeValue.hpp>
#include <State/Address.hpp>
#include <iscore/serialization/VisitorInterface.hpp>
#include <ossia/dataflow/dataflow.hpp>
namespace QtNodes { class Node; }
namespace Dataflow
{
class CustomDataModel;
enum class PortType { Message, Audio, Midi };
struct Port
{
  PortType type;
  State::AddressAccessor address;

  friend bool operator==(const Port& lhs, const Port& rhs)
  {
    return lhs.type == rhs.type && lhs.address == rhs.address;
  }
};

class ProcessModel : public Process::ProcessModel
{
  Q_OBJECT
  ISCORE_SERIALIZE_FRIENDS
public:

    using Process::ProcessModel::ProcessModel;

  explicit ProcessModel(
          const ProcessModel& source,
          const Id<Process::ProcessModel>& id,
          const QString& name,
          QObject* parent);

  template<typename Impl>
  explicit ProcessModel(
          Impl& vis,
          QObject* parent) :
      Process::ProcessModel{vis, parent}
  {
      vis.writeTo(*this);
  }

  ~ProcessModel()
  {

  }

  void setInlets(const std::vector<Port>& inlets)
  {
    if(inlets != m_inlets)
    {
      m_inlets = inlets;
      emit inletsChanged();
    }
  }

  void setOutlets(const std::vector<Port>& outlets)
  {
    if(outlets != m_outlets)
    {
      m_outlets = outlets;
      emit outletsChanged();
    }
  }

  const std::vector<Port>& inlets() const { return m_inlets; }
  const std::vector<Port>& outlets() const { return m_outlets; }

  CustomDataModel* nodeModel{};
  QtNodes::Node* node{};
signals:
  void inletsChanged();
  void outletsChanged();

private:
  std::vector<Port> m_inlets;
  std::vector<Port> m_outlets;
};
}
