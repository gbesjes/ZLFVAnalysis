// vim: ts=4 sw=4

#include <stdexcept>
#include <vector>

#include "xAODRootAccess/tools/ReturnCheck.h"
#include "xAODRootAccess/TEvent.h"
#include "xAODRootAccess/TActiveStore.h"
#include "xAODRootAccess/TStore.h"
#include "xAODCore/ShallowCopy.h"
#include "xAODBase/IParticleHelpers.h"
#include "AthContainers/AuxElement.h"

#include "xAODEventInfo/EventInfo.h"
#include "xAODJet/JetContainer.h"
#include "xAODJet/JetAuxContainer.h"
#include "xAODMuon/MuonContainer.h"
#include "xAODEgamma/ElectronContainer.h"
#include "xAODEgamma/PhotonContainer.h"
#include "xAODTau/TauJetContainer.h"
#include "xAODMissingET/MissingETContainer.h"
#include "xAODMissingET/MissingETAuxContainer.h"
#include "PATInterfaces/SystematicSet.h"

#include "SUSYTools/SUSYObjDef_xAOD.h"

#include "ZLFVAnalysis/ObjectBuilder.h"
#include "ZLFVAnalysis/Utils.h"

#include "cafe/Config.h"

#include "TVector2.h"
#include "TRegexp.h"

ObjectBuilder::ObjectBuilder(const char *name) :
    cafe::Processor(name),
    m_SUSYObjTool(0),
    m_isData(false),
    m_isAtlfast(false),
    m_useSmearedJets(false),
    m_doSystematics(false),
    m_photonInOR(false),
    m_electronContainerKey(""),
    m_photonContainerKey(""),
    m_jetContainerKey(),
    m_tauContainerKey(),
    m_suffix(),
    //m_period(INVALID),
    m_derivationTag(DerivationTag::INVALID_Derivation),
    m_JESNuisanceParameterSet(0),
    m_SystInfoList(),
    m_SystMatch()
{

    cafe::Config config(name);
    m_isData = config.get("IsData", false);
    m_is25ns = config.get("Is25ns", true);
    m_isAtlfast = config.get("IsAtlfast", false);
    m_jetContainerKey = config.get("JetContainerKey", "xxxx");
    m_tauContainerKey = config.get("TauContainerKey", "xxxx");
    m_electronContainerKey = config.get("ElectronContainerKey", "xxxx");
    m_photonContainerKey = config.get("PhotonContainerKey", "xxxx");
    m_suffix = config.get("suffix", "");
    m_electronContainerKey = config.get("ElectronContainerKey", "ElectronCollection");
    m_photonContainerKey = config.get("PhotonContainerKey", "PhotonCollection");
    m_useSmearedJets = config.get("UseSmearedJets", false);
    m_doSystematics = config.get("DoSystematics", false);
    if ( m_useSmearedJets && m_doSystematics ) { 
        throw std::logic_error("Cannot use jet smearing and systematics variations at the same time");
    }
    m_photonInOR = config.get("PhotonInOR", false);
    //m_period = periodFromString(config.get("Period","p13tev"));
    //if ( m_period == p7tev ) throw(std::domain_error("ObjectBuilder does not support the 7tev run period"));
    //if ( m_period == p8tev ) throw(std::domain_error("Due to interface changes in SUSYTools ObjectBuilder no longer supports the 8tev run period"));

    m_derivationTag = Utils::derivationTagFromString(config.get("DerivationTag", ""));
    if ( m_derivationTag == DerivationTag::INVALID_Derivation ) {
        throw(std::domain_error("ObjectBuilder: invalid derivation tag specified"));
    }

    m_JESNuisanceParameterSet = config.get("JESNuisanceParameterSet", 0);
    m_SystMatch = config.getVString("SystMatch"); 
}

ObjectBuilder::~ObjectBuilder() {
    //FIXME crash in SUSYObjDef_xAOD destructor
    if ( m_SUSYObjTool ) 
        delete m_SUSYObjTool;
}

