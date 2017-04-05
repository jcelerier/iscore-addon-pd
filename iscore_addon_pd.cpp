#include "iscore_addon_pd.hpp"
#include <Pd/PdFactory.hpp>
#include <Pd/PdLayer.hpp>
#include <Pd/Executor/PdExecutor.hpp>
#include <Pd/Commands/PdCommandFactory.hpp>
#include <Pd/Inspector/PdInspectorFactory.hpp>
#include <Pd/ApplicationPlugin.hpp>
#include <Pd/DataflowClock.hpp>
#include <iscore/plugins/customfactory/FactorySetup.hpp>
#include <iscore_addon_pd_commands_files.hpp>

#include <Scenario/iscore_plugin_scenario.hpp>
#include <iscore_plugin_deviceexplorer.hpp>

#include "z_libpd.h"
#include "m_imp.h"
std::pair<const CommandGroupKey, CommandGeneratorMap> iscore_addon_pd::make_commands()
{
    using namespace Pd;
    using namespace Dataflow;
    std::pair<const CommandGroupKey, CommandGeneratorMap> cmds{
        Pd::CommandFactoryName(),
                CommandGeneratorMap{}};

    using Types = TypeList<
#include <iscore_addon_pd_commands.hpp>
      >;
    for_each_type<Types>(iscore::commands::FactoryInserter{cmds.second});


    return cmds;
}
std::vector<std::unique_ptr<iscore::InterfaceBase>> iscore_addon_pd::factories(
        const iscore::ApplicationContext& ctx,
        const iscore::InterfaceKey& key) const
{
    return instantiate_factories<
            iscore::ApplicationContext,
         FW<Process::ProcessModelFactory, Pd::ProcessFactory>
        , FW<Process::LayerFactory, Pd::LayerFactory>
        , FW<Engine::Execution::ProcessComponentFactory, Pd::ComponentFactory>
        , FW<Process::InspectorWidgetDelegateFactory, Pd::InspectorFactory>
        , FW<Engine::Execution::ClockManagerFactory, Dataflow::ClockFactory>
    >(ctx, key);
}

iscore_addon_pd::iscore_addon_pd()
{
    libpd_init();
    libpd_init_audio(2, 2, 44100);

    libpd_set_printhook([] (const char *s) { qDebug() << "string: " << s; });
    libpd_set_floathook([] (const char *s, float x)  { qDebug() << "float: " << s << x; });
}

iscore::GUIApplicationPlugin*
iscore_addon_pd::make_applicationPlugin(
        const iscore::GUIApplicationContext& app)
{
    return new Dataflow::ApplicationPlugin{app};
}

auto iscore_addon_pd::required() const
  -> std::vector<iscore::PluginKey>
{
    return {
      iscore_plugin_scenario::static_key(),
      iscore_plugin_deviceexplorer::static_key()
    };
}


iscore_addon_pd::~iscore_addon_pd()
{

}

iscore::Version iscore_addon_pd::version() const
{
    return iscore::Version{1};
}

UuidKey<iscore::Plugin> iscore_addon_pd::key() const
{
    return_uuid("ed87a509-7319-4303-8cf7-3bba849458cf");
}
