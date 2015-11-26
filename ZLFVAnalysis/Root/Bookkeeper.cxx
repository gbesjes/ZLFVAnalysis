//vim: ts=4 sw=4
#include "ZLFVAnalysis/Bookkeeper.h"
#include "ZLFVAnalysis/Utils.h"

#include "cafe/Config.h"

#include "xAODRootAccess/TEvent.h"
#include "xAODEventInfo/EventInfo.h"
#include "xAODCutFlow/CutBookkeeper.h"
#include "xAODCutFlow/CutBookkeeperContainer.h"

#include "TDirectory.h"
#include "TH1F.h"
#include "TObjArray.h"
#include "TObjString.h"
#include "TFile.h"
#include "TTree.h"

#include <stdexcept>
#include <fstream>
#include <sstream>

Bookkeeper::Bookkeeper(const char *name) : 
    cafe::Processor(name), m_name(name), m_counter(0), m_fileInfos(0), m_eventCounter(0),
    m_derivationTag(DerivationTag::INVALID_Derivation), m_isData(false), _directory(0) {
    
    cafe::Config config(name);
    m_isData = config.get("IsData",false);
    m_derivationTag = Utils::derivationTagFromString(config.get("DerivationTag","xxxx"));
    if ( m_derivationTag == DerivationTag::INVALID_Derivation ) { 
        throw(std::domain_error("Bookkeeper: invalid derivation tag specified"));
    }

    openPoolFileCatalog();
}

void Bookkeeper::openPoolFileCatalog() {
    std::ifstream pfcfile("pfc.txt", std::ios::in);
    if ( !(pfcfile.is_open() && pfcfile) ) {
        std::cerr << "Could not open text PoolFileCatalog pfc.txt" << std::endl;
        return;
    }
    
    std::string fname, guid;
    while ( true ) {
        pfcfile >> fname;
        if ( !pfcfile ) break;
        pfcfile >> guid;
        if ( !pfcfile ) break;
        m_fileCatalog[fname] = guid;
        std::cout << "Adding " << fname << " " << guid << " to file catalog " << std::endl;
    }
    pfcfile.close();

}

void Bookkeeper::begin() {
    TDirectory* dir = getDirectory();
    if ( !dir ) {
        throw std::runtime_error("No root TDirectory defined in processor ZeroLeptonSR("+this->name()+")");
    }

    m_counter = new TH1D(("Counter_Bookkeeper_"+name()).c_str(),"Bookkeeper counter",2,0.,2.);
    m_counter->GetXaxis()->SetBinLabel(1,"Number of events");
    m_counter->GetXaxis()->SetBinLabel(2,"Sum of gen. weights");

    m_fileInfos = new TObjArray;
}

