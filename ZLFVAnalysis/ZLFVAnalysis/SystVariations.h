// vim: ts=4 sw=4
#ifndef HAVE_SYSTVARIATIONS_H
#define HAVE_SYSTVARIATIONS_H

#include "cafe/Processor.h"
#include "ZLFVAnalysis/Counter.h"

#include <string>
#include <list>
#include <vector>
#include <memory>

#include "TFile.h"

//--------------------------------------------------------------------------
// SystVariations
//
// Works a bit like a cafe::Controller except that the child
// Processors are called for each systematic
//--------------------------------------------------------------------------
class SystVariations : public cafe::Processor {
    public:
        SystVariations(const char *name);
        ~SystVariations();
        void begin();
        bool processEvent(xAOD::TEvent& event);
        void finish();
        virtual void inputFileOpened(TFile *file);
        virtual void inputFileClosing(TFile *file);

    private:
        Counter* m_counter;

        std::list<cafe::Processor*> m_processors;
        bool add(const std::list<cafe::Processor*>& procs);
        bool add(cafe::Processor *proc);

    public:
        ClassDef(SystVariations, 0);
};

#endif 
