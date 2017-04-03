#pragma once
#include <Pd/PdProcess.hpp>
#include <Pd/Commands/EditPd.hpp>
#include <Pd/Inspector/DataflowWidget.hpp>
#include <Process/Inspector/ProcessInspectorWidgetDelegate.hpp>
#include <Process/Inspector/ProcessInspectorWidgetDelegateFactory.hpp>
#include <iscore/command/Dispatchers/CommandDispatcher.hpp>
#include <QVBoxLayout>
#include <QLineEdit>
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
      QWidget* parent)
    : InspectorWidgetDelegate_T{object, parent}
    , m_lay{this}
    , m_dfWidg{context, object, this}
  {
    setObjectName("PdInspectorWidget");
    setParent(parent);

    m_lay.addWidget(&m_dfWidg);
    m_lay.addWidget(&m_ledit);
    m_lay.addStretch(1);

    con(m_ledit, &QLineEdit::editingFinished,
        this, [&] {
      CommandDispatcher<> cmd{context.commandStack};
      cmd.submitCommand<EditPdPath>(object, m_ledit.text());
    });

    con(object, &ProcessModel::scriptChanged,
        this, &PdWidget::on_patchChange);
  }

signals:
  void pressed();

private:
  void on_patchChange(const QString& newText)
  {
    m_ledit.setText(newText);

  }

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
