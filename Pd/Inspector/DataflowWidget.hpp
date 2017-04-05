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
#include <QLineEdit>

namespace Dataflow
{
class PortWidget :
    public QWidget
{
  Q_OBJECT
  iscore::MarginLess<QHBoxLayout> m_lay;
  QPushButton m_remove;
public:
  PortWidget(Explorer::DeviceExplorerModel& model, QWidget* parent);

  QLineEdit localName;
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
      QWidget* parent);

  void reinit();

};
}
