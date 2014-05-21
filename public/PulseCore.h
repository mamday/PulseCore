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

class PulseCore : public I3ConditionalModule{
  public:
    PulseCore(const I3Context& ctx);
    virtual ~PulseCore();
    void Configure();
    void Geometry(I3FramePtr frame);
    void Finish();
  private:
    SET_LOGGER("PulseCore");
};

#endif //PULSECORE
