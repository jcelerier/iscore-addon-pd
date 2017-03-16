#pragma once
#include <ossia/editor/scenario/time_process.hpp>
#include <QJSEngine>
#include <QJSValue>
#include <QString>
#include <memory>
#include <Engine/Executor/ProcessComponent.hpp>
#include <Engine/Executor/ExecutorContext.hpp>
#include <iscore/document/DocumentContext.hpp>
#include <iscore/document/DocumentInterface.hpp>
#include <ossia/editor/scenario/time_value.hpp>


#include "z_libpd.h"
#include "m_imp.h"

#include <ossia/editor/scenario/time_process.hpp>
#include <ossia/editor/scenario/time_constraint.hpp>
#include <ossia/editor/state/state_element.hpp>
#include <boost/graph/topological_sort.hpp>
#include <boost/graph/adjacency_list.hpp>
namespace ossia
{
struct glutton_connection { };
struct needful_connection { };

struct delayed_glutton_connection {
  // delayed at the source or at the target
};
struct delayed_needful_connection {
  // same
};
struct parallel_connection {
};
struct replacing_connection {
};
using connection = eggs::variant<
  glutton_connection,
  needful_connection,
  delayed_glutton_connection,
  delayed_needful_connection,
  parallel_connection,
  replacing_connection>;

struct audio_port;
struct midi_port;
struct value_port;
using port = eggs::variant<audio_port, midi_port, value_port>;
struct audio_port {

};

struct midi_port {

};

struct value_port {
  eggs::variant<ossia::Destination, std::shared_ptr<port>> destination;
};
class graph_node;
struct graph_edge
{
  connection con;
  std::shared_ptr<port> in;
  std::shared_ptr<port> out;
  std::shared_ptr<graph_node> in_node;
  std::shared_ptr<graph_node> out_node;
};

template<typename T, typename... Args>
auto make_port(Args&&... args)
{
  return std::make_shared<port>(T{std::forward<Args>(args)...});
}

class graph_node
{
  // Note : pour QtQuick : Faire View et Model qui hérite de View, puis faire binding automatique entre propriétés de la vue et du modèle
  // Utiliser... DSPatch ? Pd ?
  // Ports : midi, audio, value

  std::vector<std::shared_ptr<port>> in_ports;
  std::vector<std::shared_ptr<port>> out_ports;

  double previous_time{};
  double time{};
public:
  virtual ~graph_node()
  {
    // TODO moveme in cpp
  }

  graph_node()
  {
    in_ports.push_back(make_port<audio_port>());
    in_ports.push_back(make_port<value_port>());

    out_ports.push_back(make_port<audio_port>());
  }

  virtual void event()
  {

  }

  virtual void run()
  {

  }

  void set_time(double d)
  {
    previous_time = time;
    time = d;
  }
};

class graph : public ossia::time_process
{
  std::vector<std::shared_ptr<graph_node>> nodes;
  std::vector<std::shared_ptr<graph_edge>> edges;

  std::set<std::shared_ptr<graph_node>> enabled_nodes;

  std::vector<int> delay_ringbuffers;

  using graph_impl = boost::adjacency_list<
    boost::vecS,
    boost::vecS,
    boost::directedS,
    std::shared_ptr<graph_node>,
    std::shared_ptr<graph_edge>>;

  graph_impl user_graph;

  std::vector<std::function<void()>> call_list;
public:
  void enable(const std::shared_ptr<graph_node>& n)
  {
    enabled_nodes.insert(n);

    reconfigure();
  }

  void disable(const std::shared_ptr<graph_node>& n)
  {
    enabled_nodes.erase(n);

    reconfigure();
  }

  void connect(const std::shared_ptr<graph_edge>& edge)
  {

  }

  void disconnect(const std::shared_ptr<graph_edge>& edge)
  {

  }

  void reconfigure()
  {
    call_list.clear();
    std::deque<std::size_t> topo_order;
    try {
      boost::topological_sort(user_graph, std::front_inserter(topo_order));
    }
    catch(const boost::not_a_dag& e)
    {
      return;
    }

    for(std::size_t vtx : topo_order)
    {
      const auto& node = user_graph[vtx];
      call_list.push_back([=] {
        node->run();
      });
    }
  }

  state_element offset(time_value) override
  {
    return {};
  }

  state_element state() override
  {
    // Pull the graph
    for(auto& call : call_list)
      call();

    return {};
  }
};



class graph_process : public ossia::time_process
{
  std::shared_ptr<ossia::graph> graph;
  std::shared_ptr<ossia::graph_node> node;

public:
  ossia::state_element offset(ossia::time_value) override
  {
    return {};
  }

  ossia::state_element state() override
  {
    node->set_time(parent()->getDate() / parent()->getDurationNominal());
    return {};
  }

  void start() override
  {
    graph->enable(node);
  }

  void stop() override
  {
    graph->disable(node);
  }

  void pause() override
  {
  }

  void resume() override
  {
  }

  void mute_impl(bool) override
  {
  }
};
}

namespace Explorer
{
class DeviceDocumentPlugin;
}
namespace Device
{
class DeviceList;
}
namespace Engine { namespace Execution
{
class ConstraintComponent;
} }
namespace Pd
{
class ProcessModel;
class ProcessExecutor final :
        public ossia::time_process
{
    public:
        ProcessExecutor(
                const Explorer::DeviceDocumentPlugin& devices);

        ~ProcessExecutor();

        void setTickFun(const QString& val);

        ossia::state_element state(double);
        ossia::state_element state() override;
        ossia::state_element offset(ossia::time_value) override;

    private:
        const Device::DeviceList& m_devices;
        t_pdinstance * m_instance{};
        int m_dollarzero = 0;
};


class Component final :
        public ::Engine::Execution::ProcessComponent_T<Pd::ProcessModel, ProcessExecutor>
{
        COMPONENT_METADATA("78657f42-3a2a-4b80-8736-8736463442b4")
    public:
        Component(
                Engine::Execution::ConstraintComponent& parentConstraint,
                Pd::ProcessModel& element,
                const Engine::Execution::Context& ctx,
                const Id<iscore::Component>& id,
                QObject* parent);
};

using ComponentFactory = ::Engine::Execution::ProcessComponentFactory_T<Component>;
}
