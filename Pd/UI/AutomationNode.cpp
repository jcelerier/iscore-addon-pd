#include "AutomationNode.hpp"
#include <Pd/UI/NodeItem.hpp>
#include <Curve/CurveModel.hpp>
#include <Curve/Segment/CurveSegmentData.hpp>
#include <Device/Protocol/DeviceInterface.hpp>
#include <Device/Protocol/DeviceList.hpp>
#include <Device/Protocol/DeviceSettings.hpp>

#include <ossia/editor/curve/curve.hpp>
#include <ossia/editor/curve/curve_segment.hpp>
#include <ossia/editor/automation/curve_value_visitor.hpp>
#include <ossia/editor/value/value.hpp>
#include <ossia/network/base/address.hpp>
#include <ossia/network/base/node.hpp>
#include <Engine/CurveConversion.hpp>
#include <Engine/Executor/ProcessComponent.hpp>
#include <Engine/Protocols/OSSIADevice.hpp>
#include <Engine/iscore2OSSIA.hpp>
#include <State/Address.hpp>
#include <iscore/document/DocumentInterface.hpp>
#include <iscore/plugins/customfactory/StringFactoryKey.hpp>

#include <ossia/editor/dataspace/dataspace_visitors.hpp> // temporary
#include <Engine/Executor/DocumentPlugin.hpp>
#include <Engine/Executor/ExecutorContext.hpp>
#include <Explorer/DocumentPlugin/DeviceDocumentPlugin.hpp>
namespace Dataflow
{


AutomationComponent::AutomationComponent(
    Automation::ProcessModel& autom,
    DocumentPlugin& doc,
    const Id<iscore::Component>& id,
    QObject* parent_obj):
  ProcessComponent_T<Automation::ProcessModel>{autom, doc, id, "AutomationComponent", parent_obj}
{
  connect(&autom, &Automation::ProcessModel::addressChanged,
          this, [=] { this->outletsChanged(); });
  auto itm = new NodeItem{*this};
  itm->setParentItem(doc.window.view.contentItem());
  itm->setPosition(QPointF(100, 100));
  ui = itm;
}






ExecComponent::ExecComponent(
    Engine::Execution::ConstraintComponent& parentConstraint,
    Automation::ProcessModel& element,
    const Dataflow::DocumentPlugin& df,
    const ::Engine::Execution::Context& ctx,
    const Id<iscore::Component>& id,
    QObject* parent)
  : Engine::Execution::
    ProcessComponent_T<Automation::ProcessModel, ossia::automation>{
      parentConstraint,
      element,
      ctx,
      id, "Executor::AutomationComponent", parent}
  , m_df{df}
{
  m_ossia_process = std::make_shared<ossia::automation>();

  con(element, &Automation::ProcessModel::addressChanged,
      this, [this] (const auto&) { this->recompute(); });
  con(element, &Automation::ProcessModel::minChanged,
      this, [this] (const auto&) { this->recompute(); });
  con(element, &Automation::ProcessModel::maxChanged,
      this, [this] (const auto&) { this->recompute(); });

  // TODO the tween case will reset the "running" value,
  // so it may not work perfectly.
  con(element, &Automation::ProcessModel::tweenChanged,
      this, [this] (const auto&) { this->recompute(); });
  con(element, &Automation::ProcessModel::curveChanged,
      this, [this] () { this->recompute(); });

  recompute();
}

void ExecComponent::recompute()
{
  auto dest = Engine::iscore_to_ossia::makeDestination(
                system().devices.list(),
                process().address());

  if (dest)
  {
    auto& d = *dest;
    auto addressType = d.address().get_value_type();

    auto curve = process().tween()
                 ? on_curveChanged(addressType, d)
                 : on_curveChanged(addressType, {});

    if (curve)
    {
      system().executionQueue.enqueue(
            [proc=std::dynamic_pointer_cast<ossia::automation>(m_ossia_process)
            ,curve
            ,d_=d]
      {
        proc->set_destination(std::move(d_));
        proc->set_behavior(curve);
      });
      return;
    }
  }
  else
  {

  }

  m_ossia_process =
      std::make_shared<ossia::node_process>(m_df.execGraph, node);

  /*
  connectCables(
        iscore::component<Dataflow::ProcessComponent>(element.components()),
        m_df);
*/}

template <typename Y_T>
std::shared_ptr<ossia::curve_abstract>
ExecComponent::on_curveChanged_impl(const optional<ossia::Destination>& d)
{
  using namespace ossia;

  const double min = process().min();
  const double max = process().max();

  auto scale_x = [](double val) -> double { return val; };
  auto scale_y = [=](double val) -> Y_T { return val * (max - min) + min; };

  auto segt_data = process().curve().sortedSegments();
  if (segt_data.size() != 0)
  {
    return Engine::iscore_to_ossia::curve<double, Y_T>(
          scale_x, scale_y, segt_data, d);
  }
  else
  {
    return {};
  }
}

std::shared_ptr<ossia::curve_abstract>
ExecComponent::on_curveChanged(
    ossia::val_type type,
    const optional<ossia::Destination>& d)
{
  switch (type)
  {
    case ossia::val_type::INT:
      return on_curveChanged_impl<int>(d);
    case ossia::val_type::FLOAT:
      return on_curveChanged_impl<float>(d);
    case ossia::val_type::TUPLE:
    case ossia::val_type::VEC2F:
    case ossia::val_type::VEC3F:
    case ossia::val_type::VEC4F:
      return on_curveChanged_impl<float>(d);
    default:
      qDebug() << "Unsupported curve type: " << (int)type;
      ISCORE_TODO;
  }

  return {};
}

void AutomationGraphNode::run(ossia::execution_state& e)
{
  ossia::val_type driven_type{ossia::val_type::FLOAT};

  auto base_curve = m_curve.get();
  if (!base_curve)
  {
    return;
  }

  ossia::value v;
  auto t = base_curve->get_type();
  if (t.first == ossia::curve_segment_type::DOUBLE)
  {
    switch (driven_type)
    {
      case ossia::val_type::FLOAT:
      case ossia::val_type::INT:
      case ossia::val_type::BOOL:
      {
        switch (t.second)
        {
          case ossia::curve_segment_type::FLOAT:
          {
            v = float{static_cast<ossia::curve<double, float>*>(base_curve)
                ->value_at(position())};
            break;
          }
          case ossia::curve_segment_type::INT:
          {
            v = int32_t{static_cast<ossia::curve<double, int>*>(base_curve)
                ->value_at(position())};
            break;
          }
          case ossia::curve_segment_type::BOOL:
          {
            v = bool{static_cast<ossia::curve<double, bool>*>(base_curve)
                ->value_at(position())};
            break;
          }
          case ossia::curve_segment_type::DOUBLE:
          {
            return; // TODO
          }
          case ossia::curve_segment_type::ANY:
          {
            // TODO we need a specific handling for destination.
            v = static_cast<ossia::constant_curve*>(base_curve)->value();
            break;
          }
        }
        break;
      }
      case ossia::val_type::VEC2F:
      {
        v = ossia::detail::make_filled_vec<2>(base_curve, t.second, position());
        break;
      }
      case ossia::val_type::VEC3F:
      {
        v = ossia::detail::make_filled_vec<3>(base_curve, t.second, position());
        break;
      }
      case ossia::val_type::VEC4F:
      {
        v = ossia::detail::make_filled_vec<4>(base_curve, t.second, position());
        break;
      }
      case ossia::val_type::TUPLE:
      {
        return; // TODO
      }

      default:
        break;
    }
  }

  ossia::value_port* vp = m_outlets[0]->data.target<ossia::value_port>();
  vp->data.push_back(v);
}

}

