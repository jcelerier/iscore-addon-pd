#pragma once

#include <nodes/FlowScene>
#include <nodes/FlowView>
namespace Dataflow
{
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
