#pragma once

#include <nodes/FlowScene>
#include <nodes/FlowView>
#include <nodes/../../src/Node.hpp>
#include <nodes/../../src/NodeGraphicsObject.hpp>
#include <nodes/../../src/ConnectionGraphicsObject.hpp>
#include <QPointer>
#include <QGraphicsSceneMouseEvent>
#include <Pd/DataflowProcess.hpp>
#include <QMenu>
#include <QAction>
#include <QActionGroup>
#include <QApplication>
#include <QMenuBar>
#include <QVBoxLayout>

namespace Dataflow
{
class CustomConnection : public QtNodes::ConnectionGraphicsObject
{
  Q_OBJECT
public:
  using QtNodes::ConnectionGraphicsObject::ConnectionGraphicsObject;

  Process::CableType cableType;

signals:
  void selectionChanged(bool);

private:
  QVariant itemChange(
      QGraphicsItem::GraphicsItemChange change,
      const QVariant& value)
  {
    if(change == QGraphicsItem::ItemSelectedHasChanged)
    {
      emit selectionChanged(value.toBool());
    }

    return ConnectionGraphicsObject::itemChange(change, value);
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


class DataflowWindow: public QObject
{
  Q_OBJECT
public:
  DataflowWindow()
  {
    layout.addWidget(&menu);
    layout.addWidget(&view);

    // ImmediateGlutton, ImmediateStrict, DelayedGlutton, DelayedStrict
    a1->setCheckable(true);
    a2->setCheckable(true);
    a3->setCheckable(true);
    a4->setCheckable(true);

    auto acts = new QActionGroup{&menu};
    acts->addAction(a1);
    acts->addAction(a2);
    acts->addAction(a3);
    acts->addAction(a4);
    acts->setExclusive(true);
    menu.addAction(a1);
    menu.addAction(a2);
    menu.addAction(a3);
    menu.addAction(a4);

    connect(a1, &QAction::toggled, this, &DataflowWindow::on_typeChanged);
    connect(a2, &QAction::toggled, this, &DataflowWindow::on_typeChanged);
    connect(a3, &QAction::toggled, this, &DataflowWindow::on_typeChanged);
    connect(a4, &QAction::toggled, this, &DataflowWindow::on_typeChanged);
  }

  void cableSelected(QtNodes::Connection& con)
  {
    selected.push_back(&con);
    auto go = static_cast<CustomConnection*>(&con.getConnectionGraphicsObject());
    if(selected.size() == 1)
    {
      switch(go->cableType) {
        case Process::CableType::ImmediateGlutton: a1->setChecked(true); break;
        case Process::CableType::ImmediateStrict: a2->setChecked(true); break;
        case Process::CableType::DelayedGlutton: a3->setChecked(true); break;
        case Process::CableType::DelayedStrict: a4->setChecked(true); break;
        default: break;
      }
    }
    else
    {
      // TODO Set "half" check state for everyone
    }
  }

  void cableDeselected(QtNodes::Connection& con)
  {
    selected.removeAll(&con);
  }


  void on_typeChanged()
  {
    if(a1->isChecked()) emit typeChanged(selected, Process::CableType::ImmediateGlutton);
    else if (a2->isChecked()) emit typeChanged(selected, Process::CableType::ImmediateStrict);
    else if (a3->isChecked()) emit typeChanged(selected, Process::CableType::DelayedGlutton);
    else if (a4->isChecked()) emit typeChanged(selected, Process::CableType::DelayedStrict);

  }

  QAction* a1 = new QAction{"Immediate Glutton"};
  QAction* a2 = new QAction{"Immediate Strict"};
  QAction* a3 = new QAction{"Delayed Glutton"};
  QAction* a4 = new QAction{"Delayed Strict"};

  QWidget window;
  QMenuBar menu;
  ReleaseFlowScene scene;

  QVBoxLayout layout{&window};

  QList<QtNodes::Connection*> selected;
signals:
  void typeChanged(QList<QtNodes::Connection*>, Process::CableType);

private:

  QtNodes::FlowView view{&scene};
};
}
