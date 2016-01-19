#ifndef HAVE_PRWPROCESSOR_H
#define HAVE_PRWPROCESSOR_H

#include "PileupReweighting/PileupReweightingTool.h"
#include "PileupReweighting/TPileupReweighting.h"
#include "cafe/Processor.h"
#include <memory>

//FIXME : at some point one may use the PileupReweightingTool

class PileUpRWProcessor : public cafe::Processor {
  public:
    PileUpRWProcessor(const char *name);
    bool processEvent(xAOD::TEvent& event);
  private:
    bool m_noReweighting;
    std::auto_ptr<CP::PileupReweightingTool> m_PileupTool_CENTRAL;
    std::auto_ptr<CP::PileupReweightingTool> m_PileupTool_UP;
    std::auto_ptr<CP::PileupReweightingTool> m_PileupTool_DOWN;
    int m_forcedRunNumber;

  public:
    ClassDef(PileUpRWProcessor, 0);
};

#endif
