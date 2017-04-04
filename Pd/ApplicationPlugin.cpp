#include <Pd/ApplicationPlugin.hpp>
#include <Pd/DocumentPlugin.hpp>
#include <Scenario/Document/ScenarioDocument/ScenarioDocumentModel.hpp>
#include <core/document/Document.hpp>
#include <core/document/DocumentModel.hpp>
#include <iscore/actions/ActionManager.hpp>
#include <iscore/actions/MenuManager.hpp>
#include <iscore/tools/IdentifierGeneration.hpp>
#include <Scenario/Application/ScenarioActions.hpp>
#include <QAction>

ISCORE_DECLARE_ACTION(GraphView, "&Graph view", Dataflow, Qt::ALT + Qt::SHIFT + Qt::Key_G)
namespace Dataflow
{

ApplicationPlugin::ApplicationPlugin(
    const iscore::GUIApplicationContext& app):
  GUIApplicationPlugin {app}
{
  m_showScene = new QAction{this};
  connect(m_showScene, &QAction::triggered, this, [this] {
    auto doc = this->currentDocument();
    if(doc)
    {
      auto& plug = doc->context().plugin<DocumentPlugin>();
      plug.reload();
      plug.window.window.show();
    }
  });
}

iscore::GUIElements ApplicationPlugin::makeGUIElements()
{
  GUIElements e;
  auto& scenario_doc_cond = context.actions.condition<
      iscore::EnableWhenDocumentIs<Scenario::ScenarioDocumentModel>>();

  auto& actions = e.actions;
  actions.add<Actions::GraphView>(m_showScene);

  iscore::Menu& menu = context.menus.get().at(iscore::Menus::View());
  menu.menu()->addAction(m_showScene);

  return e;
}

void ApplicationPlugin::on_initDocument(iscore::Document& doc)
{
  doc.model().addPluginModel(new DocumentPlugin{
      doc.context(), getStrongId(doc.model().pluginModels()), &doc.model()});
}

void ApplicationPlugin::on_documentChanged(
    iscore::Document* olddoc,
    iscore::Document* newdoc)
{
  if (olddoc)
  {
    auto& doc_plugin = olddoc->context().plugin<DocumentPlugin>();

    doc_plugin.window.window.hide();
  }
}
}
