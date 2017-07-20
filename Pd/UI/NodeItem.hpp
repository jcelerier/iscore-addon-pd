#pragma once
#include <QObject>
#include <QQuickPaintedItem>
#include <Process/Dataflow/DataflowProcess.hpp>
#include <Pd/UI/ScenarioNode.hpp>
#include <QPainter>
namespace Dataflow
{
struct NodeItem: public QQuickPaintedItem
{
    Q_OBJECT
    Dataflow::ProcessComponent& m_proc;
  public:
    NodeItem(Dataflow::ProcessComponent& p);
    ~NodeItem();

    void recreate();
    void paint(QPainter* painter) override;

    QPointF depInlet() const;
    QPointF depOutlet() const;

    QPointF inletPosition(int i) const;
    QPointF outletPosition(int i) const;
    double xMv() const { return 4.; }
    double yMv() const { return 4.; }
    double objectX() const { return x()+4.; }
    double objectY() const { return y()+4.; }
    double objectW() const { return width()-8.; }
    double objectH() const { return height()-8.; }

  signals:
    void aboutToDelete();

  private:
    void mousePressEvent(QMouseEvent* ev) override;
    void mouseMoveEvent(QMouseEvent* ev) override;
    void mouseReleaseEvent(QMouseEvent* ev) override;

    void hoverEnterEvent(QHoverEvent* ev) override;
    void hoverMoveEvent(QHoverEvent* ev) override;
    void hoverLeaveEvent(QHoverEvent* ev) override;

    void dragEnterEvent(QDragEnterEvent* ev) override;
    void dragLeaveEvent(QDragLeaveEvent* ev) override;
    void dragMoveEvent(QDragMoveEvent* ev) override;

    void mouseDoubleClickEvent(QMouseEvent* ev) override;
};

struct CableItem: public QQuickPaintedItem
{
  public:
    Process::Cable& m_cable;
    NodeItem* source{};
    NodeItem* target{};
    CableItem(Process::Cable& p);

    void recreate();
    void paint(QPainter* painter) override;

  private:
    std::vector<QMetaObject::Connection> cons;
    QPointF m_clickPos{};
    QPointF m_curPos{};
    void mousePressEvent(QMouseEvent* ev) override;
    void mouseMoveEvent(QMouseEvent* ev) override;
    void mouseReleaseEvent(QMouseEvent* ev) override;

    void hoverEnterEvent(QHoverEvent* ev) override;
    void hoverMoveEvent(QHoverEvent* ev) override;
    void hoverLeaveEvent(QHoverEvent* ev) override;

    void dragEnterEvent(QDragEnterEvent* ev) override;
    void dragLeaveEvent(QDragLeaveEvent* ev) override;
    void dragMoveEvent(QDragMoveEvent* ev) override;

    void mouseDoubleClickEvent(QMouseEvent* ev) override;
};
}
