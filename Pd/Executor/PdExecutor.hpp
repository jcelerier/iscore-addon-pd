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
#include <boost/container/flat_set.hpp>
#include <boost/bimap.hpp>
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
  graph_edge(connection c, std::shared_ptr<port> in, std::shared_ptr<port> out, std::shared_ptr<graph_node> in_node, std::shared_ptr<graph_node> out_node):
    con{c},
    in{in},
    out{out},
    in_node{in_node},
    out_node{out_node}
  {

  }

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

using ports = std::vector<std::shared_ptr<port>>;

class graph_node
{
protected:
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

  const ports& inputs() const { return in_ports; }
  const ports& outputs() const { return out_ports; }
};

template<typename T>
using set = boost::container::flat_set<T>;
template<typename T, typename U>
using bimap = boost::bimap<T, U>;
class graph : public ossia::time_process
{
  using graph_t = boost::adjacency_list<
    boost::vecS,
    boost::vecS,
    boost::directedS,
    std::shared_ptr<graph_node>,
    std::shared_ptr<graph_edge>>;

  using graph_vertex_t = graph_t::vertex_descriptor;
  using graph_edge_t = graph_t::edge_descriptor;

  using node_bimap = bimap<graph_vertex_t, std::shared_ptr<graph_node>>;
  using edge_bimap = bimap<graph_edge_t, std::shared_ptr<graph_edge>>;
  using node_bimap_v = node_bimap::value_type;
  using edge_bimap_v = edge_bimap::value_type;

  node_bimap nodes;
  edge_bimap edges;

  set<std::shared_ptr<graph_node>> enabled_nodes;

  std::vector<int> delay_ringbuffers;


  graph_t user_graph;

  std::vector<std::function<void()>> call_list;
public:
  void add_node(const std::shared_ptr<graph_node>& n)
  {
    nodes.insert(node_bimap_v{boost::add_vertex(n, user_graph), n});
    reconfigure();
  }

  void remove_node(const std::shared_ptr<graph_node>& n)
  {
    auto it = nodes.right.find(n);
    if(it != nodes.right.end())
    {
      boost::remove_vertex(it->second, user_graph);
      nodes.right.erase(it);
      reconfigure();
    }
  }

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
    auto it1 = nodes.right.find(edge->in_node);
    auto it2 = nodes.right.find(edge->out_node);
    if(it1 != nodes.right.end() && it2 != nodes.right.end())
    {
      auto res = boost::add_edge(it1->second, it2->second, edge, user_graph);
      if(res.second)
      {
        edges.insert(edge_bimap_v{res.first, edge});
      }
    }

    reconfigure();
  }

  void disconnect(const std::shared_ptr<graph_edge>& edge)
  {
    auto it = edges.right.find(edge);
    if(it != edges.right.end())
    {
      boost::remove_edge(it->second, user_graph);
      reconfigure();
    }
  }

  void reconfigure()
  {
    call_list.clear();
    std::deque<graph_vertex_t> topo_order;
    try {
      boost::topological_sort(user_graph, std::front_inserter(topo_order));
    }
    catch(const boost::not_a_dag& e)
    {
      return;
    }

    // topo_order contains a list of connected things starting from the root and going through the leaves
    // now how to handle nodes with the same parameters without specification ?

    // two kinds of edges : explicit edges and implicit edges

    // note : implicit edges must be checked at runtime (imagine a js process that writes to random adresses on each tick)
    // this also means that besides the policy, the adresses should have a default behaviour set.
    // Or maybe it should be per node or per port ?

    // Maybe we should have the "connection graph", between ports, and the "node graph" where the user explicitely says
    // he wants a process to be executed before another. And a check that both aren't contradictory.

    // What we cannot do, however, is know automatically which process reads which address : it works only for outbound ports.
    // Inbound ports have to be specified
    for(graph_vertex_t vtx : topo_order)
    {
      const auto& node = user_graph[vtx];
      call_list.push_back([=] {
        node->run();
      });
    }

    // So
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

class node_mock : public graph_node {

public:
  node_mock(ports in, ports out)
  {
    in_ports = std::move(in);
    out_ports = std::move(out);
  }
};

class graph_mock
{
  graph g;
public:
  graph_mock()
  {
    auto n1 = std::make_shared<node_mock>(ports{make_port<value_port>()}, ports{make_port<value_port>()});
    auto n2 = std::make_shared<node_mock>(ports{make_port<value_port>()}, ports{make_port<value_port>()});
    auto n3 = std::make_shared<node_mock>(ports{make_port<value_port>()}, ports{make_port<value_port>()});
    auto n4 = std::make_shared<node_mock>(ports{make_port<value_port>(), make_port<value_port>()}, ports{make_port<value_port>()});

    g.add_node(n1);
    g.add_node(n2);
    g.add_node(n3);
    g.add_node(n4);

    auto c1 = std::make_shared<graph_edge>(connection{glutton_connection{}}, n1->outputs()[0], n2->inputs()[0], n1, n2);
    auto c2 = std::make_shared<graph_edge>(connection{glutton_connection{}}, n1->outputs()[0], n3->inputs()[0], n1, n3);
    auto c3 = std::make_shared<graph_edge>(connection{glutton_connection{}}, n2->outputs()[0], n4->inputs()[0], n2, n4);
    auto c4 = std::make_shared<graph_edge>(connection{glutton_connection{}}, n3->outputs()[0], n4->inputs()[1], n3, n4);

    g.connect(c1);
    g.connect(c2);
    g.connect(c3);
    g.connect(c4);
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
