#pragma once
#include <Pd/Inspector/PdInspectorWidget.hpp>
#include <Pd/PdProcess.hpp>
#include <Process/GenericProcessFactory.hpp>
#include <Process/WidgetLayer/WidgetProcessFactory.hpp>

namespace Pd {
using Layer = Process::LayerModel_T<Pd::ProcessModel>;
}
LAYER_METADATA(
        ,
        Pd::Layer,
        "5d802627-8b20-4737-b14e-639523a17e7a",
        "PdLayer",
        "PdLayer"
        )

namespace Pd
{
using LayerFactory = WidgetLayer::LayerFactory<Pd::ProcessModel, Pd::Layer, PdWidget>;
}
