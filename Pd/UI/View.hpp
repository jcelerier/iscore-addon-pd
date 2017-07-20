#pragma once
#include <QWidget>
#include <QQmlEngine>
#include <QQuickView>
#include <QHBoxLayout>
namespace Dataflow
{
class View: public QWidget
{
  public:
    View()
    {
      auto lay = new QHBoxLayout;
      this->setLayout(lay);

      auto widg = QWidget::createWindowContainer(&m_view, this);
      lay->addWidget(widg);

    }

  private:
    QQuickView m_view;

};
}