void  ObjectBuilder::inputFileOpened(TFile *file) {
    std::cout << "Opened input file " << file->GetName() << std::endl;
    // SUSYTools initialisation must be delayed until we have a TEvent associated
    // with a file due to xAODConfigTool 
    if ( !m_SUSYObjTool ) {
        initSUSYTools();
    }
}

void ObjectBuilder::initSUSYTools() {
    // SUSYObjDef_xAOD needs a TEvent to be defined, moved from constructor to begin()
    m_SUSYObjTool = new ST::SUSYObjDef_xAOD( m_photonInOR ? "ZLST_GAMMA" : "ZLST");
    m_SUSYObjTool->msg().setLevel( MSG::WARNING);

    // Tell SUSYTools what we're running on
    ST::SettingDataSource datasource = m_isData ? ST::Data : (m_isAtlfast ? ST::AtlfastII : ST::FullSim);
    m_SUSYObjTool->setProperty("DataSource", datasource).ignore();
    m_SUSYObjTool->setProperty("METTauTerm", "").ignore();
    //m_SUSYObjTool->setProperty("Is8TeV", false).ignore(); //TODO

    // Check if we have a valid jet type
    xAOD::JetInput::Type jetType = Utils::JetTypeFromString(m_jetContainerKey);
    if ( jetType == xAOD::JetInput::Uncategorized ) {
        throw std::domain_error("Could not identify JetType");
    }
    m_SUSYObjTool->setProperty("JetInputType", jetType).ignore();
    m_SUSYObjTool->setProperty("JESNuisanceParameterSet", m_JESNuisanceParameterSet).ignore();
    //m_SUSYObjTool->setProperty("DoJetAreaCalib", true).ignore(); //TODO
    //m_SUSYObjTool->setProperty("DoJetGSCCalib", true).ignore(); //TODO

    /* if ( m_derivationTag == p1872 ) {
       m_SUSYObjTool->setProperty("METInputCont","MET_RefFinalFix").ignore();
       m_SUSYObjTool->setProperty("METInputMap","METMap_RefFinalFix").ignore();
       } */

    // Photon selection
    if ( m_photonInOR ) { 
        m_SUSYObjTool->setProperty("DoPhotonOR", true); 
    }
    m_SUSYObjTool->setProperty("PhotonIsoWP", "Cone20").ignore();
    m_SUSYObjTool->setProperty("PhotonId", "Loose").ignore();

    // set our own tau selection
    initializeTauSelectionTools();

    if ( !m_SUSYObjTool->initialize().isSuccess() ) {
        throw std::runtime_error("Could not initialise SUSYOBjDef !");
    }

    initializeSystematics();
    for ( const auto& sys : m_SystInfoList ) {
        std::cout << "systematics: " << sys.systset.name()  << std::endl;
    }
}

