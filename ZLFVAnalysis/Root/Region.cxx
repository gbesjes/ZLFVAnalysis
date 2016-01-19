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
    m_derivationTag(DerivationTag::INVALID_Derivation)
{
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
    }
    if ( m_period == RunPeriod::INVALID ) {
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

Region::~Region()
{
    if ( !m_DoSystematics && m_counter ) delete m_counter;
    //if ( m_physobjsFiller ) delete m_physobjsFiller;
    //if ( m_physobjsFillerTruth ) delete m_physobjsFillerTruth;
}

TTree* Region::bookTree(const std::string& treename)
{
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
        pos = m_treeRepository.insert(std::make_pair(treename,bookTree(treename))).first;
    }
    return pos->second;
}

void Region::begin()
{
    std::string sSR = m_stringRegion;
    if(m_doSmallNtuple) { 
        sSR+="NT";
    }

    if ( m_DoSystematics ) {
        m_counterRepository = CounterRepository("ZeroLeptonCounter"+m_stringRegion,m_IsSignal,getDirectory());
    }
    else {
        m_counter = new Counter("ZeroLeptonCounter"+m_stringRegion,40,m_IsSignal);
        if(m_doSmallNtuple) m_tree = bookTree(sSR);
    }

    //if (  m_fillTRJigsawVars ) {    m_proxyUtils.RJigsawInit(); }

}


