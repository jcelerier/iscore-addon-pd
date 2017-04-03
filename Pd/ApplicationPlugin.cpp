#include <Pd/ApplicationPlugin.hpp>
#include <Pd/DocumentPlugin.hpp>
#include <core/document/Document.hpp>
#include <core/document/DocumentModel.hpp>
#include <iscore/tools/IdentifierGeneration.hpp>

namespace Dataflow
{

ApplicationPlugin::ApplicationPlugin(
    const iscore::GUIApplicationContext& app):
  GUIApplicationPlugin {app}
{

}

iscore::GUIElements ApplicationPlugin::makeGUIElements()
{
  return {};
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

    doc_plugin.window.view.hide();
  }
}
}
