#pragma once
#include <Process/ProcessMetadata.hpp>
#include <QString>
#include <score_addon_pd_export.h>

namespace Pd
{
class ProcessModel;
}

PROCESS_METADATA(
        SCORE_ADDON_PD_EXPORT,
        Pd::ProcessModel,
        "7b3b18ea-311b-40f9-b04e-60ec1fe05786",
        "PureData",
        "PureData",
        "Script",
        {},
        Process::ProcessFlags::SupportsAll |
        Process::ProcessFlags::PutInNewSlot
        )

