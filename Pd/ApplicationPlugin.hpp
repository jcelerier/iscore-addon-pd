#pragma once
#include <iscore/plugins/application/GUIApplicationPlugin.hpp>
class QAction;
namespace Dataflow
{
class ApplicationPlugin final :
    public QObject,
    public iscore::GUIApplicationPlugin
{
public:
  ApplicationPlugin(
      const iscore::GUIApplicationContext& app);

private:
  void on_documentChanged(
      iscore::Document* olddoc,
      iscore::Document* newdoc) override;

  iscore::GUIElements makeGUIElements() override;

  void on_initDocument(iscore::Document& doc) override;

  QAction* m_showScene{};

};
}