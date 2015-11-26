// vim: ts=4 sw=4
#ifndef HAVE_OBJECTBUILDER_H
#define HAVE_OBJECTBUILDER_H

#include <string>
#include <vector>

#include "TFile.h"

#include "AsgTools/ToolHandle.h"
#include "TauAnalysisTools/TauSelectionTool.h"
#include "TauAnalysisTools/TauEfficiencyCorrectionsTool.h"
#include "TauAnalysisTools/TauTruthMatchingTool.h"

#include "SUSYTools/SUSYObjDef_xAOD.h"
#include "cafe/Processor.h"

#include "ZLFVAnalysis/Utils.h"

//---------------------------------------------------------------------
// ObjectBuilder use the collections of physics objects in the event
// and pass then through SUSYTools that applies calibration and add
// properties (as "decorations" to the objects). The output containers
// are stored in the transient store with keys "SUSYJets", "SUSYMuons",
// "SUSYElectrons", "SUSYPhotons", "SUSYTaus"
//
// If UseSmearedJets is true this is supposed to be called from within
// a loop on smeared iterations from an event.
//---------------------------------------------------------------------
class ObjectBuilder : public cafe::Processor {
    public:
        ObjectBuilder(const char *name);
        ~ObjectBuilder();
        bool processEvent(xAOD::TEvent& event);
        void inputFileOpened(TFile *file);

    private:
        void initSUSYTools();
        void fillTriggerInfo(xAOD::TEvent& event) const;

        void initializeTauSelectionTools();
        void initializeSystematics();
        void initializeNoSystematics();

        ST::SUSYObjDef_xAOD* m_SUSYObjTool;
        
        bool m_isData;
        bool m_is25ns;
        bool m_isAtlfast;
        bool m_useSmearedJets;
        bool m_doSystematics;
        bool m_photonInOR;

        ToolHandle<TauAnalysisTools::ITauSelectionTool> m_tauSelTool;
        ToolHandle<TauAnalysisTools::ITauEfficiencyCorrectionsTool> m_tauEffTool;
        ToolHandle<TauAnalysisTools::ITauTruthMatchingTool> m_tauTruthMatchTool;
        std::vector<CP::SystematicSet> m_tauEffSystSetList;

        std::string m_electronContainerKey; 
        std::string m_photonContainerKey; 
        std::string m_jetContainerKey; 
        std::string m_tauContainerKey; 
        std::string m_suffix;
        std::string m_suffixRecl;
        //ZeroLeptonRunPeriod m_period;
        DerivationTag m_derivationTag;
        int m_JESNuisanceParameterSet;

        std::vector<ST::SystInfo> m_SystInfoList;
        std::vector<std::string> m_SystMatch;

    public:
        ClassDef(ObjectBuilder,0);
};

#endif 

