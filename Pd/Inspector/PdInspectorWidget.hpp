#pragma once
#include <Pd/PdProcess.hpp>
#include <Pd/Commands/EditPd.hpp>
#include <Dataflow/Inspector/DataflowWidget.hpp>
#include <Process/Inspector/ProcessInspectorWidgetDelegate.hpp>
#include <Process/Inspector/ProcessInspectorWidgetDelegateFactory.hpp>
#include <iscore/command/Dispatchers/CommandDispatcher.hpp>
#include <QVBoxLayout>
#include <QLineEdit>
#include <iscore/widgets/MarginLess.hpp>
#include <Dataflow/Inspector/DataflowWidget.hpp>
namespace Pd
{

class PdWidget final
    : public Process::InspectorWidgetDelegate_T<Pd::ProcessModel>
{
  Q_OBJECT
public:
  explicit PdWidget(
      const Pd::ProcessModel& object,
      const iscore::DocumentContext& context,
      QWidget* parent);

signals:
  void pressed();

private:
  void on_patchChange(const QString& newText);

  iscore::MarginLess<QVBoxLayout> m_lay;
  Dataflow::DataflowWidget m_dfWidg;
  QLineEdit m_ledit;
};


class InspectorFactory final :
        public Process::InspectorWidgetDelegateFactory_T<ProcessModel, PdWidget>
{
        ISCORE_CONCRETE("ac3f1317-1381-4a19-a10f-2e7ae711bf58")
};
}
