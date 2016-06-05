// $Id$

#ifndef PULSECORE 
#define PULSECORE 

#include <icetray/I3ConditionalModule.h>
#include <icetray/I3Frame.h>
#include <dataclasses/geometry/I3Geometry.h>
#include <dataclasses/physics/I3DOMLaunch.h>
#include <dataclasses/physics/I3RecoPulse.h>
#include <dataclasses/physics/I3Waveform.h>
#include <icetray/I3Units.h>

#include "boost/make_shared.hpp"

typedef std::pair<double, double> dpair;
typedef std::vector<std::map<double, std::map<OMKey, I3RecoPulse> > > string_om_map_vec;
typedef std::map<double, std::map<OMKey, I3RecoPulse> > string_om_map;
typedef std::map<OMKey, I3RecoPulse> om_map;
typedef std::map<double, dpair> dpair_map;

struct MergePulses{
  std::vector<bool> merged;
  std::vector<double> charge;
  std::vector<double> time;
};

struct MapValueComp 
{
    template <typename Lhs, typename Rhs>
    bool operator()(const Lhs& lhs, const Rhs& rhs) const
    {
        return lhs.second < rhs.second;
    }
};

class PulseCore : public I3ConditionalModule{
  public:
    PulseCore(const I3Context& ctx);
    virtual ~PulseCore();
    void Configure();
    void Physics(I3FramePtr frame);
    void Finish();

    void MakeStringInfo(I3RecoPulseSeriesMapConstPtr pulses, std::map<double, double> &stringCharge);
    void SelectStrings(std::map<double, double> stringCharge, std::vector<OMKey> &selKeys);
    void I3RecoPulseSeriesMapFromKeys(std::vector<OMKey> selKeys, I3RecoPulseSeriesMapConstPtr pulses, I3RecoPulseSeriesMap &selMap);

  private:
    string_om_map_vec stringPairs_;
    std::map<double,double> sMean_;
    std::map<double,double> sVar_;
    std::string inputPulses_;
    std::string outputName_;
    unsigned int nStrings_;
    double pCharge_;
    double mTime_;
    double timeWindow_;
    double cCharge_;
    SET_LOGGER("PulseCore");
};

#endif //PULSECORE