bool Region::processEvent(xAOD::TEvent& event)
{
    // access the transient store
    xAOD::TStore* store = xAOD::TActiveStore::store();
    std::string systag = "";
    if ( m_DoSystematics ) {
        CP::SystematicSet* currentSyst = 0;
        if ( ! store->retrieve(currentSyst, "CurrentSystematicSet").isSuccess() ) throw std::runtime_error("Could not retrieve CurrentSystematicSet");
        std::string sysname = currentSyst->name();
        if (sysname != "" ) systag = "_"+sysname+"_";
        m_counter = m_counterRepository.counter(sysname);
        if (sysname == "" ) {
            m_tree = getTree(m_stringRegion+"NT");
        }
        else {
            m_tree = getTree(m_stringRegion+"NT_"+sysname);
            //m_physobjsFiller->setSuffix(m_suffix+systag);
        }
    }

    // eventInfo
    const xAOD::EventInfo* eventInfo = 0;
    if ( ! event.retrieve( eventInfo, "EventInfo").isSuccess() ) throw std::runtime_error("Could not retrieve EventInfo");
    uint32_t RunNumber = eventInfo->runNumber();
    unsigned long long EventNumber = eventInfo->eventNumber();
    uint32_t LumiBlockNumber = eventInfo->lumiBlock();
    uint32_t mc_channel_number = 0;
    if ( ! m_IsData ) mc_channel_number = eventInfo->mcChannelNumber();

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
    }
    else {
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
        if ( !store->retrieve<unsigned int>(pveto,"mcVetoCode").isSuccess() ) throw std::runtime_error("could not retrieve mcVetoCode");
        veto = *pveto;
        bool* mcaccept = 0;
        if ( !store->retrieve<bool>(mcaccept,"mcAccept").isSuccess() ) throw std::runtime_error("could not retrieve mcaccept");
        if ( ! *mcaccept ) return true;
    }
    m_counter->increment(weight,incr++,"Truth filter",trueTopo);

    // Good run list
    if ( m_IsData ) {
        bool* passGRL = 0;
        if ( !store->retrieve<bool>(passGRL,"passGRL").isSuccess() ) throw std::runtime_error("could not retrieve passGRL");
        if ( ! *passGRL ) return true;
    }
    m_counter->increment(weight,incr++,"GRL",trueTopo);

    // HFor veto
    // FIXME : to be implemented ... not in xAOD yet
    m_counter->increment(weight,incr++,"hfor veto",trueTopo);

    // Trigger selection
    if(! m_IsTruth){
        if( !(int)eventInfo->auxdata<char>("HLT_xe70")==1) return true;
    }
    m_counter->increment(weight,incr++,"Trigger",trueTopo);

    // These jets have overlap removed
    //std::vector<JetProxy> good_jets, bad_jets, b_jets, c_jets, good_jets_recl;
    std::vector<float> vD2;
    if(! m_IsTruth){
        //m_physobjsFiller->FillJetProxies(good_jets,bad_jets,b_jets);
        if(m_doRecl){
            //m_physobjsFiller->FillJetReclProxies(good_jets_recl,vD2);
        }
    }
    if(m_IsTruth){
        //m_physobjsFillerTruth->FillJetProxies(good_jets,b_jets);
    }
    std::vector<float> btag_weight(7,1.); // not implemented in SUSYTools
    std::vector<float> ctag_weight(7,1.); // not implemented in SUSYTools

    // isolated_xxx have overlap removed
    //std::vector<ElectronProxy> baseline_electrons, isolated_baseline_electrons, isolated_signal_electrons;
    if(! m_IsTruth){
        //m_physobjsFiller->FillElectronProxies(baseline_electrons, isolated_baseline_electrons, isolated_signal_electrons);
    }
    //std::vector<ElectronTruthProxy> baseline_electrons_truth, isolated_baseline_electrons_truth, isolated_signal_electrons_truth;
    if(m_IsTruth){
        //m_physobjsFillerTruth->FillElectronProxies(baseline_electrons_truth, isolated_baseline_electrons_truth, isolated_signal_electrons_truth);
    }
    // isolated_xxx have overlap removed
    //std::vector<MuonProxy> baseline_muons, isolated_baseline_muons, isolated_signal_muons;
    if(! m_IsTruth){
        //m_physobjsFiller->FillMuonProxies(baseline_muons, isolated_baseline_muons, isolated_signal_muons);
    }
    //std::vector<MuonTruthProxy> baseline_muons_truth, isolated_baseline_muons_truth, isolated_signal_muons_truth;
    if(m_IsTruth){
        //m_physobjsFillerTruth->FillMuonProxies(baseline_muons_truth, isolated_baseline_muons_truth, isolated_signal_muons_truth);
    }
    //std::vector<TauProxy> baseline_taus, signal_taus;
    if(! m_IsTruth){
        //m_physobjsFiller->FillTauProxies(baseline_taus, signal_taus);
    }

    // Boson Tagging

    std::vector<float> vReclJetMass ;
    std::vector<float> vReclJetPt;
    std::vector<float> vReclJetEta;
    std::vector<float> vReclJetPhi;
    std::vector<bool> visWtight ;
    std::vector<bool> visWmedium ;
    std::vector<bool> visZtight ;
    std::vector<bool> visZmedium ;

    if(m_doRecl){
        //for ( size_t j0=0; j0<good_jets_recl.size(); j0++){
            //vReclJetMass.push_back(good_jets_recl[j0].M());
            //vReclJetPt.push_back(good_jets_recl[j0].Pt());
            //vReclJetEta.push_back(good_jets_recl[j0].Eta());
            //vReclJetPhi.push_back(good_jets_recl[j0].Phi());

            //float jetpt = good_jets_recl[j0].Pt();
            //float jetm  = good_jets_recl[j0].M();
            //float jetD2 = vD2.at(j0);

            //BosonTagging BT;

            //bool isWmedium = BT.ReturnTag(1,jetpt,jetm,jetD2);
            //bool isWtight  = BT.ReturnTag(2,jetpt,jetm,jetD2);
            //bool isZmedium = BT.ReturnTag(3,jetpt,jetm,jetD2);
            //bool isZtight  = BT.ReturnTag(4,jetpt,jetm,jetD2);

            //visWmedium.push_back(isWmedium);
            //visWtight.push_back(isWtight);
            //visZmedium.push_back(isZmedium);
            //visZtight.push_back(isZtight);
        //}
    }

    // missing ET
    TVector2* missingET = 0;
    if(!m_IsTruth){
        if ( ! store->retrieve<TVector2>(missingET,"SUSYMET"+m_suffix+systag).isSuccess() ) throw std::runtime_error("could not retrieve SUSYMET"+m_suffix+systag);
    }
    if(m_IsTruth){
        if ( ! store->retrieve<TVector2>(missingET,"TruthMET"+m_suffix).isSuccess() ) throw std::runtime_error("could not retrieve TruthMET"+m_suffix);
    }
    double MissingEt = missingET->Mod();

    // LAr, Tile, reco problems in data
    if ( m_IsData ) {
        bool* badDetectorQuality = 0 ; 
        if ( !store->retrieve<bool>(badDetectorQuality,"badDetectorQuality").isSuccess() ) throw std::runtime_error("could not retrieve badDetectorQuality");
        if ( *badDetectorQuality ) return true;
    }
    m_counter->increment(weight,incr++,"Detector cleaning",trueTopo);

    // MET track
    double MET_Track = -999.;
    double MET_Track_phi = -999.;
    if( !m_IsTruth ){
        //m_ZLUtils.trackMET(event, MET_Track, MET_Track_phi);
    }
    // primary vertex cut
    const xAOD::Vertex* primaryVertex = 0;
    if(! m_IsTruth){
        primaryVertex = Utils::GetPrimaryVertex(event);
        if ( !primaryVertex ||  !( primaryVertex->nTrackParticles() > 2) ) return true;
    }
    m_counter->increment(weight,incr++,"Vertex Cut",trueTopo);

    // 0 lepton
    if( !m_IsTruth ){
        //if ( !isolated_baseline_electrons.empty() ) return true;
        //if ( !isolated_baseline_muons.empty() ) return true;
    }
    if( m_IsTruth ){
        //if ( !isolated_baseline_electrons_truth.empty() ) return true;
        //if ( !isolated_baseline_muons_truth.empty() ) return true;
    }
    m_counter->increment(weight,incr++,"0 Lepton",trueTopo);


    //if (good_jets.size()<1) return true;  
    m_counter->increment(weight,incr++,"At least one jet",trueTopo);

    // jet timing
    std::vector<float> time;
    //m_proxyUtils.EnergyWeightedTime(good_jets,time);

    // MissingET cut
    if( !m_IsTruth ){
        //if (!(MissingEt > m_cutVal.m_cutEtMiss)) return true;
    }
    if( m_IsTruth ){
        //if (!(MissingEt > m_cutVal.m_cutEtMissTruthTest)) return true;
    }

    m_counter->increment(weight,incr++,"MET cut",trueTopo);
    // Leading jet Pt cut
    //if (!(good_jets[0].Pt() > m_cutVal.m_cutJetPt0)) return true; 
    m_counter->increment(weight,incr++,"1 jet Pt > 130 GeV Selection",trueTopo);

    // leave counter to keep the cutflow sequence
    m_counter->increment(weight,incr++,"jet Pt Selection",trueTopo);

    // Calculate variables for ntuple -----------------------------------------
    //double phi_met = TMath::ATan2(missingET->Y(),missingET->X());
    //double minDphi = m_proxyUtils.SmallestdPhi(good_jets,phi_met);
    //double RemainingminDPhi = m_proxyUtils.SmallestRemainingdPhi(good_jets,phi_met);
    ////out() << " minDphi " << minDphi << " " << RemainingminDPhi << std::endl;
    ////out() << " MissingEt " << MissingEt << std::endl;
    ////out() << " jet Pt ";
    ////for ( size_t i = 0; i<good_jets.size(); ++i ) out() << " " << good_jets[i].Pt();
    ////out() << std::endl;
    //double Meff[6];
    //for ( size_t i = 0; i<6; ++i ) {
        //Meff[i] = m_proxyUtils.Meff(good_jets,
                //std::max<size_t>(1,i+1),
                //MissingEt,
                //m_cutVal.m_cutJetPt1,
                //m_cutVal.m_cutJetPt4);
        ////out() << " Meff[" << i <<"] " << Meff[i] << std::endl;
    //}
    //double meffincl = m_proxyUtils.Meff(good_jets,
            //good_jets.size(),
            //MissingEt,
            //m_cutVal.m_cutJetPt4,
            //m_cutVal.m_cutJetPt4);
    //out() << " MeffInc " << meffincl << std::endl;

    // ttbar reweighting not available yet in SUSYOBJDef_xAOD
    float ttbarWeightHT = 1.;
    float ttbarWeightPt2 = 1.;
    float ttbarAvgPt = 0.;
    //float HT = meffincl -  MissingEt;

    // Sherpa MassiveCB W/Z reweighting : not implemented yet in SUSYOBJDef_xAOD
    float WZweight = 1.;
    
    // Fill ISR variables. These vectors have for each jet > 50GeV the ISR variables in them. 
    std::vector<size_t> isr_jet_indices;
    std::vector<std::vector<double> > ISRvars;
    std::vector<double> ISRvar_alpha;
    //m_proxyUtils.GetAlphaISRVar(good_jets,MissingEt,ISRvar_alpha);
    std::vector<double> ISRvar_minPtDistinction;
    //m_proxyUtils.GetMinPtDistinctionISR(good_jets,ISRvar_minPtDistinction);
    std::vector<double> ISRvar_minDeltaFraction;
    //m_proxyUtils.GetMinDeltaFraction(good_jets,ISRvar_minDeltaFraction);
    std::vector<double> ISRvar_minRapidityGap;
    //m_proxyUtils.GetMinRapidityGap(good_jets,ISRvar_minRapidityGap);
    std::vector<double> ISRvar_maxRapidityOtherJets;
    //m_proxyUtils.GetMaxRapidityOtherJets(good_jets,ISRvar_maxRapidityOtherJets);
    std::vector<double> ISRvar_dPhiJetMET;
    //m_proxyUtils.GetdPhiJetMet(good_jets,phi_met,ISRvar_dPhiJetMET);
    //m_proxyUtils.GetISRJet(good_jets,isr_jet_indices,MissingEt,phi_met,"squark",false);  
    ISRvars.push_back(ISRvar_alpha);
    ISRvars.push_back(ISRvar_minPtDistinction);
    ISRvars.push_back(ISRvar_minDeltaFraction);
    ISRvars.push_back(ISRvar_minRapidityGap);
    ISRvars.push_back(ISRvar_maxRapidityOtherJets);
    ISRvars.push_back(ISRvar_dPhiJetMET);
    //std::vector<JetProxy> nonISR_jets = good_jets;
    if (isr_jet_indices.size()==1) {
        //nonISR_jets.erase(nonISR_jets.begin()+isr_jet_indices[0],nonISR_jets.end());
    }


    double mT2=-9; 
    //if (good_jets.size()>=2) mT2 = m_proxyUtils.MT2(good_jets,*missingET);
    double mT2_noISR=-9;
    //if (nonISR_jets.size()>=2) mT2_noISR = m_proxyUtils.MT2(nonISR_jets,*missingET); 
    //out() << " mT2 " << mT2 << " " << mT2_noISR << std::endl; 

    std::map<TString,float> RJigsawVariables;
    if (  m_fillTRJigsawVars ) {
        //m_proxyUtils.CalculateRJigsawVariables(good_jets, 
                //missingET->X(),
                //missingET->Y(),
                //RJigsawVariables,
                //m_cutVal.m_cutRJigsawJetPt);
    }


    //Super Razor variables
    double gaminvRp1 =-999;
    double shatR =-999;
    double mdeltaR =-999;
    double cosptR =-999;
    double Minv2 =-999;
    double Einv =-999;
    double  gamma_R=-999;
    double dphi_BETA_R =-999; 
    double dphi_leg1_leg2 =-999; 
    double costhetaR =-999;
    double dphi_BETA_Rp1_BETA_R=-999;
    double gamma_Rp1=-999;
    double Eleg1=-999;
    double Eleg2=-999; 
    double costhetaRp1=-999;

    //m_proxyUtils.RazorVariables(good_jets, 
            //missingET->X(),
            //missingET->Y(),
            //gaminvRp1 ,
            //shatR ,
            //mdeltaR ,
            //cosptR ,
            //Minv2 ,
            //Einv ,
            //gamma_R,
            //dphi_BETA_R , 
            //dphi_leg1_leg2 , 
            //costhetaR ,
            //dphi_BETA_Rp1_BETA_R,
            //gamma_Rp1,
            //Eleg1,
            //Eleg2, 
            //costhetaRp1);

    double Sp,ST,Ap=-1;
    //m_proxyUtils.ComputeSphericity(good_jets, Sp,ST,Ap);

    if(m_doSmallNtuple) { 
        unsigned int runnum = RunNumber;
        if ( ! m_IsData && ! m_IsTruth) runnum = mc_channel_number;

        std::vector<float> jetSmearSystW;

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
        if (fabs(time[0]) > 5) cleaning += power2;
        power2 *= 2;

        if(!m_IsTruth){
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

        //m_proxyUtils.FillNTVars(m_ntv, runnum, EventNumber, LumiBlockNumber, veto, weight, normWeight, *pileupWeights, genWeight,ttbarWeightHT,ttbarWeightPt2,ttbarAvgPt,WZweight, btag_weight, ctag_weight, b_jets.size(), c_jets.size(), MissingEt, phi_met, Meff, meffincl, minDphi, RemainingminDPhi, good_jets, trueTopo, cleaning, time[0],jetSmearSystW,0, 0., 0.,dPhiBadTile,isNCBEvent,m_IsTruth,baseline_taus,signal_taus);

        if ( systag == ""  && !m_IsTruth ) {
            std::vector<float>* p_systweights = 0;
            if ( ! store->retrieve(p_systweights,"event_weights"+m_suffix).isSuccess() ) throw std::runtime_error("Could not retrieve event_weights"+m_suffix);
            //m_ntv.systWeights = *p_systweights;

            std::vector<float>* p_btagSystweights = 0;
            if ( ! store->retrieve(p_btagSystweights,"btag_weights"+m_suffix).isSuccess() ) throw std::runtime_error("Could not retrieve btag_weights"+m_suffix);
            //m_ntv.btagSystWeights = *p_btagSystweights;
        }

        //m_proxyUtils.FillNTExtraVars(m_extrantv, MET_Track, MET_Track_phi, mT2,mT2_noISR,Ap);

        //if ( m_fillTRJigsawVars ) m_proxyUtils.FillNTRJigsawVars(m_rjigsawntv, RJigsawVariables );

        if(!m_IsTruth && m_fillReclusteringVars)
            //m_proxyUtils.FillNTReclusteringVars(m_RTntv,good_jets,vReclJetMass,vReclJetPt,vReclJetEta,vReclJetPhi,vD2,visWmedium, visWtight, visZmedium, visZtight);

        m_tree->Fill();
    }
    return true;
}

void Region::finish()
{
    if ( m_DoSystematics ) {
        out() << m_counterRepository << std::endl;
    } 
    else {
        out() << *m_counter << std::endl;
    }
}

ClassImp(Region);

