#pragma once
#include <Process/GenericProcessFactory.hpp>
#include <Process/LayerModel.hpp>
#include <Pd/PdProcess.hpp>
#include <Pd/PdMetadata.hpp>

namespace Pd
{
using Layer = Process::LayerModel_T<Pd::ProcessModel>;
}

LAYER_METADATA(
        ,
        Pd::Layer,
        "3de556e6-6f7d-4d5d-abe3-5aa44a7c6252",
        "PdLayer",
        "PdLayer"
        )

namespace Pd
{
using ProcessFactory = Process::GenericProcessModelFactory<ProcessModel>;
using LayerFactory = Process::GenericDefaultLayerFactory<Layer>;
}
