#pragma once
#include <Process/Dataflow/DataflowObjects.hpp>
#include <QQuickPaintedItem>
#include <QPainter>
namespace Dataflow
{

class SliderUI final : public QQuickPaintedItem
{
    Q_OBJECT
public:
    SliderUI();

signals:
    void valueChanged(double);

private:
    void paint(QPainter *painter) override;

    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

    void changeValue(double pos);
    double m_value{};
};

class Slider final :
        public Process::Node
{
    Q_OBJECT
public:
    Slider(const DocumentPlugin& doc, Id<Node> c, QObject* parent);

    void setVolume(double v);

signals:
    void volumeChanged(double);
private:

    QString getText() const override;
    std::size_t audioInlets() const override;
    std::size_t messageInlets() const override;
    std::size_t midiInlets() const override;
    std::size_t audioOutlets() const override;
    std::size_t messageOutlets() const override;
    std::size_t midiOutlets() const override;
    std::vector<Process::Port> inlets() const override;
    std::vector<Process::Port> outlets() const override;
    std::vector<Id<Process::Cable> > cables() const override;
    void addCable(Id<Process::Cable> c) override;
    void removeCable(Id<Process::Cable> c) override;

    double m_volume{};
    std::vector<Id<Process::Cable>> m_cables;
};

}
