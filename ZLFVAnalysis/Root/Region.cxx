// vim: ts=4 sw=4
#include <iostream>
#include <stdexcept>

#include "TDirectory.h"
#include "TVector2.h"

#include "cafe/Processor.h"
#include "cafe/Controller.h"
#include "cafe/Config.h"

#include "ZLFVAnalysis/Region.h"
//#include "ZLFVAnalysis/PhysObjProxies.h"

#include "xAODRootAccess/TEvent.h"
#include "xAODRootAccess/TActiveStore.h"
#include "xAODRootAccess/TStore.h"
#include "xAODTracking/Vertex.h"
#include "xAODEventInfo/EventInfo.h"
#include "xAODMissingET/MissingETContainer.h"
#include "PATInterfaces/SystematicSet.h"

Region::Region(const char *name)
    : cafe::Processor(name),
      m_tree(0),
      m_stringRegion("SRAll"),
      m_doSmallNtuple(true),
      m_fillTRJigsawVars(true),
      m_fillReclusteringVars(true),
      m_doRecl(false),
      m_IsData(false),
      m_IsTruth(false),
      m_IsSignal(false),
      m_DoSystematics(false),
      m_period(RunPeriod::INVALID),
      m_suffix(""),
      m_suffixRecl(""),
      //m_physobjsFiller(0),
      //m_cutVal(),
      //m_proxyUtils(m_IsData),
      //m_ZLUtils(m_IsData, NotADerivation),
      m_counter(0),
      m_counterRepository("",false,0),
      m_treeRepository(),
      m_derivationTag(DerivationTag::INVALID_Derivation) {
    cafe::Config config(name);
    m_fillTRJigsawVars = config.get("fillTRJigsawVars",true);
    m_IsData = config.get("IsData",false);
    m_IsTruth = config.get("IsTruth",false);
    m_IsSignal = config.get("IsSignal",false);
    m_suffixRecl = config.get("suffixRecl","");
    m_doRecl = config.get("doRecl",false);
    m_DoSystematics = config.get("DoSystematics",false);

    m_period = Utils::periodFromString(config.get("Period","p13tev"));
    if ( m_period == RunPeriod::p7tev ) {
        throw(std::domain_error("Region does not support the 7tev run period"));
    } else if ( m_period == RunPeriod::INVALID ) {
        throw(std::domain_error("Region: invalid run period specified"));
    }

    m_derivationTag = Utils::derivationTagFromString(config.get("DerivationTag",""));
    if ( m_derivationTag == DerivationTag::INVALID_Derivation ) {
        throw(std::domain_error("Region: invalid derivation tag specified"));
    }

    std::string cutfile = config.get("cutfile","None");
    if ( cutfile == "None" ) {
        throw(std::domain_error("Region: invalid cut file specified"));
    }
    //m_cutVal.ReadCutValues(cutfile);

    m_suffix = config.get("suffix","");
    m_suffixSyst = "test";
    //m_physobjsFiller = new PhysObjProxyFiller(20000.f,10000.f,10000.f,25000.f,m_suffix,m_doRecl,m_suffixRecl,m_suffixSyst);
    //m_physobjsFillerTruth = new PhysObjProxyFillerTruth(20000.f,10000.f,10000.f,25000.f,m_suffix);
    //m_proxyUtils = PhysObjProxyUtils(m_IsData);

    //m_ZLUtils = ZeroLeptonUtils(m_IsData, m_derivationTag);
}

Region::~Region() {
    if ( !m_DoSystematics && m_counter ) delete m_counter;
    //if ( m_physobjsFiller ) delete m_physobjsFiller;
    //if ( m_physobjsFillerTruth ) delete m_physobjsFillerTruth;
}

TTree* Region::bookTree(const std::string& treename) {
    const char* name(treename.c_str());
    TTree* tree = new TTree(name,"ZeroLepton final optimisation");
    tree->SetDirectory(getDirectory());

    // the following function add several variables to this tree

    //bookNTVars(tree,m_ntv,false);
    //if ( m_fillReclusteringVars ) bookNTReclusteringVars(tree,m_RTntv);
    //bookNTExtraVars(tree,m_extrantv);
    //if ( m_fillTRJigsawVars) bookNTRJigsawVars(tree,m_rjigsawntv);
    return tree;
}

TTree* Region::getTree(const std::string& treename) {
    std::map<std::string,TTree*>::const_iterator pos = m_treeRepository.find(treename);
    if ( pos == m_treeRepository.end() ) {
        pos = m_treeRepository.insert(std::make_pair(treename, bookTree(treename))).first;
    }
    return pos->second;
}

