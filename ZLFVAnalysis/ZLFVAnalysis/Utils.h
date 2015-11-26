// vim: ts=4 sw=4
#ifndef HAVE_UTILS_H
#define HAVE_UTILS_H

#include <string>

#include "TVector2.h"
#include "xAODTracking/VertexFwd.h"
#include "xAODJet/JetContainerInfo.h"

//--------------------------------------------------------------------------
// Utility functions that may require access to the event store
//--------------------------------------------------------------------------

enum class RunPeriod { p7tev ,p8tev ,p13tev, INVALID };
enum class DerivationTag { NotADerivation, p1872, p2353, p2363, p2372, p2375, p2377, p2384, p2419, p2425, INVALID_Derivation };

namespace Utils {
    RunPeriod periodFromString(const std::string& period);
    DerivationTag derivationTagFromString(const std::string& tag);
    xAOD::JetInput::Type JetTypeFromString(const std::string& algname);
}

#endif
