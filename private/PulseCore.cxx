#include "PulseCore/PulseCore.h"

#include "dataclasses/I3MapOMKeyMask.h"

//I3RecoPulseSeriesMap MergeSplits(I3RecoPulseSeriesMap& inMap){

//} 

~PulseCore(){}

PulseCore::PulseCore(const I3Context ctx) : I3ConditionalModule(context)
{
  AddOutBox("OutBox")

  inputPulses_ = ""

  AddParameter("InputPulses",
               "Name of the input I3RecoPulseSeriesMap",
                inputPulses_);
}

PulseCore::Configure(){
  GetParameter("InputPulses", inputPulses_);
}

PulseCore::Physics(){
  if(!frame->Has(inputPulses_)){
    log_warn("No input pulses");
    PushFrame(frame, "OutBox");
    return;
  }
  std::cout<<inputPulses_<<std::endl;
}

PulseCore::Finish(){
  log_trace("PulseCore::Finish()");
}