void Region::begin() {
    std::string sSR = m_stringRegion;
    if(m_doSmallNtuple) {
        sSR += "NT";
    }

    if ( m_DoSystematics ) {
        m_counterRepository = CounterRepository("ZLFVCounter"+m_stringRegion, m_IsSignal, getDirectory());
    } else {
        m_counter = new Counter("ZLFVCounter"+m_stringRegion,40,m_IsSignal);
        if(m_doSmallNtuple) m_tree = bookTree(sSR);
    }
}

//void Region::initializeTree() {

//}

bool Region::processEvent(xAOD::TEvent& event) {
    // access the transient store
    xAOD::TStore* store = xAOD::TActiveStore::store();
    std::string systag = ""; //TODO: better choice of variable here!!
    if ( m_DoSystematics ) {
        CP::SystematicSet* currentSyst = Utils::GetCurrentSystematicSet(event);
        std::string sysname = currentSyst->name();
        m_counter = m_counterRepository.counter(sysname);
        
        if (sysname == "" ) {
            m_tree = getTree(m_stringRegion+"NT");
        } else {
            systag = "_"+sysname+"_";
            m_tree = getTree(m_stringRegion+"NT_"+sysname);
            //m_physobjsFiller->setSuffix(m_suffix+systag);
        }
    }

    // eventInfo
    const xAOD::EventInfo* eventInfo = Utils::GetEventInfo(event);
    const Utils::EventInfo _eventInfo = Utils::ExtractEventInfo(eventInfo);

    // global event weight
    float weight = 1.f;

    // get generator weight
    float genWeight = 1.f;
    if ( !m_IsData ) {
        genWeight = eventInfo->mcEventWeight(0);
        //out() << " gen weight " << genWeight << std::endl;
        weight *= genWeight;
    }

    // get pileup weights
    std::vector<float>* pileupWeights = 0;
    if ( !m_IsData && !m_IsTruth ) {
        if ( !store->retrieve< std::vector<float> >(pileupWeights,"pileupWeights").isSuccess() ) throw std::runtime_error("could not retrieve pileupWeights");
        //out() << " pileup weight " << (*pileupWeights)[0] << std::endl;
        //weight *= (*pileupWeights)[0];
    } else {
        static std::vector<float> dummy(3,1.);
        pileupWeights = &dummy;
    }

    // hardproc (see SUSYTools)
    // FIXME : to be implemented for SM MC
    int trueTopo = 0;
    if ( m_IsSignal ) {
        unsigned int* finalstate = 0;
        if ( !store->retrieve<unsigned int>(finalstate,"HardProcess").isSuccess() ) throw std::runtime_error("could not retrieve HardProcess");
        trueTopo = *finalstate;
    }

    // counters
    int incr=0;
    m_counter->increment(1.,incr++,"NbOfEvents",trueTopo);
    m_counter->increment(weight,incr++,"runNumber",trueTopo);
    // FIXME do something with bin 38 & 39 ?!

    // Normalisation weight, e.g. MC cross-section vs luminosity
    // FIXME : to be implemented ?
    std::vector<float> normWeight(3,0.);

    unsigned int veto = 0;
    // MC event veto (e.g. to remove sample phase space overlap)
    if ( ! m_IsData && (m_period == RunPeriod::p8tev || m_period == RunPeriod::p13tev) && !m_IsTruth ) {
        unsigned int* pveto = 0;
        if ( !store->retrieve<unsigned int>(pveto, "mcVetoCode").isSuccess() ) {
            throw std::runtime_error("could not retrieve mcVetoCode");
        }
        veto = *pveto;
        bool* mcaccept = 0;
        if ( !store->retrieve<bool>(mcaccept,"mcAccept").isSuccess() ) {
            throw std::runtime_error("could not retrieve mcaccept");
        }
        if ( !*mcaccept ) {
            return true;
        }
    }
    m_counter->increment(weight,incr++,"Truth filter",trueTopo);

    // Good run list
    if ( m_IsData ) {
        bool* passGRL = 0;
        if ( !store->retrieve<bool>(passGRL,"passGRL").isSuccess() ) {
            throw std::runtime_error("could not retrieve passGRL");
        }
        if ( ! *passGRL ) {
            return true;
        }
    }
    m_counter->increment(weight,incr++,"GRL",trueTopo);

    // REAL SELECTION HERE

    // Trigger selection
    if(! m_IsTruth) {
        if( !(int)eventInfo->auxdata<char>("HLT_xe70")==1) return true;
    }
    m_counter->increment(weight,incr++,"Trigger",trueTopo);

    // LAr, Tile, reco problems in data
    if ( m_IsData ) {
        bool* badDetectorQuality = 0 ;
        if ( !store->retrieve<bool>(badDetectorQuality,"badDetectorQuality").isSuccess() ) throw std::runtime_error("could not retrieve badDetectorQuality");
        if ( *badDetectorQuality ) return true;
    }
    m_counter->increment(weight,incr++,"Detector cleaning",trueTopo);

    // primary vertex cut
    const xAOD::Vertex* primaryVertex = 0;
    if(! m_IsTruth) {
        primaryVertex = Utils::GetPrimaryVertex(event);
        if ( !primaryVertex ||  !( primaryVertex->nTrackParticles() > 2) ) return true;
    }
    m_counter->increment(weight,incr++,"Vertex Cut",trueTopo);


    // TODO: how do we inherit nicely here?

    if(m_doSmallNtuple) {
        unsigned int runnum = _eventInfo.RunNumber;
        if ( not m_IsData && not m_IsTruth) runnum = _eventInfo.mc_channel_number;

        // other cleaning tests
        unsigned int cleaning = 0;
        unsigned int power2 = 1;

        // bad jet veto
        //if ( !bad_jets.empty() ) cleaning += power2;
        power2 *= 2;

        // bad muons for MET cut: based on non isolated muons
        //if ( m_proxyUtils.isbadMETmuon(baseline_muons, MissingEt, *missingET) ) cleaning += power2;
        power2 *= 2;

        // bad Tile cut
        //if ( m_proxyUtils.badTileVeto(good_jets,*missingET)) cleaning += power2;
        power2 *= 2;

        // Negative-cell cleaning cut (no longer used)
        power2 *= 2;

        // average timing of 2 leading jets
        //if (fabs(time[0]) > 5) cleaning += power2;
        power2 *= 2;

        if(!m_IsTruth) {
            //bool chfTileVeto =  m_proxyUtils.chfTileVeto(good_jets);
            //if ( m_period == RunPeriod::p8tev && chfTileVeto ) cleaning += power2;
            //bool chfVeto = m_proxyUtils.chfVeto(good_jets);
            //if ( chfVeto ) cleaning += power2 * 2;
        }
        power2 *= 4;

        //float dPhiBadTile = m_proxyUtils.dPhiBadTile(good_jets,*missingET);

        bool isNCBEvent = false;
        if ( m_IsData ) {
            bool* NCBEventFlag = 0;
            if ( !store->retrieve<bool>(NCBEventFlag,"NCBEventFlag").isSuccess() ) throw std::runtime_error("could not retrieve NCBEventFlag");
            isNCBEvent = *NCBEventFlag;
        }

        // TODO: some clever fill function here
        //m_proxyUtils.FillNTVars(m_ntv, runnum, EventNumber, LumiBlockNumber, veto, weight, normWeight, *pileupWeights, genWeight,ttbarWeightHT,ttbarWeightPt2,ttbarAvgPt,WZweight, btag_weight, ctag_weight, b_jets.size(), c_jets.size(), MissingEt, phi_met, Meff, meffincl, minDphi, RemainingminDPhi, good_jets, trueTopo, cleaning, time[0],jetSmearSystW,0, 0., 0.,dPhiBadTile,isNCBEvent,m_IsTruth,baseline_taus,signal_taus);

        if ( systag == ""  && !m_IsTruth ) {
            // systag == "" is the nominal one
            std::vector<float>* p_systweights = 0;
            if ( ! store->retrieve(p_systweights,"event_weights"+m_suffix).isSuccess() ) throw std::runtime_error("Could not retrieve event_weights"+m_suffix);
            //m_ntv.systWeights = *p_systweights;

            std::vector<float>* p_btagSystweights = 0;
            if ( ! store->retrieve(p_btagSystweights,"btag_weights"+m_suffix).isSuccess() ) throw std::runtime_error("Could not retrieve btag_weights"+m_suffix);
            //m_ntv.btagSystWeights = *p_btagSystweights;
        }
        m_tree->Fill();
    }
    return true;
}

void Region::finish() {
    if ( m_DoSystematics ) {
        out() << m_counterRepository << std::endl;
    } else {
        out() << *m_counter << std::endl;
    }
}

ClassImp(Region);

