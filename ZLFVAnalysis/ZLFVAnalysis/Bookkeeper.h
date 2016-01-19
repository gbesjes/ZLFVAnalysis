// vim: ts=4 sw=4
#ifndef HAVE_BOOKKEEPER_H
#define HAVE_BOOKKEEPER_H

#include <vector>
#include <map>
#include <string>

#include "TROOT.h"
#include "TDirectory.h"
#include "TH1D.h"
#include "TFile.h"
#include "TObjArray.h"

#include <xAODRootAccess/TEvent.h>

#include "cafe/Processor.h"
#include "ZLFVAnalysis/Utils.h"

class Bookkeeper : public cafe::Processor {
  public:
    Bookkeeper(const char *name);
    void begin();
    void inputFileOpened(TFile *file);
    void inputFileClosing(TFile *file);
    bool processEvent(xAOD::TEvent& event);
    void finish();

    std::string name() {
        return m_name;
    }

    TDirectory* getDirectory() const {
        return _directory  ? _directory : _directory = gROOT;
    }

  private:
    void openPoolFileCatalog();

    const char *m_name;
    TH1D* m_counter;
    TObjArray* m_fileInfos;
    unsigned int m_eventCounter;
    std::vector<unsigned int> m_eventsPerFile;
    std::vector<std::string> m_openedFiles;
    std::vector<std::string> m_closedFiles;
    std::map<std::string, std::string> m_fileCatalog;

    DerivationTag m_derivationTag;
    bool m_isData;

    mutable TDirectory *_directory;

  public:
    ClassDef(Bookkeeper,0);
};

#endif
