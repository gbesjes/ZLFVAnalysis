#ifndef HAVE_COUNTER_H
#define HAVE_COUNTER_H

// ---------------------------------------------------------------------------
// This is a simple class that implements a set of counters associated to a 
// name. 
// ---------------------------------------------------------------------------

#include <TH1D.h>
#include <TH2D.h>
#include <TDirectory.h>

#include <vector>
#include <map>
#include <iostream>
#include <fstream>
#include <string>

class Counter
{
    public:

        /// create a counter set with a name and maxcounters counters.
        Counter(const std::string & name, int maxcounters, bool isSignal=false);
        /// create a counter from an histogram
        Counter(TH2D* hist/*,int firstproc=-1,int lastproc=-1*/);
        /// Take a 1D histogram and expand the dimensionality
        Counter(TH1D* hist/*,int firstproc=-1,int lastproc=-1*/);

        ~Counter();

        /// increment a counter and a note
        void increment(double poids=1,int counter=1, std::string note="", int proc = -1);

        /// get the current status of all counters
        void get_counters(std::vector<double> & counters, int iproc=-1) const;

        /// print counters
        friend std::ostream&  operator<< (std::ostream& os, const Counter& counter);

        /// get access to the internal histogram
        TH1D* histo() {return _histo;}
        /// get the 2D counter histogram for SUSY signal
        TH2D* histo_signal() {return _histo_signal;}

    private:
        std::string _name;
        TH1D * _histo;
        TH2D * _histo_signal;
        std::vector<std::string> _histo_cuts;
        std::vector<std::string> _histo_signal_cuts;
};

class CounterRepository
{
    public:
        CounterRepository(const std::string& prefix, bool isSignal,TDirectory* dir): m_prefix(prefix), m_IsSignal(isSignal), m_directory(dir) {}
        ~CounterRepository();

        // find counter with label or create
        Counter* counter(const std::string& label);

        /// print counters
        friend std::ostream&  operator<< (std::ostream& os, const CounterRepository& counterRep);

    private:
        std::string m_prefix;
        bool m_IsSignal;
        TDirectory* m_directory;
        std::map<std::string,Counter*> m_CounterMap;
};

#endif
