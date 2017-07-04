
#include <Pd/Inspector/PdInspectorWidget.hpp>

namespace Pd
{


PdWidget::PdWidget(const Pd::ProcessModel& object, const iscore::DocumentContext& context, QWidget* parent)
  : InspectorWidgetDelegate_T{object, parent}
  , m_lay{this}
  , m_dfWidg{context, object, this}
{
  setObjectName("PdInspectorWidget");
  setParent(parent);

  m_lay.addWidget(&m_dfWidg);
  m_lay.addWidget(&m_ledit);
  m_ledit.setText(object.script());
  m_lay.addStretch(1);

  con(m_ledit, &QLineEdit::editingFinished,
      this, [&] {
    CommandDispatcher<> cmd{context.commandStack};
    cmd.submitCommand<EditPdPath>(object, m_ledit.text());
  });

  con(object, &ProcessModel::scriptChanged,
      this, &PdWidget::on_patchChange);
}


void PdWidget::on_patchChange(const QString& newText)
{
  m_ledit.setText(newText);

}

}
