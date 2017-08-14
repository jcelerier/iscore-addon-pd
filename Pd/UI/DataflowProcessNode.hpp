#pragma once
#include <Process/Dataflow/DataflowObjects.hpp>
#include <Pd/DocumentPlugin.hpp>
#include <iscore_addon_pd_export.h>

namespace Dataflow
{
class NodeItem;

class DataflowProcessNode
        : public Process::Node
{
  public:
    DataflowProcessNode(
          Dataflow::DocumentPlugin& doc,
          Process::DataflowProcess& proc,
          Id<Node> c,
          QObject* parent);

    Process::DataflowProcess& process;

    ~DataflowProcessNode()
    {
      cleanup();
    }

    QString getText() const override { return process.metadata().getName(); }
    std::size_t audioInlets() const override { return process.audioInlets(); }
    std::size_t messageInlets() const override { return process.messageInlets(); }
    std::size_t midiInlets() const override { return process.midiInlets(); }

    std::size_t audioOutlets() const override { return process.audioOutlets(); }
    std::size_t messageOutlets() const override{ return process.messageOutlets(); }
    std::size_t midiOutlets() const override{ return process.midiOutlets(); }

    std::vector<Process::Port> inlets() const override { return process.inlets(); }
    std::vector<Process::Port> outlets() const override { return process.outlets(); }

    std::vector<Id<Process::Cable>> cables() const override { return process.cables; }
    void addCable(Id<Process::Cable> c) override { process.cables.push_back(c); }
    void removeCable(Id<Process::Cable> c) override {
      auto it = ossia::find(process.cables, c);
      if(it != process.cables.end())
        process.cables.erase(it);
    }
};

class ISCORE_ADDON_PD_EXPORT DataFlowProcessComponent
        : public ProcessComponent_T<Process::DataflowProcess>
{
  public:
    DataFlowProcessComponent(
        Process::DataflowProcess& process,
        DocumentPlugin& doc,
        const Id<iscore::Component>& id,
        const QString& name,
        QObject* parent);

    ~DataFlowProcessComponent();

    DataflowProcessNode& mainNode() override { return m_node; }
  private:
    DataflowProcessNode m_node;
};

}