void ObjectBuilder::initializeTauSelectionTools() {
    // truth tau matching is needed for FillTau
    // https://svnweb.cern.ch/trac/atlasoff/browser/PhysicsAnalysis/TauID/TauAnalysisTools/trunk/doc/README-TauTruthMatchingTool.rst
    if ( !m_isData && m_tauTruthMatchTool.empty() ) {
        TauAnalysisTools::TauTruthMatchingTool* tauTruthMatchTool = new TauAnalysisTools::TauTruthMatchingTool("TauTruthMatchingTool");
        tauTruthMatchTool->setProperty("WriteTruthTaus",true);
        tauTruthMatchTool->initialize().ignore();
        m_tauTruthMatchTool = tauTruthMatchTool;
    }

    TauAnalysisTools::TauSelectionTool* tauSelTool;
    TauAnalysisTools::TauEfficiencyCorrectionsTool* tauEffTool;

    tauSelTool = new TauAnalysisTools::TauSelectionTool( m_photonInOR ? "TauSelectionTool_GAMMA" : "TauSelectionTool");
    tauSelTool->msg().setLevel( MSG::WARNING);
    //tauSelTool->msg().setLevel( MSG::VERBOSE);

    std::vector<double> vAbsEtaRegion = {0, 1.37, 1.52, 2.5};
    std::vector<size_t> vNTracks = {1, 3};

    tauSelTool->setProperty("PtMin", 20. ).ignore(); // pt in GeV
    tauSelTool->setProperty("AbsEtaRegion", vAbsEtaRegion).ignore();
    tauSelTool->setProperty("AbsCharge", 1.).ignore();
    tauSelTool->setProperty("NTracks", vNTracks).ignore();
    tauSelTool->setProperty("JetIDWP", int(TauAnalysisTools::JETIDBDTMEDIUM)).ignore();

    tauSelTool->setProperty(
            "SelectionCuts",
            (int) TauAnalysisTools::SelectionCuts(TauAnalysisTools::CutPt |
                TauAnalysisTools::CutAbsEta    |
                TauAnalysisTools::CutAbsCharge |
                TauAnalysisTools::CutNTrack    |
                TauAnalysisTools::CutJetIDWP )
            ).ignore();

    tauSelTool->initialize().ignore();
    m_tauSelTool = tauSelTool;

    tauEffTool = new TauAnalysisTools::TauEfficiencyCorrectionsTool("TauEfficiencyCorrectionsTool"+m_suffix,tauSelTool);
    tauEffTool->msg().setLevel( MSG::WARNING);
    //tauEffTool->msg().setLevel( MSG::VERBOSE);
    tauEffTool->setProperty("EfficiencyCorrectionType", (int)TauAnalysisTools::SFJetID).ignore();
    tauEffTool->setProperty("SysDirection", 1).ignore();
    tauEffTool->initialize().ignore();
    m_tauEffTool = tauEffTool;

    m_SUSYObjTool->setProperty("TauSelectionTool", m_tauSelTool).ignore();
    m_SUSYObjTool->setProperty("TauEfficiencyCorrectionsTool", m_tauEffTool).ignore();

    const CP::SystematicSet& recommendedSystematics = m_tauEffTool->recommendedSystematics();
    std::vector<CP::SystematicSet> systSetList;
    systSetList.reserve(recommendedSystematics.size()*2); // allow for continuous systematics
    systSetList.push_back(CP::SystematicSet());
    for(const auto& syst : recommendedSystematics){
        systSetList.push_back(CP::SystematicSet());
        systSetList.back().insert(syst);
    }
    m_tauEffSystSetList = systSetList;

}

void ObjectBuilder::initializeSystematics() {
    out() << "ObjectBuilder::initializeSystematics()" << std::endl;

    // Do we actually have systematics?
    if ( !m_doSystematics ) {
        initializeNoSystematics();
        return;
    }

    // If we don't select anything, use everything
    std::vector<ST::SystInfo> sysInfos = m_SUSYObjTool->getSystInfoList();
    if (  m_SystMatch.empty() ) {
        m_SystInfoList = sysInfos;
        for ( const auto& sys: sysInfos){
            const CP::SystematicSet& systSet = sys.systset;
            out() << " => Will use systematic " << systSet.name() << std::endl;
        }
        return;
    } 

    for ( const auto & sys : sysInfos ) {
        const CP::SystematicSet& systSet = sys.systset;
        std::string name = systSet.name(); 
        bool matched = false;
        if ( name == "" ) {
            matched = true;
        } else {
            // check if name matches wildcard expression
            for ( const auto& wildcardexp : m_SystMatch ){
                TRegexp re = TRegexp(wildcardexp.c_str(), kTRUE);
                Ssiz_t l;
                if ( re.Index(name,&l) >= 0 ) {
                    matched = true;
                    break;
                }
            }
        }
        if ( matched ) {
            m_SystInfoList.push_back(sys);
            out() << " => Will use systematic " << name << std::endl;
        } else {
            out() << " => Will _not_ use systematic " << name << std::endl;
        }
    }
}

