#pragma once
#include <Pd/Inspector/PdInspectorWidget.hpp>
#include <Pd/PdProcess.hpp>
#include <Process/GenericProcessFactory.hpp>
#include <Process/WidgetLayer/WidgetProcessFactory.hpp>
namespace Pd
{
using LayerFactory = WidgetLayer::LayerFactory<Pd::ProcessModel, PdWidget>;
}
