#include "NodeItem.hpp"

namespace Dataflow
{

NodeItem::NodeItem(Dataflow::ProcessComponent& p):
  m_proc{p}
{
  this->setFlag(QQuickItem::ItemHasContents, true);
  QObject::connect(&m_proc, &Dataflow::ProcessComponent::inletsChanged,
                   this, &NodeItem::recreate);
  QObject::connect(&m_proc, &Dataflow::ProcessComponent::outletsChanged,
                   this, &NodeItem::recreate);

  this->setAntialiasing(true);
  this->setAcceptedMouseButtons(Qt::LeftButton);
  recreate();
}

NodeItem::~NodeItem()
{
  aboutToDelete();
}

void NodeItem::recreate()
{
  setWidth(std::max((std::size_t)30, 20 * std::max(m_proc.inlets().size(), m_proc.outlets().size())));
  setHeight(30);
  update();
}

void NodeItem::paint(QPainter* painter)
{
  painter->setPen(QPen(Qt::darkGray, 3));
  painter->setBrush(QBrush(QColor::fromHsl(0, 0, 240)));
  painter->drawRoundedRect(QRectF{xMv(), yMv(), objectW(), objectH()}, 5, 5);

  // Port input and output
  painter->setPen(QPen(QColor::fromHsl(0, 0, 220), 3));
  painter->setBrush(QBrush(Qt::gray));
  {
    for(std::size_t i = 0; i < m_proc.inlets().size(); i++)
    {
      painter->drawEllipse(inletPosition(i), 5, 5);
    }
  }
  {
    for(std::size_t i = 0; i < m_proc.outlets().size(); i++)
    {
      painter->drawEllipse(outletPosition(i), 5, 5);
    }
  }

  // Dependency input and output
  painter->drawEllipse(depInlet(), 5, 5);
  painter->drawEllipse(depOutlet(), 5, 5);
}

QPointF NodeItem::depInlet() const
{ return QPointF{0., height()/2.}; }

QPointF NodeItem::depOutlet() const
{ return QPointF{width(), height()/2.}; }

QPointF NodeItem::inletPosition(int i) const
{
  const double spc = objectW() / m_proc.inlets().size();
  return QPointF{xMv() + i * spc, yMv()};
}

QPointF NodeItem::outletPosition(int i) const
{
  const double spc = objectW() / m_proc.outlets().size();
  return QPointF{xMv() + i * spc, objectH()};
}

void NodeItem::mousePressEvent(QMouseEvent* ev)
{
  QQuickPaintedItem::mousePressEvent(ev);
  ev->accept();
}

void NodeItem::mouseMoveEvent(QMouseEvent* ev)
{
  QQuickPaintedItem::mouseMoveEvent(ev);
  setPosition(ev->pos());
  ev->accept();
}

void NodeItem::mouseReleaseEvent(QMouseEvent* ev)
{
  QQuickPaintedItem::mouseReleaseEvent(ev);
  ev->accept();
}

void NodeItem::hoverEnterEvent(QHoverEvent* ev)
{
  QQuickPaintedItem::hoverEnterEvent(ev);
  ev->accept();
}

void NodeItem::hoverMoveEvent(QHoverEvent* ev)
{
  QQuickPaintedItem::hoverMoveEvent(ev);
  ev->accept();
}

void NodeItem::hoverLeaveEvent(QHoverEvent* ev)
{
  QQuickPaintedItem::hoverLeaveEvent(ev);
  ev->accept();
}

void NodeItem::dragEnterEvent(QDragEnterEvent* ev)
{
  QQuickPaintedItem::dragEnterEvent(ev);
  ev->accept();
}

void NodeItem::dragLeaveEvent(QDragLeaveEvent* ev)
{
  QQuickPaintedItem::dragLeaveEvent(ev);
  ev->accept();
}

void NodeItem::dragMoveEvent(QDragMoveEvent* ev)
{
  QQuickPaintedItem::dragMoveEvent(ev);
  ev->accept();
}

void NodeItem::mouseDoubleClickEvent(QMouseEvent* ev)
{
  // Dialog to set addresses
  QQuickPaintedItem::mouseDoubleClickEvent(ev);

  // Also allow to drag addresses on ports
  ev->accept();
}




CableItem::CableItem(Process::Cable& p):
  m_cable{p}
{
  this->setFlag(QQuickItem::ItemHasContents, true);
  connect(&m_cable, &Process::Cable::inletChanged,
          this, [=] { recreate(); });
  connect(&m_cable, &Process::Cable::outletChanged,
          this, [=] { recreate(); });
  connect(&m_cable, &Process::Cable::sourceChanged,
          this, [=] { recreate(); });
  connect(&m_cable, &Process::Cable::sinkChanged,
          this, [=] { recreate(); });

  recreate();
}

void CableItem::recreate()
{
  for(auto& con : cons)
    disconnect(con);
  cons.clear();

  if(source)
  {
    cons.push_back(connect(source, &NodeItem::xChanged, this, [=] { update(); }));
    cons.push_back(connect(source, &NodeItem::yChanged, this, [=] { update(); }));
    cons.push_back(connect(source, &NodeItem::aboutToDelete, this, [=] { source = nullptr; recreate(); }));
  }
  if(target)
  {
    cons.push_back(connect(target, &NodeItem::xChanged, this, [=] { update(); }));
    cons.push_back(connect(target, &NodeItem::yChanged, this, [=] { update(); }));
    cons.push_back(connect(target, &NodeItem::aboutToDelete, this, [=] { target = nullptr; recreate(); }));
  }
  update();
}

void CableItem::paint(QPainter* painter)
{
  if(source && target)
  {
    if(m_cable.inlet() && m_cable.outlet())
    {
      painter->drawLine(source->position() + source->outletPosition(*m_cable.outlet()),
                        target->position() + target->inletPosition(*m_cable.inlet()));
    }
    else
    {
      painter->drawLine(source->position() + source->depOutlet(),
                        target->position() + target->depInlet());
    }
  }
  else if(source)
  {
    if(m_cable.outlet())
      painter->drawLine(source->position() + source->outletPosition(*m_cable.outlet()), m_curPos);
    else
      painter->drawLine(source->position() + source->depOutlet(), m_curPos);
  }
  else if(target)
  {
    if(m_cable.inlet())
      painter->drawLine(target->position() + target->inletPosition(*m_cable.inlet()), m_curPos);
    else
      painter->drawLine(target->position() + target->depInlet(), m_curPos);
  }
}

void CableItem::mousePressEvent(QMouseEvent* ev)
{
  QQuickPaintedItem::mousePressEvent(ev);
  m_clickPos = ev->pos();
  ev->accept();
}

void CableItem::mouseMoveEvent(QMouseEvent* ev)
{
  QQuickPaintedItem::mouseMoveEvent(ev);
  if(QLineF(m_clickPos, ev->pos()).length() > 5)
    m_curPos = ev->pos();
  else
    m_curPos = m_clickPos;
  ev->accept();
}

void CableItem::mouseReleaseEvent(QMouseEvent* ev)
{
  QQuickPaintedItem::mouseReleaseEvent(ev);
  ev->accept();
}

void CableItem::hoverEnterEvent(QHoverEvent* ev)
{
  QQuickPaintedItem::hoverEnterEvent(ev);
  ev->accept();
}

void CableItem::hoverMoveEvent(QHoverEvent* ev)
{
  QQuickPaintedItem::hoverMoveEvent(ev);
  ev->accept();
}

void CableItem::hoverLeaveEvent(QHoverEvent* ev)
{
  QQuickPaintedItem::hoverLeaveEvent(ev);
  ev->accept();
}

void CableItem::dragEnterEvent(QDragEnterEvent* ev)
{
  QQuickPaintedItem::dragEnterEvent(ev);
  ev->accept();
}

void CableItem::dragLeaveEvent(QDragLeaveEvent* ev)
{
  QQuickPaintedItem::dragLeaveEvent(ev);
  ev->accept();
}

void CableItem::dragMoveEvent(QDragMoveEvent* ev)
{
  QQuickPaintedItem::dragMoveEvent(ev);
  ev->accept();
}

void CableItem::mouseDoubleClickEvent(QMouseEvent* ev)
{
  QQuickPaintedItem::mouseDoubleClickEvent(ev);
  ev->accept();
}

}
