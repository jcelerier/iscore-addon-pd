#include "score_addon_pd.hpp"
#include <Pd/PdFactory.hpp>
#include <Pd/PdLayer.hpp>
#include <Pd/Executor/PdExecutor.hpp>
#include <Pd/Commands/PdCommandFactory.hpp>
#include <Dataflow/ApplicationPlugin.hpp>
#include <score/plugins/customfactory/FactorySetup.hpp>
#include <score_addon_pd_commands_files.hpp>

#include <Scenario/score_plugin_scenario.hpp>
#include <Pd/UI/PdNode.hpp>
#include <score_plugin_deviceexplorer.hpp>

#include "z_libpd.h"
#include "m_imp.h"
std::pair<const CommandGroupKey, CommandGeneratorMap> score_addon_pd::make_commands()
{
    using namespace Pd;
    using namespace Dataflow;
    std::pair<const CommandGroupKey, CommandGeneratorMap> cmds{
        Pd::CommandFactoryName(),
                CommandGeneratorMap{}};

    using Types = TypeList<
#include <score_addon_pd_commands.hpp>
      >;
    for_each_type<Types>(score::commands::FactoryInserter{cmds.second});


    return cmds;
}
std::vector<std::unique_ptr<score::InterfaceBase>> score_addon_pd::factories(
        const score::ApplicationContext& ctx,
        const score::InterfaceKey& key) const
{
    return instantiate_factories<
            score::ApplicationContext,
         FW<Process::ProcessModelFactory, Pd::ProcessFactory>
        , FW<Process::InspectorWidgetDelegateFactory, Pd::InspectorFactory>
        , FW<Process::LayerFactory, Pd::LayerFactory>
        , FW<Engine::Execution::ProcessComponentFactory, Pd::ComponentFactory>
    >(ctx, key);
}

score_addon_pd::score_addon_pd()
{
    libpd_init();
    // TODO should not be necessarey
    libpd_init_audio(2, 2, 44100);

    libpd_set_printhook([] (const char *s) { qDebug() << "string: " << s; });
    libpd_set_floathook([] (const char *s, float x)  { qDebug() << "float: " << s << x; });
}


auto score_addon_pd::required() const
  -> std::vector<score::PluginKey>
{
    return {
      score_plugin_scenario::static_key(),
      score_plugin_deviceexplorer::static_key()
    };
}

score_addon_pd::~score_addon_pd()
{

}

score::Version score_addon_pd::version() const
{
  return score::Version{1};
}

UuidKey<score::Plugin> score_addon_pd::key() const
{
    return_uuid("ed87a509-7319-4303-8cf7-3bba849458cf");
}
