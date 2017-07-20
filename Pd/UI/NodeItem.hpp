#pragma once
#include <QObject>
#include <QQuickPaintedItem>
#include <Process/Dataflow/DataflowProcess.hpp>
#include <QPainter>
namespace Dataflow
{
struct NodeItem: public QQuickPaintedItem
{
    Process::DataflowProcess& m_proc;
  public:
    NodeItem(Process::DataflowProcess& p):
      m_proc{p}
    {
      this->setFlag(QQuickItem::ItemHasContents, true);
      QObject::connect(&m_proc, &Process::DataflowProcess::inletsChanged, this, &NodeItem::recreate);
      QObject::connect(&m_proc, &Process::DataflowProcess::outletsChanged, this, &NodeItem::recreate);

      recreate();
    }

    void recreate()
    {
      setWidth(20 * std::max(m_proc.inlets().size(), m_proc.outlets().size()));
      setHeight(30);
      update();
    }

    void paint(QPainter* painter) override
    {
      painter->setPen(QPen(Qt::darkGray, 3));
      painter->setBrush(QBrush(QColor::fromHsl(0, 0, 240)));
      painter->drawRoundedRect(boundingRect(), 5, 5);


      // Port input and output
      painter->setPen(QPen(QColor::fromHsl(0, 0, 220), 3));
      painter->setBrush(QBrush(Qt::gray));
      {
        double top = width() / m_proc.inlets().size();
        for(int i = 0; i < m_proc.inlets().size(); i++)
        {
          double x = i * top;
          double y = 0;
          painter->drawEllipse(QPointF{x, y}, 5, 5);
        }
      }
      {
        double bottom = width() / m_proc.outlets().size();
        for(int i = 0; i < m_proc.outlets().size(); i++)
        {
          double x = i * bottom;
          double y = height();
          painter->drawEllipse(QPointF{x, y}, 5, 5);
        }
      }

      // Dependency input and output
      painter->drawEllipse(QPointF{0., height()/2.}, 5, 5);
      painter->drawEllipse(QPointF{width(), height()/2.}, 5, 5);
    }

  private:
    void mousePressEvent(QMouseEvent* ev) override
    {
      QQuickPaintedItem::mousePressEvent(ev);
      ev->accept();
    }
    void mouseMoveEvent(QMouseEvent* ev) override
    {
      QQuickPaintedItem::mouseMoveEvent(ev);
      setPosition(ev->pos());
      ev->accept();
    }
    void mouseReleaseEvent(QMouseEvent* ev) override
    {
      QQuickPaintedItem::mouseReleaseEvent(ev);
      ev->accept();
    }

    void hoverEnterEvent(QHoverEvent* ev) override
    {
      QQuickPaintedItem::hoverEnterEvent(ev);
      ev->accept();
    }
    void hoverMoveEvent(QHoverEvent* ev) override
    {
      QQuickPaintedItem::hoverMoveEvent(ev);
      ev->accept();
    }
    void hoverLeaveEvent(QHoverEvent* ev) override
    {
      QQuickPaintedItem::hoverLeaveEvent(ev);
      ev->accept();
    }

    void dragEnterEvent(QDragEnterEvent* ev) override
    {
      QQuickPaintedItem::dragEnterEvent(ev);
      ev->accept();
    }
    void dragLeaveEvent(QDragLeaveEvent* ev) override
    {
      QQuickPaintedItem::dragLeaveEvent(ev);
      ev->accept();
    }
    void dragMoveEvent(QDragMoveEvent* ev) override
    {
      QQuickPaintedItem::dragMoveEvent(ev);
      ev->accept();
    }

    void mouseDoubleClickEvent(QMouseEvent* ev) override
    {
      // Dialog to set addresses
      QQuickPaintedItem::mouseDoubleClickEvent(ev);

      // Also allow to drag addresses on ports
      ev->accept();
    }
};
}
