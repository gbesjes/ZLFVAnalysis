// vim: ts=4 sw=4
#ifndef HAVE_DETECTORQUALITY_H
#define HAVE_DETECTORQUALITY_H

#include "cafe/Processor.h"
#include "ZLFVAnalysis/Utils.h"

//------------------------------------------------
// DetectorQuality:
//   select detector and processing quality flags
//   and write decision to keep event or not in
//   transient store
// -----------------------------------------------

class DetectorQuality : public cafe::Processor {
    public:
        DetectorQuality(const char *name);
        bool processEvent(xAOD::TEvent& event);

    private:
        RunPeriod m_period;

    public:
        ClassDef(DetectorQuality,0);
};

#endif 