void Bookkeeper::inputFileOpened(TFile *file) {
    m_eventCounter = 0;
    m_openedFiles.push_back(file->GetName());
    if ( m_isData ) {
        std::cout << " Derivation bookeeping temporarily switched off for real data due to bugs in event processing " << std::endl;
        return;
    }
    if ( m_derivationTag == DerivationTag::p2353 || m_derivationTag == DerivationTag::p2363 || 
            m_derivationTag == DerivationTag::p2372 || m_derivationTag == DerivationTag::p2375 ||
            m_derivationTag == DerivationTag::p2377 || m_derivationTag == DerivationTag::p2384 ||
            m_derivationTag == DerivationTag::p2419 || m_derivationTag == DerivationTag::p2425 ) {    

        // extract information from CutBookkeeperContainer in Metadata
        // https://twiki.cern.ch/twiki/bin/view/AtlasProtected/AnalysisMetadata
        xAOD::TEvent* event = dynamic_cast< xAOD::TEvent* >(xAOD::TActiveEvent::event());
        if ( !event ) {
            throw std::logic_error("Bookkeeper: could not get active TEvent !");
        }
        
        const xAOD::CutBookkeeperContainer* incompleteCBC = 0;
        if ( !event->retrieveMetaInput(incompleteCBC, "IncompleteCutBookkeepers").isSuccess()) {
            throw std::logic_error("Bookkeeper: could not retrieve CutBookkeeperContainer with tag IncompleteCutBookkeepers");
        }
        
        if ( incompleteCBC->size() != 0 ) {
            throw std::logic_error("Bookkeeper: IncompleteCutBookkeepers not empty");
        }
        
        const xAOD::CutBookkeeperContainer* completeCBC = 0;
        if ( !event->retrieveMetaInput(completeCBC, "CutBookkeepers").isSuccess()) { 
            throw std::logic_error("Bookkeeper: could not retrieve CutBookkeeperContainer with tag CutBookkeepers");
        }
        
        // Find the smallest cycle number, the original first processing step/cycle
        int minCycle = 10000;
        for ( auto cbk : *completeCBC ) {
            if ( ! cbk->name().empty()  && minCycle > cbk->cycle() ){ 
                minCycle = cbk->cycle(); 
            }
        }

        // Now, find the right one that contains all the needed info...
        const xAOD::CutBookkeeper* allEventsCBK=0;
        for ( auto cbk : *completeCBC ) {
            if ( minCycle == cbk->cycle() && cbk->name() == "AllExecutedEvents" ){
                allEventsCBK = cbk;
                break;
            }
        }

        uint64_t nEventsProcessed  = allEventsCBK->nAcceptedEvents();
        double sumOfWeights        = allEventsCBK->sumOfEventWeights();
        //double sumOfWeightsSquared = allEventsCBK->sumOfEventWeightsSquared(); 

        std::cout << "Derivation file : " << file->GetName() << " nprocessed " <<  nEventsProcessed << " sumW " << sumOfWeights << std::endl;

        m_counter->Fill(0.1,nEventsProcessed);
        m_counter->Fill(1.1,sumOfWeights);

    }
}

void Bookkeeper::inputFileClosing(TFile *file) {
    m_eventsPerFile.push_back(m_eventCounter);
    m_closedFiles.push_back(file->GetName());
}

bool Bookkeeper::processEvent(xAOD::TEvent& event) {
    ++m_eventCounter;
    if ( m_derivationTag == DerivationTag::NotADerivation ||  m_derivationTag == DerivationTag::p1872 ) {
        m_counter->Fill(0.1,1.);
        const xAOD::EventInfo* eventInfo = 0;
        if ( ! event.retrieve( eventInfo, "EventInfo").isSuccess() ) throw std::runtime_error("Could not retrieve EventInfo");

        if(eventInfo->eventType( xAOD::EventInfo::IS_SIMULATION ) ){
            m_counter->Fill(1.1, eventInfo->mcEventWeight(0));
        }
        else {
            m_counter->Fill(1.1, 1.);
        }
    }
    return true;
}

void Bookkeeper::finish() {
    std::cout << "Files processed: " << std::endl;
    for ( std::size_t i = 0; i < m_closedFiles.size(); ++i ){
        std::string guid("UNKNOW-GUID");
        std::string logicalName =  m_closedFiles[i];
        // has to cover files like 
        // root://atlas-xrd-central.usatlas.org:1094//atlas/rucio/mc14_8TeV:AOD.01507240._010001.pool.root.2
        // which has a full path and a ":" in the basename
        std::size_t pos = logicalName.rfind("/");
        if ( pos != std::string::npos ) logicalName = logicalName.substr(pos+1);
        pos = logicalName.rfind(":"); // ruci has 
        if ( pos != std::string::npos ) logicalName = logicalName.substr(pos+1);

        std::map<std::string, std::string>::const_iterator it = m_fileCatalog.find(logicalName);
        if ( it != m_fileCatalog.end() ) guid = (*it).second;
        std::cout << " " << logicalName << " : GUID " << guid << " : "<<  m_eventsPerFile[i] << " events " << std::endl;
        std::stringstream s;
        s << logicalName << " " << guid << " " << m_eventsPerFile[i];
        m_fileInfos->Add(new TObjString(s.str().c_str()));
    }

    // FIXME these info are lost in the panda merging jobs
    getDirectory()->cd();
    m_fileInfos->Write(("FileList_Bookkeeper_"+name()).c_str(),TObject::kSingleKey);
}

ClassImp(Bookkeeper);


