// This is an open source non-commercial project. Dear PVS-Studio, please check
// it. PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com
#include <ossia/dataflow/dataflow.hpp>
#include <ossia/dataflow/execution_state.hpp>
#include <ossia/dataflow/fx_node.hpp>
#include <ossia/dataflow/graph_edge.hpp>
#include <ossia/dataflow/graph_node.hpp>
#include <ossia/dataflow/node_process.hpp>

namespace ossia
{

node_process::node_process(node_ptr n)
{
  assert(n);
  node = std::move(n);
}

void node_process::offset(time_value date, double pos)
{
}

void node_process::transport(time_value date, double pos)
{
}

void node_process::state(
    ossia::time_value from, ossia::time_value to, double relative_position,
    time_value tick_offset, double gspeed)
{
  node->request({from, to, relative_position, tick_offset, gspeed});
}

void node_process::start()
{
  // TODO reset all delay buffer positions
}

void node_process::stop()
{
  if(node) node->all_notes_off();
}

void node_process::pause()
{
}

void node_process::resume()
{
}

void node_process::mute_impl(bool)
{
}

node_process::~node_process()
{
}

graph_edge::graph_edge(
    connection c, std::size_t pout, std::size_t pin, node_ptr pout_node,
    node_ptr pin_node) noexcept
    : graph_edge{c, pout_node->outputs()[pout], pin_node->inputs()[pin],
                 std::move(pout_node), std::move(pin_node)}
{
}

graph_edge::graph_edge(
    connection c, outlet_ptr pout, inlet_ptr pin, node_ptr pout_node,
    node_ptr pin_node) noexcept
    : con{c}
    , out{std::move(pout)}
    , in{std::move(pin)}
    , out_node{std::move(pout_node)}
    , in_node{std::move(pin_node)}
{
  assert(out_node);
  assert(in_node);
}

void graph_edge::init() noexcept
{
  if (in && out)
  {
    out->connect(this);
    in->connect(this);

    if (auto delay = con.target<delayed_glutton_connection>())
    {
      ossia::apply(init_delay_line{delay->buffer}, out->data);
    }
    else if (auto sdelay = con.target<delayed_strict_connection>())
    {
      ossia::apply(init_delay_line{sdelay->buffer}, out->data);
    }
  }
}

graph_edge::~graph_edge()
{
  clear();
}

void graph_edge::clear() noexcept
{
  if (in && out)
  {
    out->disconnect(this);
    in->disconnect(this);
  }

  con = connection{};
  out = {};
  in = {};
  out_node.reset();
  in_node.reset();
}

graph_node::~graph_node()
{
  for (auto inl : m_inlets)
    delete inl;
  for (auto outl : m_outlets)
    delete outl;
}

void graph_node::prepare(const execution_state& st) const noexcept
{
}
graph_node::graph_node() noexcept
{
}

bool graph_node::consumes(const execution_state&) const noexcept
{
  return false;
}

void graph_node::run(token_request t, exec_state_facade) noexcept
{
}

std::string graph_node::label() const noexcept
{
  return {};
}

bool graph_node::has_port_inputs() const noexcept
{
  return ossia::any_of(inputs(), [](const inlet_ptr& inlet) {
    return !inlet->sources.empty();
  });
}

bool graph_node::has_global_inputs() const noexcept
{
  return ossia::any_of(inputs(), [&](const inlet_ptr& inlet) {
    return (inlet->scope & port::scope_t::global) && bool(inlet->address);
  });
}

bool graph_node::has_local_inputs(const execution_state& st) const noexcept
{
  return ossia::any_of(inputs(), [&](const inlet_ptr& inlet) {
    if (inlet->scope & port::scope_t::local)
    {
      bool b = false;

      // TODO optimize by stopping when found
      apply_to_destination(
          inlet->address, st.exec_devices(),
          [&](ossia::net::parameter_base* addr, bool) {
            if (!b || st.in_local_scope(*addr))
              b = true;
          }, do_nothing_for_nodes{});

      if (b)
        return true;

      if (consumes(st))
        return true;
    }
    return false;
  });
}

void graph_node::clear() noexcept
{
  for (auto inl : m_inlets)
  {
    for (ossia::graph_edge* e : inl->sources)
    {
      e->clear();
    }
  }
  for (auto outl : m_outlets)
  {
    for (ossia::graph_edge* e : outl->targets)
    {
      e->clear();
    }
  }

  for (auto inl : m_inlets)
  {
    delete inl;
  }
  for (auto outl : m_outlets)
  {
    delete outl;
  }
  m_inlets.clear();
  m_outlets.clear();
}

void graph_node::request(token_request req) noexcept
{
  /*
  if(std::abs(req.date - req.prev_date) > 1000)
  {
    std::cerr << typeid(this).name() << " " << req.prev_date << " => " <<
  req.date << std::endl;
  }
  */

  requested_tokens.push_back(std::move(req));
}

void graph_node::all_notes_off() noexcept
{
}

nonowning_graph_node::~nonowning_graph_node()
{
  m_inlets.clear();
  m_outlets.clear();
}

void nonowning_graph_node::clear() noexcept
{
}
}
