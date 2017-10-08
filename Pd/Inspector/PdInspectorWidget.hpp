#pragma once
#include <Pd/PdProcess.hpp>
#include <Pd/Commands/EditPd.hpp>
#include <Dataflow/Inspector/DataflowWidget.hpp>
#include <Process/Inspector/ProcessInspectorWidgetDelegate.hpp>
#include <Process/Inspector/ProcessInspectorWidgetDelegateFactory.hpp>
#include <score/command/Dispatchers/CommandDispatcher.hpp>
#include <QVBoxLayout>
#include <QLineEdit>
#include <QFormLayout>
#include <score/widgets/MarginLess.hpp>
#include <Dataflow/Inspector/DataflowWidget.hpp>
#include <QSpinBox>
#include <QCheckBox>
namespace Pd
{

class PdWidget final
    : public Process::InspectorWidgetDelegate_T<Pd::ProcessModel>
{
  Q_OBJECT
public:
  explicit PdWidget(
      const Pd::ProcessModel& object,
      const score::DocumentContext& context,
      QWidget* parent);

signals:
  void pressed();
  void contextMenuRequested(QPoint);


private:
  void reinit();
  void on_patchChange(const QString& newText);

  CommandDispatcher<> m_disp;
  const Pd::ProcessModel& m_proc;
  Explorer::DeviceExplorerModel& m_explorer;


  QLineEdit m_ledit;
  score::MarginLess<QVBoxLayout> m_lay;
  QWidget m_portwidg;

  score::MarginLess<QFormLayout> m_sublay;
  QSpinBox m_audioIn, m_audioOut;
  QCheckBox m_midiIn, m_midiOut;
  std::vector<Dataflow::PortWidget*> m_inlets;
  std::vector<Dataflow::PortWidget*> m_outlets;
};


class InspectorFactory final :
        public Process::InspectorWidgetDelegateFactory_T<ProcessModel, PdWidget>
{
        SCORE_CONCRETE("ac3f1317-1381-4a19-a10f-2e7ae711bf58")
};
}
