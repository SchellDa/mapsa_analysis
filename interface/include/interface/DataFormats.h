#ifndef DataFormats_h
#define DataFormats_h
#include "TROOT.h"
#include "TObject.h"
#include <vector>
#include <map>
#include <string>
#include <stdint.h>
using std::vector;
using std::map;
using std::string;


namespace tbeam {
  class cluster;
  class stub;
  class cluster : public TObject {
   public:
     cluster();
     virtual ~cluster();
     uint16_t x;
     uint16_t size;
     //std::vector<tbeam::stub *> stubs;
     ClassDef(cluster,1)
  };
  class stub : public TObject {
   public:
     stub();
     stub(const tbeam::stub& t);
     virtual ~stub();
     tbeam::cluster * seeding;  // Bottom sensor cluster
     tbeam::cluster * matched;  // Top sensor cluster
     uint16_t x;        // Position of the stub (bottom sensor)
     int16_t direction; // Direction of the stub (cl0-cl1)
     ClassDef(stub,1)
  };
  class cbc : public TObject {
   public:
     cbc();
     virtual ~cbc();
     uint16_t pipelineAdd;
     uint8_t status;
     uint8_t error;
     ClassDef(cbc,1)
  };
  
  class dutEvent : public TObject {
   public:
     dutEvent();
     dutEvent(const tbeam::dutEvent& t);
     virtual ~dutEvent();
     std::map < std::string, std::vector <tbeam::cluster*> > clusters;
     std::map< std::string,std::vector<int> > dut_channel;
     std::map< std::string,std::vector<int> > dut_row;
     std::vector <tbeam::stub*> stubs;
     uint32_t stubWord;
     uint32_t stubWordReco;
     //bool isGood;
     ClassDef(dutEvent,1)
  };
  class condEvent : public TObject {
   public:
    unsigned int run;
    unsigned int lumiSection;
    unsigned int event;
    unsigned long long time;
    unsigned long int unixtime;
    unsigned int tdcPhase;
    unsigned int HVsettings;
    unsigned int DUTangle;
    uint32_t window;
    uint32_t offset;
    uint32_t cwd;
    uint32_t tilt;
    uint32_t vcth;
    uint32_t stubLatency;
    uint32_t triggerLatency;
    int condData;
    int glibStatus;
    std::vector<tbeam::cbc> cbcs;
    condEvent();
    virtual ~condEvent();
    ClassDef(condEvent,1)
  };

  class TelescopeEvent : public TObject {
    public :
     Int_t           nTrackParams;
     Int_t           euEvt;
     std::vector<double>  *xPos;
     std::vector<double>  *yPos;
     std::vector<double>  *dxdz;
     std::vector<double>  *dydz;
     std::vector<int>     *trackNum;
     std::vector<int>     *iden;
     std::vector<double>  *chi2;
     std::vector<double>  *ndof;
     TelescopeEvent();
     TelescopeEvent(const TelescopeEvent& t);
     virtual ~TelescopeEvent();
     ClassDef(TelescopeEvent,1)
  };
}
#endif
