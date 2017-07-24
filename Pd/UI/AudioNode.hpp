#pragma once
#include <Audio/SoundProcess/SoundProcessModel.hpp>
#include <Pd/DocumentPlugin.hpp>
#include <Engine/Executor/ProcessComponent.hpp>
#include <Process/Dataflow/DataflowObjects.hpp>
#include <Process/Process.hpp>

#include <Pd/Executor/PdExecutor.hpp>
#include <ossia/editor/value/value.hpp>

#include <iscore_addon_pd_export.h>

namespace Dataflow
{

class SoundComponent :
    public ProcessComponent_T<Audio::Sound::ProcessModel>
{
  COMPONENT_METADATA("92540a97-741c-4e36-a532-9edafc45c768")
public:
  SoundComponent(
      Audio::Sound::ProcessModel& autom,
      DocumentPlugin& doc,
      const Id<iscore::Component>& id,
      QObject* parent_obj):
    ProcessComponent_T<Audio::Sound::ProcessModel>{autom, doc, id, "SoundComponent", parent_obj}
  {
    connect(&autom, &Audio::Sound::ProcessModel::fileChanged,
            this, [=] { this->outletsChanged(); });
    auto itm = new NodeItem{*this};
    itm->setParentItem(doc.window.view.contentItem());
    itm->setPosition(QPointF(100, 100));
    ui = itm;
  }
/*

  std::size_t audioInlets() const override { return 0; }
  std::size_t messageInlets() const override { return 0; }
  std::size_t midiInlets() const override { return 0; }

  std::size_t audioOutlets() const override { return m_outlets.size(); }
  std::size_t messageOutlets() const override{ return 0; }
  std::size_t midiOutlets() const override{ return 0; }

  std::vector<Process::Port> inlets() const override { return {}; }
  std::vector<Process::Port> outlets() const override { return m_outlets; }

  ~SoundComponent()
  {
    cleanup();
  }

  std::vector<Id<Process::Cable>> cables() const override
  { return m_cables; }
  void addCable(Id<Process::Cable> c) override
  { m_cables.push_back(c); }
  void removeCable(Id<Process::Cable> c) override
  { m_cables.erase(ossia::find(m_cables, c)); }

private:
  std::vector<Process::Port> m_outlets;
  std::vector<Id<Process::Cable>> m_cables;
  */
};

using SoundComponentFactory = ProcessComponentFactory_T<SoundComponent>;

class ISCORE_ADDON_PD_EXPORT SoundGraphNode final :
    public ossia::graph_node
{
public:
  SoundGraphNode()
  {
  }

  void setSound(AudioArray vec)
  {
    m_data = std::move(vec);
    m_outlets.clear();
    for(int i = 0; i < m_data.size(); i++)
      m_outlets.push_back(ossia::make_outlet<ossia::audio_port>());
  }

  ~SoundGraphNode()
  {

  }

private:
  void run(ossia::execution_state& e) override
  {
    if(m_data.empty())
      return;
    const std::size_t len = m_data[0].size();

    for(std::size_t i = 0; i < m_data.size(); i++)
    {
      ossia::audio_port& ap = *m_outlets[i].target<ossia::audio_port>();

      if(m_date > m_prev_date)
      {
        std::size_t max_N = std::min(m_date, len);
        ap.samples.resize(max_N - m_prev_date);
        for(int j = m_prev_date; j < max_N; j++)
        {
          ap.samples[j - m_prev_date] = m_data[i][j];
        }
      }
      else
      {
        // TODO play backwards
      }

    }

  }

  AudioArray m_data;
};

class SoundExecComponent final
    : public ::Engine::Execution::
    ProcessComponent_T<Audio::Sound::ProcessModel, ossia::node_process>
{
  COMPONENT_METADATA("a25d0de0-74e2-4011-aeb6-4188673015f2")
public:
  SoundExecComponent(
      Engine::Execution::ConstraintComponent& parentConstraint,
      Audio::Sound::ProcessModel& element,
      const Dataflow::DocumentPlugin& df,
      const ::Engine::Execution::Context& ctx,
      const Id<iscore::Component>& id,
      QObject* parent)
    : Engine::Execution::ProcessComponent_T<Audio::Sound::ProcessModel, ossia::node_process>{
        parentConstraint,
        element,
        ctx,
        id, "Executor::SoundComponent", parent}
    , m_df{df}
  {
    m_ossia_process = std::make_shared<ossia::node_process>(m_df.execGraph);
    node = std::make_shared<SoundGraphNode>();

    con(element, &Audio::Sound::ProcessModel::fileChanged,
        this, [this] (const auto&) { this->recompute(); });
  }

  void recompute()
  {
    system().executionQueue.enqueue(
          [n=std::dynamic_pointer_cast<SoundGraphNode>(this->node)
          ,data=process().file().data()
          ]
    {
      n->setSound(std::move(data));
    });
  }

  ~SoundExecComponent()
  {
    if(node) node->clear();
  }

private:
  ossia::node_ptr node;
  const Dataflow::DocumentPlugin& m_df;
};
using SoundExecComponentFactory
= Pd::ProcessComponentFactory_T<SoundExecComponent>;

}
