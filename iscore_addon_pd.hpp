#pragma once
#include <QObject>
#include <iscore/plugins/qt_interfaces/PluginRequirements_QtInterface.hpp>
#include <iscore/plugins/qt_interfaces/FactoryInterface_QtInterface.hpp>
#include <iscore/plugins/qt_interfaces/FactoryFamily_QtInterface.hpp>
#include <iscore/plugins/qt_interfaces/CommandFactory_QtInterface.hpp>
#include <iscore/plugins/qt_interfaces/GUIApplicationPlugin_QtInterface.hpp>

class iscore_addon_pd final:
        public QObject,
        public iscore::Plugin_QtInterface,
        public iscore::FactoryInterface_QtInterface,
        public iscore::FactoryList_QtInterface,
        public iscore::CommandFactory_QtInterface,
        public iscore::ApplicationPlugin_QtInterface
{
        Q_OBJECT
        Q_PLUGIN_METADATA(IID FactoryInterface_QtInterface_iid)
        Q_INTERFACES(
                iscore::Plugin_QtInterface
                iscore::FactoryInterface_QtInterface
                iscore::FactoryList_QtInterface
                iscore::CommandFactory_QtInterface
                iscore::ApplicationPlugin_QtInterface
                )

    public:
        iscore_addon_pd();
        ~iscore_addon_pd();
        std::vector<std::unique_ptr<iscore::InterfaceListBase>> factoryFamilies() override;

        std::vector<std::unique_ptr<iscore::InterfaceBase>> factories(
                const iscore::ApplicationContext& ctx,
                const iscore::InterfaceKey& factoryName) const override;

        // CommandFactory_QtInterface interface
        std::pair<const CommandGroupKey, CommandGeneratorMap> make_commands() override;

        // Defined in ApplicationPlugin_QtInterface
        iscore::GUIApplicationPlugin* make_guiApplicationPlugin(
                const iscore::GUIApplicationContext& app) override;

        iscore::Version version() const override;
        UuidKey<iscore::Plugin> key() const override;
        std::vector<iscore::PluginKey> required() const override;
};
