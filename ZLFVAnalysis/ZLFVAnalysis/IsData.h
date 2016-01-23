// vim: ts=4 sw=4
#ifndef HAVE_ISDATA_H
#define HAVE_ISDATA_H

#include "cafe/Processor.h"

// -------------------------------------------------------------
// IsData: processEvent() return true if event from data
// -------------------------------------------------------------
class IsData : public cafe::Processor {
  public:
    IsData(const char *name);
    bool processEvent(xAOD::TEvent& event);
  private:
    bool m_enforce;
    bool m_expected;

  public:
    ClassDef(IsData,0);
};

#endif
