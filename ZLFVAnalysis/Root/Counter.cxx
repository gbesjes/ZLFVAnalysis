// vim: ts=4 sw=4

#include "ZLFVAnalysis/Counter.h"

#include <ios>
#include <iostream>
#include <iomanip>
#include <cmath>
#include <stdexcept>
#include <assert.h>

#include <TH1D.h>
#include <TH2D.h>

#define NMAX 60
using namespace std;

Counter::Counter(const std::string & name, int maxcounters, bool isSignal) :
    _name(name), _histo(0), _histo_signal(0), _histo_cuts(maxcounters,""),
    _histo_signal_cuts(maxcounters,"") {
    if ( maxcounters > 0 ) {
        if ( !isSignal ) {
            _histo = new TH1D("Counter_for_"+TString(name), "Counter for "+TString(name), maxcounters+1 , 0, maxcounters+1);
        } else {
            _histo_signal = new TH2D("Counter_for_"+TString(name), "Counter for "+TString(name), maxcounters+1 , 0, maxcounters+1,  250,  0,  250);
        }
    } else { 
        throw std::logic_error("invalid number of counters");
    }
}

Counter::Counter(TH1D* hist) : _histo(hist), _histo_signal(0) {
    TString hname = hist->GetName();
    if ( hname.Index("Counter_for_") == 0 ) {
        hname.Remove(0, 12);
    }
    _name = hname.Data();
}

Counter::Counter(TH2D* hist) : _histo(0), _histo_signal(hist) {
    TString hname = hist->GetName();
    if ( hname.Index("Counter_for_") == 0 ) {
        hname.Remove(0, 12);
    }
    _name = hname.Data();
}

Counter::~Counter() {
}

void Counter::increment(double poids,int counter, std::string note, int iproc) {
    if ( note.size() > NMAX) {
        std::cout << "the string must have less than NMAX characters : " << note << std::endl;
    }

    if ( _histo ) {
        if ( counter < 0 || counter >= _histo->GetXaxis()->GetNbins()-1 ) {
            cout << "%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%" << endl;
            cout << "Invalid counter for "+_name << "  " << endl;
            cout << note << "  " << counter  << endl;
            cout << "%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%" << endl;
            assert(0);
            //throw std::out_of_range("In valid counter for "+_name);
        }
        
        if ( _histo->GetBinContent(counter+1) == 0. ) {
            _histo_cuts[counter] = note;
        }
        _histo->Fill(counter,poids);
    } else {
        if ( counter < 0 || counter >= _histo_signal->GetXaxis()->GetNbins()-1 ) {
            cout << "%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%" << endl;
            cout << "Invalid counter for "+_name << "  " << endl;
            cout << note << "  " << counter  << endl;
            cout << "%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%" << endl;
            assert(0);
            //throw std::out_of_range("In valid counter for "+_name);
        } 
        
        if ( _histo_signal->Integral(counter+1,counter+1,1,250) == 0.){
            _histo_signal_cuts[counter] = note;
        }
        _histo_signal->Fill(counter,iproc,poids);
    }
}

void Counter::get_counters(std::vector<double> & counters, int iproc) const {
    if ( iproc < 0 ) {
        for ( int i = 0; i < _histo->GetXaxis()->GetNbins()-1; ++i) {
            counters.push_back(_histo->GetBinContent(i+1));
        }
    } else { 
        for ( int i = 0; i < _histo_signal->GetXaxis()->GetNbins()-1; ++i) {
            counters.push_back(_histo_signal->GetBinContent(i+1, iproc));
        }
    } 
}

std::ostream&  operator<< (std::ostream& os, const Counter& counter) {
    if ( !counter._histo ) return os;
    os.setf(ios::floatfield,ios::fixed);
    os<<setprecision(1);

    // get the width for the stream, so that we can restore its value
    std::streamsize width = os.width();

    std::streamsize wcount = static_cast<std::streamsize>(std::log10((double)(counter._histo->GetXaxis()->GetNbins()-2)))+1;
    os << endl;
    os << "%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%% "<< endl;
    os << " Counters for " << counter._name << std::endl;
    os << "==============================================================================" << endl;
    for ( int i = 0;  i < counter._histo->GetXaxis()->GetNbins()-1; ++i)  {
        if( i!=counter._histo->GetXaxis()->GetNbins() && counter._histo->GetBinContent(i+1)==0) {
            continue;
        }
        double effrel = 1.;
        if(i>=1 && (double)counter._histo->GetBinContent(i)!=0 ) { 
            effrel = (double)counter._histo->GetBinContent(i+1) / 
                (double)counter._histo->GetBinContent(i);
        }

        std::string descript = counter._histo_cuts[i]; 
        if( descript !="") { 
            os << std::setw(wcount) << descript << 
            std::setw(NMAX-descript.size() )<< ": " << std::setw(8) <<
                setprecision(3)<< counter._histo->GetBinContent(i+1) << " " << std::setw(width)<<setprecision(3)<<" "<<(double)counter._histo->GetBinContent(i+1)/counter._histo->GetBinContent(1)<<setprecision(3)<<" "<< std::setw(width)<<setprecision(3)<<effrel;
        } else {
            os << std::setw(wcount) <<  i << std::setw(NMAX-2)<< ": " << std::setw(8) << counter._histo->GetBinContent(i+1) << " " << std::setw(width);
       }
        if ( (i+1)%1 == 0 ) os << std::endl;
    }
    os << "%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%" << endl; 
    os << std::endl;

    return os;
}

Counter* CounterRepository::counter(const std::string& label) {
    std::map<std::string,Counter*>::const_iterator pos = m_CounterMap.find(label);
    if ( pos == m_CounterMap.end() ) {
        std::string tag;
        if ( label != "" ) { 
            tag = "_"+label;
        }

        pos = m_CounterMap.insert(std::make_pair(label, new Counter(m_prefix+tag,40,m_IsSignal))).first;
        if ( m_IsSignal ) {
            pos->second->histo_signal()->SetDirectory(m_directory);
        } else {
            pos->second->histo()->SetDirectory(m_directory);
        }
    }
    return pos->second;
}

CounterRepository::~CounterRepository() {
    for ( std::map<std::string,Counter*>::iterator pos = m_CounterMap.begin();
            pos != m_CounterMap.end(); pos++ ){
        delete pos->second;
    }
}

std::ostream&  operator<< (std::ostream& os, const CounterRepository& counterRep) {
    for ( std::map<std::string,Counter*>::const_iterator pos = counterRep.m_CounterMap.begin();
            pos != counterRep.m_CounterMap.end(); pos++ ){
        os << *(pos->second);
    }
    return os;
}

