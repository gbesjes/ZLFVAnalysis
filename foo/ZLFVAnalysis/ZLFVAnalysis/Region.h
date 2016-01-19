// vim: ts=4 sw=4
#ifndef HAVE_REGION_H
#define HAVE_REGION_H

#include "cafe/Processor.h"
//#include "ZLFVAnalysis/ZeroLeptonNTVars.h"
//#include "ZLFVAnalysis/PhysObjProxyFiller.h"
//#include "ZLFVAnalysis/PhysObjProxyFillerTruth.h"
//#include "ZLFVAnalysis/ZeroLeptonUtils.h"
//#include "ZLFVAnalysis/ZeroLeptonRunPeriod.h"
//#include "ZLFVAnalysis/ZeroLeptonCutVal.h"
//#include "ZLFVAnalysis/PhysObjProxyUtils.h"

#include "ZLFVAnalysis/Utils.h"
#include "ZLFVAnalysis/Counter.h"

#include <string>
#include <map>

#include "TTree.h"

class Region : public cafe::Processor {
  public:
    Region(const char *name);
    ~Region();
    void begin();
    void finish();
    bool processEvent(xAOD::TEvent& event);

  private:
    TTree* bookTree(const std::string& name);
    TTree* getTree(const std::string& name);

    TTree* m_tree;
    std::string m_stringRegion;
    bool m_doSmallNtuple;
    bool m_fillTRJigsawVars;
    bool m_fillReclusteringVars;
    bool m_doRecl;
    bool m_IsData;
    bool m_IsTruth;
    bool m_IsSignal;
    bool m_DoSystematics;
    RunPeriod m_period;

    /*NTVars m_ntv;
    NTExtraVars m_extrantv;
    NTRJigsawVars m_rjigsawntv;
    NTReclusteringVars m_RTntv;
    NTTheoryVars m_theoryntv;
    NTISRVars m_isrntv;
    */

    std::string m_suffix;
    std::string m_suffixRecl;
    std::string m_suffixSyst;
    /*
    PhysObjProxyFiller* m_physobjsFiller;
    PhysObjProxyFillerTruth* m_physobjsFillerTruth;
    ZeroLeptonCutVal m_cutVal;
    PhysObjProxyUtils m_proxyUtils;
    ZeroLeptonUtils m_ZLUtils;
    */

    Counter* m_counter;
    CounterRepository m_counterRepository;

    // TODO : an unordered_map would be faster
    std::map<std::string,TTree*> m_treeRepository;

    DerivationTag m_derivationTag;
  public:
    ClassDef(Region,0);
};

#endif