void ObjectBuilder::initializeNoSystematics() {
    out() << "ObjectBuilder::initializeNoSystematics()" << std::endl;
    // fill with default "no systematics"
    ST::SystInfo infodef;
    infodef.affectsKinematics = false;
    infodef.affectsWeights = false;
    infodef.affectsType = ST::Unknown;
    m_SystInfoList.push_back(infodef);
}

bool ObjectBuilder::processEvent(xAOD::TEvent& event) {

    out() << "=============================" << std::endl;
    out() << "ObjectBuilder::processEvent()" << std::endl;
    out() << "=============================" << std::endl;

    // for muon trigger SF
    if ( ! m_SUSYObjTool->setRunNumber(267639).isSuccess() ) {
        throw std::runtime_error("Could not set reference run number in SUSYTools !");
    }

    // active storage to put the physics object collections
    xAOD::TStore* store = xAOD::TActiveStore::store();

    // event info
    const xAOD::EventInfo* eventInfo = 0;
    if ( ! event.retrieve( eventInfo, "EventInfo").isSuccess() ) {
        throw std::runtime_error("ObjectBuilder: Could not retrieve EventInfo");
    }

    // make to use no systematics
    m_SUSYObjTool->resetSystematics();

/*
    ////////
    // Taus
    ////////
    std::pair< xAOD::TauJetContainer*, xAOD::ShallowAuxContainer* > susytaus = std::make_pair< xAOD::TauJetContainer*, xAOD::ShallowAuxContainer* >(NULL,NULL);
    const xAOD::TauJetContainer* taus = 0;
    if (!event.retrieve(taus, m_tauContainerKey).isSuccess()){
        throw std::runtime_error("Could not retrieve TauJetContainer");
    }

    xAOD::TauJet::Decorator<float> dec_SFJetID("SFJetID");
    xAOD::TauJet::Decorator<float> dec_SFJetIDStatUp("SFJetIDStatUp");
    xAOD::TauJet::Decorator<float> dec_SFJetIDStatDown("SFJetIDStatDown");
    xAOD::TauJet::Decorator<float> dec_SFJetIDSystUp("SFJetIDSystUp");
    xAOD::TauJet::Decorator<float> dec_SFJetIDSystDown("SFJetIDSystDown");

    susytaus = xAOD::shallowCopyContainer(*taus);
    xAOD::TauJetContainer::iterator tau_itr = susytaus.first->begin();
    xAOD::TauJetContainer::iterator tau_end = susytaus.first->end();

    if ( !m_isData ) {
        m_tauTruthMatchTool->initializeEvent();
    }

    for( ; tau_itr != tau_end; ++tau_itr ) {
        if ( !m_isData ) {
            m_tauTruthMatchTool->getTruth( **tau_itr);
        }
        if ( ! m_SUSYObjTool->FillTau( **tau_itr).isSuccess() ) { 
            throw std::runtime_error("Error in FillTau");
        }

        if ( !m_isData && (*tau_itr)->auxdecor<char>("baseline") == 1 ){
            for ( const auto& syst : m_tauEffSystSetList ){
                // one by one apply systematic variation 
                if (m_tauEffTool->applySystematicVariation(syst) != CP::SystematicCode::Ok){
                    throw std::runtime_error("Could not configure for systematic variation" );
                } else {
                    m_tauEffTool->applyEfficiencyScaleFactor( **tau_itr );
                    std::string systName = syst.name();
                    float sf = (float)( *tau_itr )->auxdata< double >("TauScaleFactorJetID");
                    //std::cout<<"TauScaleFactorJetID syst:"<<systName<<" "<<sf<<std::endl;
                    if( systName == "" )                               dec_SFJetID( **tau_itr )         = sf;
                    else if( systName == "TAUS_EFF_JETID_STAT__1up"   ) dec_SFJetIDStatUp( **tau_itr )   = sf;
                    else if( systName == "TAUS_EFF_JETID_STAT__1down" ) dec_SFJetIDStatDown( **tau_itr ) = sf;
                    else if( systName == "TAUS_EFF_JETID_SYST__1up"   ) dec_SFJetIDSystUp( **tau_itr )    = sf;
                    else if( systName == "TAUS_EFF_JETID_SYST__1down" ) dec_SFJetIDSystDown( **tau_itr )  = sf;
                }

                CP::SystematicSet defaultSet;
                if(m_tauEffTool->applySystematicVariation(defaultSet) != CP::SystematicCode::Ok){
                    throw std::runtime_error("Could not configure TauEfficiencyCorrectionsTool for default systematic setting");
                }
            }
        }
    }

    if ( ! store->record(susytaus.first, "SUSYTaus"+m_suffix).isSuccess() ) {
        throw std::runtime_error("Could not store SUSYTaus"+m_suffix);
    }
    if ( ! store->record(susytaus.second, "SUSYTaus"+m_suffix+"Aux.").isSuccess()) {
        throw std::runtime_error("Could not store SUSYTaus"+m_suffix+"Aux.");
    }
*/

    // loop over systematics variations
    std::vector<float>* event_weights = new std::vector<float>;
    std::vector<std::string>* event_weights_names = new std::vector<std::string>;
    std::vector<CP::SystematicSet>* sys_variations_kinematics = new std::vector<CP::SystematicSet>;
    for (const auto& sys : m_SystInfoList ) {
        const CP::SystematicSet& systSet = sys.systset;
        std::string tag = "";
        if ( ! systSet.empty() ) {
            tag = "_"+systSet.name()+"_";
        }

        if ( m_SUSYObjTool->applySystematicVariation(systSet) != CP::SystematicCode::Ok) {
            throw std::runtime_error("Could not apply systematics "+systSet.name());
        }

        bool storeVariation = (systSet.name() == "" || sys.affectsKinematics);

        ////////
        // Jets
        ////////
        xAOD::JetContainer* jets = 0;
        xAOD::ShallowAuxContainer* jets_aux = 0;
        if (! m_SUSYObjTool->GetJets(jets, jets_aux, false).isSuccess() ) {
            throw std::runtime_error("Could not retrieve Jets");
        }
        if ( storeVariation ) {
            if ( ! store->record(jets,"SUSYJets"+m_suffix+tag).isSuccess() ) {
                throw std::runtime_error("Could not store SUSYJets"+m_suffix+tag);
            }
            if ( ! store->record(jets_aux,"SUSYJets"+m_suffix+tag+"Aux.").isSuccess() ) {
                throw std::runtime_error("Could not store SUSYJets"+m_suffix+tag+"Aux.");
            }
        }
        
        out() <<  "Jets"+m_suffix+tag+" jets " << std::endl;
        for( const auto& jet: *jets) {
            out() << " jet " << jet->pt() << " " << jet->eta()
                << " " << jet->phi() 
                << " baseline " <<  (int) jet->auxdata<char>("baseline") 
                << " signal " <<  (int) jet->auxdata<char>("signal") 
                << std::endl;
        }
        
        ////////
        // Taus
        ////////
        xAOD::TauJetContainer* taus = 0;
        xAOD::ShallowAuxContainer* taus_aux = 0;
        if (! m_SUSYObjTool->GetTaus(taus, taus_aux, false).isSuccess() ) {
            throw std::runtime_error("Could not retrieve taus");
        }
        if ( storeVariation ) {
            if ( ! store->record(taus,"SUSYTaus"+m_suffix+tag).isSuccess() ) {
                throw std::runtime_error("Could not store SUSYTaus"+m_suffix+tag);
            }
            if ( ! store->record(taus_aux,"SUSYTaus"+m_suffix+tag+"Aux.").isSuccess() ) {
                throw std::runtime_error("Could not store SUSYTaus"+m_suffix+tag+"Aux.");
            }
        }
        
        float tauSF = 1.0;
        out() <<  "Taus"+m_suffix+tag+" taus " << std::endl;
        for ( const auto& tau : *taus ) {
            if ( !m_isData && tau->auxdata<char>("signal") != 0 ) {
                float sf = m_SUSYObjTool->GetSignalTauSF(*tau);
                tau->auxdecor<float>("sf") = sf;
                tauSF *= sf;
            } else {
                tau->auxdecor<float>("sf") = 1.0;
            }

            out() << " tau " << tau->pt() << " " << tau->eta()
                << " " << tau->phi() 
                << " baseline " <<  (int) tau->auxdata<char>("baseline") 
                << " signal " <<  (int) tau->auxdata<char>("signal") 
                << std::endl;
        }

        /////////
        // Muons
        /////////
        xAOD::MuonContainer* muons = 0;
        xAOD::ShallowAuxContainer* muons_aux = 0;
        if (! m_SUSYObjTool->GetMuons(muons,muons_aux,false).isSuccess() ) {
            throw std::runtime_error("Could not retrieve Muons");
        }
        if ( storeVariation ) {
            if ( ! store->record(muons,"SUSYMuons"+m_suffix+tag).isSuccess() ) {
                throw std::runtime_error("Could not store SUSYMuons"+m_suffix+tag);
            }
            if ( ! store->record(muons_aux,"SUSYMuons"+m_suffix+tag+"Aux.").isSuccess() ) {
                throw std::runtime_error("Could not store SUSYMuons"+m_suffix+tag+"Aux.");
            }
        }
        out() <<  "Muons"+m_suffix+tag+" muons " << std::endl;
        for ( const auto& mu : *muons ) {
            // declare calo and forward muons non baseline so they don't get used in MET
            if ( mu->muonType() != xAOD::Muon::Combined &&
                    mu->muonType() != xAOD::Muon::MuonStandAlone && 
                    mu->muonType() != xAOD::Muon::SegmentTagged ) {
                mu->auxdecor<char>("baseline") = 0;
            }
               out() << " Muon " << mu->pt() << " " << mu->eta()
               << " " << mu->phi() 
               << " bad " <<  (int) mu->auxdata<char>("bad") 
               << " baseline " <<  (int) mu->auxdata<char>("baseline") 
               << " signal " <<  (int) mu->auxdata<char>("signal") 
               <<std::endl;
        }

        /////////////
        // Electrons
        /////////////
        xAOD::ElectronContainer* electrons = 0;
        xAOD::ShallowAuxContainer* electrons_aux = 0;
        if (! m_SUSYObjTool->GetElectrons(electrons, electrons_aux, false).isSuccess() ) {
            throw std::runtime_error("Could not retrieve Electrons");
        }
        if ( storeVariation ) {
            if ( ! store->record(electrons, "SUSYElectrons"+m_suffix+tag).isSuccess() ) {
                throw std::runtime_error("Could not store SUSYElectrons"+m_suffix+tag);
            }
            if ( ! store->record(electrons_aux, "SUSYElectrons"+m_suffix+tag+"Aux.").isSuccess() ) {
                throw std::runtime_error("Could not store SUSYElectrons"+m_suffix+tag+"Aux.");
            }
        }
        out() <<  "Electrons"+m_suffix+tag+" electrons " << std::endl;
        for ( const auto& el : *electrons ) {
               out() << " Electron " << el->pt() << " " << el->eta()
                   << " " << el->phi() 
                   << " baseline " <<  (int) el->auxdata<char>("baseline") 
                   << " signal " <<  (int) el->auxdata<char>("signal") 
                   << std::endl;
             
        }

        ///////////
        // Photons
        ///////////
        xAOD::PhotonContainer* photons = 0;
        xAOD::ShallowAuxContainer* photons_aux = 0;
        if (! m_SUSYObjTool->GetPhotons(photons, photons_aux, false).isSuccess() ) {
            throw std::runtime_error("Could not retrieve Photons");
        }
        if ( storeVariation ) {
            if ( ! store->record(photons,"SUSYPhotons"+m_suffix+tag).isSuccess() ) {
                throw std::runtime_error("Could not store SUSYPhotons"+m_suffix+tag);
            }
            if ( ! store->record(photons_aux,"SUSYPhotons"+m_suffix+tag+"Aux.").isSuccess() ) {
                throw std::runtime_error("Could not store SUSYPhotons"+m_suffix+tag+"Aux.");
            }
        }
        
        out() <<  "Photons"+m_suffix+tag+" photons " << std::endl;
        float phSF = 1;
        for ( const auto& ph : *photons ) {
            if ( !m_isData && ph->auxdata<char>("signal") != 0 ) {
                float sf = m_SUSYObjTool->GetSignalPhotonSF(*ph);
                ph->auxdecor<float>("sf") = sf;
                phSF *= sf;
            } else {
                ph->auxdecor<float>("sf") = 1.0;
            }
            
            out() << " Photon " << ph->pt() << " " << ph->eta()
                << " " << ph->phi() 
                << " baseline " <<  (int) ph->auxdata<char>("baseline") 
                << " signal " <<  (int) ph->auxdata<char>("signal") 
                << std::endl;
        }
        eventInfo->auxdecor<float>("phSF") = phSF ; 

        // Overlap removal
        if ( m_photonInOR ) {
            if ( ! m_SUSYObjTool->OverlapRemoval(electrons, muons, jets, photons, taus).isSuccess() ) {
                throw std::runtime_error("Error in OverlapRemoval");
            }
        } else {
            if ( ! m_SUSYObjTool->OverlapRemoval(electrons, muons, jets, 0, taus).isSuccess() ) {
                throw std::runtime_error("Error in OverlapRemoval");
            }
        }

        // GetTotalMuonSF also test for OR
        float muSF = 1.f;
        if ( !m_isData ) {
            muSF = (float) m_SUSYObjTool->GetTotalMuonSF(*muons, true, true, "HLT_mu20_iloose_L1MU15_OR_HLT_mu50");
        }
        eventInfo->auxdecor<float>("muSF") = muSF ; 

        // idem for GetTotalElectronSF
        float elSF = 1.f;
        if ( !m_isData ) {
            elSF = (float) m_SUSYObjTool->GetTotalElectronSF(*electrons);
        }
        eventInfo->auxdecor<float>("elSF") = elSF ; 

        xAOD::MissingETContainer* rebuiltMET = new xAOD::MissingETContainer();
        xAOD::MissingETAuxContainer* rebuiltMET_aux = new xAOD::MissingETAuxContainer();
        rebuiltMET->setStore(rebuiltMET_aux);
        if ( storeVariation ) {
            if ( ! store->record(rebuiltMET, "MET_ZLFV"+m_suffix+tag).isSuccess() ) {
                throw std::runtime_error("Unable to store MissingETContainer with tag MET_ZLFV"+m_suffix+tag);
            }
            if ( ! store->record(rebuiltMET_aux, "MET_ZLFV"+m_suffix+tag+"Aux.").isSuccess() ) {
                throw std::runtime_error("Unable to store MissingETAuxContainer with tag MET_ZLFV"+m_suffix+tag+"Aux");
            }
        }

        if ( ! m_SUSYObjTool->GetMET(*rebuiltMET, jets, electrons, muons, photons, taus, true, true, 0).isSuccess() ) {
            throw std::runtime_error("Error in GetMET");
        }

        // Do we have a rebuilt MET?
        xAOD::MissingETContainer::const_iterator met_it = rebuiltMET->find("Final");
        if ( met_it == rebuiltMET->end() ) { 
            throw std::runtime_error("Could not find Final MET after running  GetMET");
        }

        // now retrieve the result
        TVector2* MissingET = new TVector2(0.,0.);
        MissingET->Set((*met_it)->mpx(), (*met_it)->mpy());
        if ( ! store->record(MissingET, "SUSYMET"+m_suffix+tag).isSuccess() ) {
            throw std::runtime_error("Could not store SUSYMET with tag SUSYMET"+m_suffix+tag);
        }

        if ( sys.affectsKinematics || systSet.name() == "" ) {
            sys_variations_kinematics->push_back(systSet);
        }
        
        if ( sys.affectsWeights || systSet.name() == "" ) {
            event_weights->push_back(elSF * phSF * muSF * tauSF);
            event_weights_names->push_back(systSet.name());
        }

        // delete shallow copies if not stored
        if ( !storeVariation ) {
            delete jets;
            delete jets_aux;
            delete taus;
            delete taus_aux;
            delete muons;
            delete muons_aux;
            delete electrons;
            delete electrons_aux;
            delete photons;
            delete photons_aux;
            delete rebuiltMET;
            delete rebuiltMET_aux;
        }
    }

    // store variations that affect kinematics -> need to rerun SR+CR
    if ( ! store->record(sys_variations_kinematics,"sys_variations_kinematics"+m_suffix).isSuccess() ) {
        throw std::runtime_error("Could not store sys_variations_kinematics"+m_suffix);
    }
    // store event weights variations
    if ( ! store->record(event_weights_names,"event_weights_names"+m_suffix).isSuccess() ) {
        throw std::runtime_error("Could not store event_weights_names"+m_suffix);
    }
    if ( ! store->record(event_weights,"event_weights"+m_suffix).isSuccess() ) {
        throw std::runtime_error("Could not store event_weights"+m_suffix);
    }

    if ( m_suffix == "" ) {
        fillTriggerInfo(event);
    }

    return true;
}

