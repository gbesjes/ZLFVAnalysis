// vim: ts=4 sw=4
#include "ZLFVAnalysis/PileUpRWProcessor.h"
#include "xAODRootAccess/TEvent.h"
#include "xAODRootAccess/TActiveStore.h"
#include "xAODRootAccess/TStore.h"
#include "xAODRootAccess/tools/ReturnCheck.h"
#include "xAODEventInfo/EventInfo.h"
#include "cafe/Config.h"
#include "PileupReweighting/TPileupReweighting.h"
#include "PileupReweighting/PileupReweightingTool.h"

#include <string>
#include <vector>
#include <stdexcept>

PileUpRWProcessor::PileUpRWProcessor(const char *name)
: cafe::Processor(name) {
    cafe::Config config(name);
    m_noReweighting = config.get("noReweighting", false);
    m_forcedRunNumber = config.get("forcedRunNumber", -1);

    std::vector<std::string> prwFiles = config.getVString("PileUpMCFileNames");
    std::vector<std::string> lumicalcFiles = config.getVString("PileUpDataFileNames");

    m_PileupTool_CENTRAL = std::auto_ptr<CP::PileupReweightingTool>(new CP::PileupReweightingTool("PileUpReweightingTool_CENTRAL"));
    if ( !m_PileupTool_CENTRAL->setProperty("Prefix","CENTRAL_").isSuccess()) throw std::runtime_error("Could not initialise PileupReweightingTool");
    if ( !m_PileupTool_CENTRAL->setProperty("ConfigFiles",prwFiles).isSuccess()) throw std::runtime_error("Could not initialise PileupReweightingTool");
    if ( !m_PileupTool_CENTRAL->setProperty("LumiCalcFiles",lumicalcFiles).isSuccess()) throw std::runtime_error("Could not initialise PileupReweightingTool");
    if ( !m_PileupTool_CENTRAL->setProperty("DefaultChannel",410000).isSuccess()) throw std::runtime_error("Could not initialise PileupReweightingTool");
    if ( !m_PileupTool_CENTRAL->initialize().isSuccess()) throw std::runtime_error("Could not initialise PileupReweightingTool");

    m_PileupTool_UP = std::auto_ptr<CP::PileupReweightingTool>(new CP::PileupReweightingTool("PileUpReweightingTool_UP"));
    if ( !m_PileupTool_UP->setProperty("Prefix","UP_").isSuccess()) throw std::runtime_error("Could not initialise PileupReweightingTool");
    if ( !m_PileupTool_UP->setProperty("DataScaleFactor",1.1).isSuccess()) throw std::runtime_error("Could not initialise PileupReweightingTool");
    if ( !m_PileupTool_UP->setProperty("ConfigFiles",prwFiles).isSuccess()) throw std::runtime_error("Could not initialise PileupReweightingTool");
    if ( !m_PileupTool_UP->setProperty("LumiCalcFiles",lumicalcFiles).isSuccess()) throw std::runtime_error("Could not initialise PileupReweightingTool");
    if ( !m_PileupTool_UP->setProperty("DefaultChannel",410000).isSuccess()) throw std::runtime_error("Could not initialise PileupReweightingTool");
    if ( !m_PileupTool_UP->initialize().isSuccess()) throw std::runtime_error("Could not initialise PileupReweightingTool");

    m_PileupTool_DOWN = std::auto_ptr<CP::PileupReweightingTool>(new CP::PileupReweightingTool("PileUpReweightingTool_DOWN"));
    if ( !m_PileupTool_DOWN->setProperty("Prefix","DOWN_").isSuccess()) throw std::runtime_error("Could not initialise PileupReweightingTool");
    if ( !m_PileupTool_DOWN->setProperty("DataScaleFactor",0.9).isSuccess()) throw std::runtime_error("Could not initialise PileupReweightingTool");
    if ( !m_PileupTool_DOWN->setProperty("ConfigFiles",prwFiles).isSuccess()) throw std::runtime_error("Could not initialise PileupReweightingTool");
    if ( !m_PileupTool_DOWN->setProperty("LumiCalcFiles",lumicalcFiles).isSuccess()) throw std::runtime_error("Could not initialise PileupReweightingTool");
    if ( !m_PileupTool_DOWN->setProperty("DefaultChannel",410000).isSuccess()) throw std::runtime_error("Could not initialise PileupReweightingTool");
    if ( !m_PileupTool_DOWN->initialize().isSuccess()) throw std::runtime_error("Could not initialise PileupReweightingTool");
}

bool PileUpRWProcessor::processEvent(xAOD::TEvent& event) {
    std::vector<float>* pileupWeight = new std::vector<float>(3, 1.);

    const xAOD::EventInfo* eventInfo = 0;
    if( !event.retrieve( eventInfo, "EventInfo").isSuccess() ) { 
        throw std::runtime_error("Could not retrieve EventInfo");
    }

    int runnumber = -1;
    if ( m_forcedRunNumber > 0 ) {
        runnumber = m_forcedRunNumber; 
    } else {
        runnumber = (int)eventInfo->runNumber();
    }

    // no reweighting for mc14_13TeV nor in mc15_13TeV week1
    if ( !m_noReweighting && runnumber != 222222  && runnumber != 222250  ) {
        if ( !m_PileupTool_CENTRAL->apply(*eventInfo).isSuccess()) throw std::runtime_error("Could not execute PileupReweightingTool");
        if ( !m_PileupTool_UP->apply(*eventInfo).isSuccess()) throw std::runtime_error("Could not execute PileupReweightingTool");
        if ( !m_PileupTool_DOWN->apply(*eventInfo).isSuccess()) throw std::runtime_error("Could not execute PileupReweightingTool");
        (*pileupWeight)[0] = eventInfo->auxdata<float>("CENTRAL_PileupWeight");
        (*pileupWeight)[1] = eventInfo->auxdata<float>("UP_PileupWeight");
        (*pileupWeight)[2] = eventInfo->auxdata<float>("DOWN_PileupWeight");
    }

    std::cout << runnumber << " Pileup weights " << pileupWeight->at(0) << " " << pileupWeight->at(1) << " " << pileupWeight->at(2) << std::endl;

    xAOD::TStore* store = xAOD::TActiveStore::store();
    RETURN_CHECK("PileUpRWProcessor::processEvent", store->record(pileupWeight, "pileupWeights"));

    return true;
}

ClassImp(PileUpRWProcessor);

