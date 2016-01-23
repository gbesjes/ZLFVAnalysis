// vim: ts=4 sw=4
#ifndef HAVE_GRL_PROCESSOR_H
#define HAVE_GRL_PROCESSOR_H

#include "cafe/Processor.h"
#include "GoodRunsLists/GoodRunsListSelectionTool.h"

//------------------------------------------------
// GRLProcessor:
//   Select event based on Good Run lists
//------------------------------------------------

class GRLProcessor : public cafe::Processor {
  public:
    GRLProcessor(const char *name);
    ~GRLProcessor();
    bool processEvent(xAOD::TEvent& event);
  private:
    GoodRunsListSelectionTool* m_GRLtool;
    bool m_passAll;

  public:
    ClassDef(GRLProcessor, 0);
};

#endif