void ObjectBuilder::fillTriggerInfo(xAOD::TEvent& event) const {
    // TODO: update list of triggers
    static std::vector<std::string> trigNames = {
        "L1_XE50",
        "L1_XE70",
        "HLT_xe70",
        "HLT_xe70_pueta",
        "HLT_xe100",
        "HLT_xe100_pueta",
        "HLT_e28_tight_iloose",
        "HLT_e60_medium",
        "HLT_mu26_imedium",
        "HLT_j30_xe10_razor170",
        "HLT_xe70_tc_em",
        "HLT_xe70_tc_lcw",
        "HLT_xe70_mht",
        "HLT_xe70_pufit",
        "HLT_xe100_tc_em",
        "HLT_xe100_tc_lcw",
        "HLT_xe100_mht",
        "HLT_xe100_pufit",
        "HLT_3j175",
        "HLT_4j85",
        "HLT_5j85",
        "HLT_6j25",
        "HLT_6j45_0eta240",
        "HLT_6j55_0eta240_L14J20",
        "HLT_7j45",
        "L1_2J15",
        "HLT_2j55_bloose",
        "HLT_j80_xe80",
        "HLT_e24_lhmedium_iloose_L1EM18VH",
        "HLT_e24_lhmedium_iloose_L1EM20VH",
        "HLT_e60_lhmedium",
        "HLT_mu20_iloose_L1MU15",
        "HLT_mu40",
        "HLT_mu50",
        "HLT_g120_loose",
        "HLT_g120_lhloose",
        "HLT_mu18",
        "HLT_e17_lhloose_L1EM15",
        "HLT_e17_loose_L1EM15",
        "HLT_mu14_iloose"
    };

    const xAOD::EventInfo* eventInfo = 0;
    if ( !event.retrieve( eventInfo, "EventInfo").isSuccess() ) {
        throw std::runtime_error("ObjectBuilder: Could not retrieve EventInfo");
    }

    std::bitset<32> triggers;
    for ( size_t i = 0; i < trigNames.size(); i++ ) {
        char pass =  m_SUSYObjTool->IsTrigPassed(trigNames[i]);
        triggers[i] = pass;
        eventInfo->auxdecor<char>(trigNames[i]) = pass;
    }

    xAOD::TStore* store = xAOD::TActiveStore::store();
    unsigned long* triggerSet = new unsigned long;
    *triggerSet =  triggers.to_ulong();
    if ( !store->record(triggerSet, "triggerbits").isSuccess() ) {
        throw std::runtime_error("Could not store trigger bits");
    }
    std::cout << " trigger " << triggers << " " << *triggerSet << std::endl;
}



ClassImp(ObjectBuilder);


