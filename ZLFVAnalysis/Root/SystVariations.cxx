// vim: ts=4 sw=4

#include "ZLFVAnalysis/SystVariations.h"

#include "cafe/Config.h"
#include "cafe/ParseRun.h"

#include "xAODRootAccess/TEvent.h"
#include "xAODRootAccess/TActiveStore.h"
#include "xAODRootAccess/TStore.h"
//#include "PATInterfaces/SystematicList.h"
#include "PATInterfaces/SystematicSet.h"
#include "PATInterfaces/SystematicVariation.h"
#include "PATInterfaces/SystematicRegistry.h"
#include "PATInterfaces/SystematicCode.h"
#include "PATInterfaces/CorrectionCode.h"

#include <stdexcept>

SystVariations::SystVariations(const char *name): 
    cafe::Processor(name), 
    m_counter(0),
    m_processors() {
    cafe::Config config(name);

    // list of child processors
    std::string run = config.get("Run","");
    if(run != "") {
        cafe::ParseRun parser;
        add(parser.parse(run));
    }

}

void SystVariations::begin() {
    TDirectory* dir = getDirectory();
    if ( !dir ) {
        throw std::runtime_error("No root TDirectory defined in processor SR("+this->name()+")");
    }

    m_counter = new Counter("SystVariationsCounter", 40); 

    std::for_each(m_processors.begin(),m_processors.end(),
            std::mem_fun(&Processor::begin));
}

SystVariations::~SystVariations() {
    if ( m_counter ) {
        delete m_counter;
    }
    
    // TODO: C++14
    for ( std::list<Processor*>::iterator it = m_processors.begin();
            it != m_processors.end();
            ++it ) {
        delete *it;
    }
}

bool SystVariations::processEvent(xAOD::TEvent& event) {
    float weight=1;

    // access the transient store
    xAOD::TStore* store = xAOD::TActiveStore::store();
    CP::SystematicSet* currentSyst = new CP::SystematicSet();
    if ( ! store->record(currentSyst,"CurrentSystematicSet").isSuccess() ) {
        throw std::runtime_error("Could not record CurrentSystematicSet");
    }

    // counters
    int incr=0;
    m_counter->increment(1., incr++, "NbOfEvents");
    m_counter->increment(weight, incr++, "runNumber");

    std::vector<CP::SystematicSet>* sys_variations = 0;
    if ( ! store->retrieve(sys_variations,"sys_variations_kinematics").isSuccess() ) {
        throw std::runtime_error("Could not retrieve sys_variations_kinematics");
    }

    for ( const auto& iSyst : *sys_variations){
        *currentSyst = iSyst;
        //out() << " processing systematics " << currentSyst->name() << std::endl;
        for(std::list<cafe::Processor*>::iterator it = m_processors.begin();
                it != m_processors.end();
                ++it) {
            (*it)->incEventCount();
            if(!(*it)->processEvent(event)) return false;
        }
    }

    return true;
}

void SystVariations::finish() {
    std::for_each(m_processors.begin(),m_processors.end(),
            std::mem_fun(&Processor::finish));
    out() << *m_counter << std::endl;
}

void SystVariations::inputFileOpened(TFile *file)  {
    // TODO: C++14
    for(std::list<Processor*>::iterator it = m_processors.begin();
            it != m_processors.end();
            ++it) {
        (*it)->inputFileOpened(file);
    }
}

void SystVariations::inputFileClosing(TFile *file) {
    for(std::list<Processor*>::iterator it = m_processors.begin();
            it != m_processors.end();
            ++it) {
        (*it)->inputFileClosing(file);
    }
}

bool SystVariations::add(const std::list<cafe::Processor*>& procs) {
    for (std::list<Processor*>::const_iterator it = procs.begin();
            it != procs.end();
            ++it) {
        add(*it);
    }
    return true;
}

bool SystVariations::add(cafe::Processor *proc) {
    if(proc) {
        //proc->setParent(this);
        setParent(proc,this);
        if(debug() > 0) {
            proc->setDebug(debug());
        }
        out() << "SystVariations[" << name() << "]: Adding " << proc->fullName() << std::endl;
        m_processors.push_back(proc);
        return true;
    } else {
        return false;
    }
}

ClassImp(SystVariations);

