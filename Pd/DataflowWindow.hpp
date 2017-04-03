#pragma once

#include <nodes/FlowScene>
#include <nodes/FlowView>
#include <QPointer>
#include <Pd/DataflowProcess.hpp>
namespace Dataflow
{
class CustomDataModel : public QtNodes::NodeDataModel
{
public:
  QPointer<Dataflow::ProcessModel> process;

  CustomDataModel(Dataflow::ProcessModel& proc):
    process{&proc}
  {
    process->nodeModel = this;
  }

  ~CustomDataModel()
  {
    if(process)
      process->nodeModel = nullptr;
  }

  QString caption() const override
  {
    return "caption";
  }

  QString name() const override
  {
    if(process)
      return process->metadata().getName();
    return "DEAD";
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

class DataflowWindow
{
public:
  DataflowWindow()
  {

  }

  QtNodes::FlowScene scene;
  QtNodes::FlowView view{&scene};
};
}
