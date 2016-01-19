// vim: ts=4 sw=4
#include "ZLFVAnalysis/IsData.h"
#include "xAODRootAccess/TEvent.h"
#include "xAODRootAccess/tools/ReturnCheck.h"
#include "xAODEventInfo/EventInfo.h"
#include "cafe/Config.h"

#include <stdexcept>

IsData::IsData(const char *name)
    : cafe::Processor(name), m_enforce(false), m_expected(true) {
    cafe::Config config(name);
    m_enforce = config.get("Enforce",false);
    m_expected = config.get("Expected",true);
}

bool IsData::processEvent(xAOD::TEvent& event) {
    const xAOD::EventInfo* eventInfo = 0;
    RETURN_CHECK("MCEventVeto::processEvent", event.retrieve( eventInfo, "EventInfo") );

    bool isData = true;
    if(eventInfo->eventType( xAOD::EventInfo::IS_SIMULATION ) ) {
        isData = false;
    }

    if ( m_enforce && isData != m_expected ) {
        throw std::runtime_error("Expected data (resp MC) event and found MC (resp data) event !");
    }

    std::cout << "IsData::processEvent: data => " << isData << std::endl;

    return isData;
}

ClassImp(IsData);

