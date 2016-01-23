// vim: ts=4 sw=4
#ifndef HAVE_UTILS_H
#define HAVE_UTILS_H

#include <string>

#include "TVector2.h"
#include "xAODTracking/VertexFwd.h"
#include "xAODJet/JetContainerInfo.h"

#include "xAODEventInfo/EventInfo.h"
#include "xAODRootAccess/TEvent.h"
#include "xAODRootAccess/TStore.h"
#include "xAODRootAccess/TActiveStore.h"
#include "xAODTracking/Vertex.h"
#include "PATInterfaces/SystematicSet.h"

//--------------------------------------------------------------------------
// Utility functions that may require access to the event store
//--------------------------------------------------------------------------

enum class RunPeriod { p7tev ,p8tev ,p13tev, INVALID };
enum class DerivationTag { NotADerivation, p1872, p2353, p2363, p2372, p2375, p2377, p2384, p2419, p2425, INVALID_Derivation };

namespace Utils {
    typedef struct {
        uint32_t RunNumber;
        unsigned long long EventNumber;
        uint32_t LumiBlockNumber;
        uint32_t mc_channel_number;
    } EventInfo;

    RunPeriod periodFromString(const std::string& period);
    DerivationTag derivationTagFromString(const std::string& tag);
    xAOD::JetInput::Type JetTypeFromString(const std::string& algname);

    const xAOD::EventInfo* GetEventInfo(xAOD::TEvent& event);
    const xAOD::Vertex* GetPrimaryVertex(xAOD::TEvent& event);
    const EventInfo ExtractEventInfo(const xAOD::EventInfo* info, bool isData = true);

    CP::SystematicSet* GetCurrentSystematicSet(xAOD::TEvent& event);
}

#endif
