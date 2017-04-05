#pragma once
#include <Pd/DataflowObjects.hpp>
#include <Process/Process.hpp>
#include <Process/TimeValue.hpp>
#include <iscore/serialization/VisitorInterface.hpp>
#include <ossia/dataflow/dataflow.hpp>
namespace QtNodes { class Node; }
namespace Dataflow
{
class CustomDataModel;
class ProcessModel : public Process::ProcessModel
{
  Q_OBJECT
  ISCORE_SERIALIZE_FRIENDS
  Q_PROPERTY(QPointF pos READ pos WRITE setPos NOTIFY posChanged)
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
      updateCounts();
  }

  ~ProcessModel();

  void setInlets(const std::vector<Port>& inlets);
  void setOutlets(const std::vector<Port>& outlets);

  std::size_t audioInlets() const { return m_portCount.audioIn; }
  std::size_t messageInlets() const { return m_portCount.messageIn; }
  std::size_t midiInlets() const { return m_portCount.midiIn; }

  std::size_t audioOutlets() const { return m_portCount.audioOut; }
  std::size_t messageOutlets() const { return m_portCount.messageOut; }
  std::size_t midiOutlets() const { return m_portCount.midiOut; };

  const std::vector<Port>& inlets() const;
  const std::vector<Port>& outlets() const;

  CustomDataModel* nodeModel{};
  QtNodes::Node* node{};

  std::vector<Cable> cables;
  QPointF pos() const;

public slots:
  void setPos(QPointF pos);

signals:
  void inletsChanged();
  void outletsChanged();
  void posChanged(QPointF pos);

private:
  void updateCounts();
  std::vector<Port> m_inlets;
  std::vector<Port> m_outlets;
  QPointF m_pos;

  struct {
  std::size_t audioIn{}, messageIn{}, midiIn{};
  std::size_t audioOut{}, messageOut{}, midiOut{};
  } m_portCount;
};
}
