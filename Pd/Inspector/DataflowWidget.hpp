#pragma once
#include <Pd/DataflowProcess.hpp>
#include <iscore/widgets/ClearLayout.hpp>
#include <iscore/widgets/TextLabel.hpp>
#include <iscore/document/DocumentContext.hpp>
#include <Explorer/Explorer/DeviceExplorerModel.hpp>
#include <Explorer/Widgets/AddressAccessorEditWidget.hpp>
#include <Explorer/DocumentPlugin/DeviceDocumentPlugin.hpp>
#include <Pd/Commands/EditPort.hpp>
#include <iscore/command/Dispatchers/CommandDispatcher.hpp>
#include <iscore/widgets/MarginLess.hpp>
#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>

namespace Dataflow
{
class PortWidget :
    public QWidget
{
  Q_OBJECT
  iscore::MarginLess<QHBoxLayout> m_lay;
  QPushButton m_remove;
public:
  PortWidget(Explorer::DeviceExplorerModel& model, QWidget* parent)
    : QWidget{parent}
    , m_lay{this}
    , m_remove{tr("X"), this}
    , accessor{model, this}
  {
    m_lay.addWidget(&accessor);
    m_lay.addWidget(&m_remove);

    con(m_remove, &QPushButton::clicked, this, &PortWidget::removeMe);
  }

  Explorer::AddressAccessorEditWidget accessor;

signals:
  void removeMe();
};

class DataflowWidget:
    public QWidget
{
  Q_OBJECT
  const Dataflow::ProcessModel& m_proc;
  Explorer::DeviceExplorerModel& m_explorer;
  CommandDispatcher<> m_disp;
  iscore::MarginLess<QVBoxLayout> m_lay;
  QPushButton* m_addInlet{};
  QPushButton* m_addOutlet{};
  std::vector<PortWidget*> m_inlets;
  std::vector<PortWidget*> m_outlets;
public:
  DataflowWidget(
      const iscore::DocumentContext& doc,
      const Dataflow::ProcessModel& proc,
      QWidget* parent)
    : QWidget{parent}
    , m_proc{proc}
    , m_explorer{doc.plugin<Explorer::DeviceDocumentPlugin>().explorer()}
    , m_disp{doc.commandStack}
    , m_lay{this}
  {
    setLayout(&m_lay);

    reinit();
    con(proc, &ProcessModel::inletsChanged, this, &DataflowWidget::reinit);
    con(proc, &ProcessModel::outletsChanged, this, &DataflowWidget::reinit);

  }

  void reinit()
  {
    iscore::clearLayout(&m_lay);
    m_inlets.clear();
    m_outlets.clear();
    m_addInlet = nullptr;
    m_addOutlet = nullptr;

    m_lay.addWidget(new TextLabel{tr("Inlets"), this});

    std::size_t i{};
    for(auto& inlet : m_proc.inlets())
    {
      auto widg = new PortWidget{m_explorer, this};
      widg->accessor.setAddress(inlet.address);

      m_lay.addWidget(widg);
      m_inlets.push_back(widg);

      con(widg->accessor, &Explorer::AddressAccessorEditWidget::addressChanged,
          this, [=] (const Device::FullAddressAccessorSettings& as) {

        auto cmd = new EditPort{m_proc, as.address, i, true};
        m_disp.submitCommand(cmd);
      }, Qt::QueuedConnection);

      connect(widg, &PortWidget::removeMe, this, [=] {
        m_disp.submitCommand<RemovePort>(m_proc, i, true);
      }, Qt::QueuedConnection);

      i++;
    }

    m_addInlet = new QPushButton{tr("Add inlet"), this};
    connect(m_addInlet, &QPushButton::clicked, this, [&] {
      m_disp.submitCommand<AddPort>(m_proc, true);
    }, Qt::QueuedConnection);
    m_lay.addWidget(m_addInlet);

    m_lay.addWidget(new TextLabel{tr("Outlets"), this});

    i = 0;
    for(auto& outlet : m_proc.outlets())
    {
      auto widg = new PortWidget{m_explorer, this};
      widg->accessor.setAddress(outlet.address);

      m_lay.addWidget(widg);
      m_outlets.push_back(widg);

      con(widg->accessor, &Explorer::AddressAccessorEditWidget::addressChanged,
          this, [=] (const Device::FullAddressAccessorSettings& as) {

        auto cmd = new EditPort{m_proc, as.address, i, false};
        m_disp.submitCommand(cmd);
      }, Qt::QueuedConnection);

      connect(widg, &PortWidget::removeMe, this, [=] {
        m_disp.submitCommand<RemovePort>(m_proc, i, false);
      }, Qt::QueuedConnection);

      i++;
    }

    m_addOutlet = new QPushButton{tr("Add outlet"), this};
    connect(m_addOutlet, &QPushButton::clicked, this, [&] {
      m_disp.submitCommand<AddPort>(m_proc, false);
    });
    m_lay.addWidget(m_addOutlet);
  }

};
}
