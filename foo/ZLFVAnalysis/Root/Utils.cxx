// vim: ts=4 sw=4
#include "ZLFVAnalysis/Utils.h"

RunPeriod Utils::periodFromString(const std::string& period) {
    if ( period == "p7tev" ) return RunPeriod::p7tev;
    else if ( period == "p8tev" ) return RunPeriod::p8tev;
    else if ( period == "p13tev" ) return RunPeriod::p13tev;
    return RunPeriod::INVALID;
}

DerivationTag Utils::derivationTagFromString(const std::string& tag) {
    if ( tag == "" || tag =="None" || tag == "none" || tag == "NA") return DerivationTag::NotADerivation;
    if ( tag == "p1872" ) return DerivationTag::p1872;
    if ( tag == "p2353" ) return DerivationTag::p2353;
    if ( tag == "p2363" ) return DerivationTag::p2363;
    if ( tag == "p2372" ) return DerivationTag::p2372;
    if ( tag == "p2375" ) return DerivationTag::p2375;
    if ( tag == "p2377" ) return DerivationTag::p2377;
    if ( tag == "p2384" ) return DerivationTag::p2384;
    if ( tag == "p2419" ) return DerivationTag::p2419;
    if ( tag == "p2425" ) return DerivationTag::p2425;
    return DerivationTag::INVALID_Derivation;
}

xAOD::JetInput::Type Utils::JetTypeFromString(const std::string& algname) {
    if ( algname.find(xAOD::JetInput::typeName(xAOD::JetInput::LCTopo)) != std::string::npos ) { 
        return xAOD::JetInput::LCTopo;
    }
    
    if ( algname.find(xAOD::JetInput::typeName(xAOD::JetInput::EMTopo)) != std::string::npos ) { 
        return xAOD::JetInput::EMTopo;
    }

    return xAOD::JetInput::Uncategorized;
}

const xAOD::Vertex* Utils::GetPrimaryVertex(xAOD::TEvent& event) {
    const xAOD::VertexContainer* vertices = nullptr;
    if (!event.retrieve( vertices, "PrimaryVertices" ).isSuccess() ) {
        return nullptr;
    }
    
    for ( const auto& vx: *vertices ) {
        if(vx->vertexType() == xAOD::VxType::PriVtx) { 
            return vx;
        }
    }
    
    return NULL;
}
