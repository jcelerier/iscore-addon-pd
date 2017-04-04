#pragma once

#include <nodes/FlowScene>
#include <nodes/FlowView>
#include <nodes/../../src/Node.hpp>
#include <nodes/../../src/NodeGraphicsObject.hpp>
#include <nodes/../../src/ConnectionGraphicsObject.hpp>
#include <QPointer>
#include <QGraphicsSceneMouseEvent>
#include <Pd/DataflowProcess.hpp>
namespace Dataflow
{
class CustomConnection : public QtNodes::ConnectionGraphicsObject
{
public:
  using QtNodes::ConnectionGraphicsObject::ConnectionGraphicsObject;

private:
  void contextMenuEvent(QGraphicsSceneContextMenuEvent *contextMenuEvent) override
  {
    qDebug("yaaaa");
  }

};

class CustomDataModel : public QtNodes::NodeDataModel
{
public:
  QPointer<Dataflow::ProcessModel> process;

  CustomDataModel(Dataflow::ProcessModel& proc):
    process{&proc}
  {
    process->nodeModel = this;
    connect(process, &Dataflow::ProcessModel::posChanged,
            this, [&] (QPointF pos) {
      if(process->node)
        process->node->nodeGraphicsObject().setPos(pos);
    });

  }

  ~CustomDataModel()
  {
    if(process)
      process->nodeModel = nullptr;
  }

  QString caption() const override
  {
    if(process)
      return process->metadata().getName();
    return QStringLiteral("DEAD");
  }

  QString name() const override
  {
    if(process)
      return process->metadata().getName();
    return QStringLiteral("DEAD");
  }

  std::unique_ptr<QtNodes::NodeDataModel> clone() const override
  {
    return {};
  }

  unsigned int nPorts(QtNodes::PortType portType) const override
  {
    if(process)
    {
      if(portType == QtNodes::PortType::In)
        return process->inlets().size();
      else if(portType == QtNodes::PortType::Out)
        return process->outlets().size();
      return 0;
    }
    return 0;
  }

  QtNodes::NodeDataType dataType(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const override
  {
    return QtNodes::NodeDataType{};
  }

  void setInData(std::shared_ptr<QtNodes::NodeData> nodeData, QtNodes::PortIndex port) override
  {
  }

  std::shared_ptr<QtNodes::NodeData> outData(QtNodes::PortIndex port) override
  {
    return {};
  }

  QWidget* embeddedWidget() override
  {
    return nullptr;
  }
};

class ReleaseFlowScene : public QtNodes::FlowScene
{
  Q_OBJECT
public:
  using FlowScene::FlowScene;

signals:
  void released(QPointF);
private:
  void mouseReleaseEvent(QGraphicsSceneMouseEvent* event) override
  {
    emit released(event->pos());
    QGraphicsScene::mouseReleaseEvent(event);
  }
};


class DataflowWindow
{
public:
  DataflowWindow()
  {

  }

  ReleaseFlowScene scene;
  QtNodes::FlowView view{&scene};
};
}
