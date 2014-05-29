#include "PulseCore/PulseCore.h"
#include <vector>
#include "dataclasses/I3MapOMKeyMask.h"

//I3RecoPulseSeriesMap MergeSplits(I3RecoPulseSeriesMap& inMap){

//} 

I3_MODULE(PulseCore);

struct MergePulses{
  std::vector<bool> merged;
  std::vector<double> charge;
  std::vector<double> time;
};

PulseCore::~PulseCore(){}

PulseCore::PulseCore(const I3Context& ctx) : I3ConditionalModule(ctx)
{
  AddOutBox("OutBox");

  inputPulses_ = "";
  outputName_ = "PulseCorePulses";
  AddParameter("InputPulses",
               "Name of the input I3RecoPulseSeriesMap",
                inputPulses_);
  AddParameter("OutputName",
               "Name of the output",
                outputName_);
  return;
}

void PulseCore::Configure(){
  GetParameter("InputPulses", inputPulses_);
  GetParameter("OutputName", outputName_);
  return;
}

void PulseCore::Physics(I3FramePtr frame){
  if(inputPulses_==""){
    log_warn("No input Pulse name specified");
    PushFrame(frame, "OutBox");
    return;
  }
  if(!frame->Has(inputPulses_)){
    log_warn("Failed to find pulse series %s in frame", inputPulses_.c_str());
    PushFrame(frame, "OutBox");
    return;
  }
  I3RecoPulseSeriesMapConstPtr pulses = frame->Get<I3RecoPulseSeriesMapConstPtr>(inputPulses_);
  for(I3RecoPulseSeriesMap::const_iterator pinfo = pulses->begin();
      pinfo!= pulses->end(); pinfo++){
    std::vector<I3RecoPulse> all_doms = pinfo->second;
    MergePulses domMerge;
    for(std::vector<I3RecoPulse>::const_iterator all_p = all_doms.begin();
        all_p!= all_doms.end(); all_p++){
      if(all_p->GetCharge()>0.5){
        domMerge.merged.push_back(true);
        domMerge.charge.push_back(all_p->GetCharge());
        domMerge.time.push_back(all_p->GetTime());
      }
//      if(all_p->GetFlags() & I3RecoPulse::ATWD){ std::cout<<"Woot! "<<" "<<all_p->GetCharge()<<std::endl;}
//      else if(all_p->GetFlags() & I3RecoPulse::FADC){ std::cout<<"fADC but no ATWD"<<std::endl;}
//      else{std::cout<<"Boo"<<std::endl;}
    }
    for(std::vector<bool>::const_iterator bTest = domMerge.merged.begin();
        bTest!=domMerge.merged.end(); bTest++){
      if(*bTest){
        std::cout<<"Tested"<<std::endl;
      }
    } 
  }
//TODO: Modify this to contain the pules after Pulse Coring
  I3RecoPulseSeriesMapConstPtr outputPulses = frame->Get<I3RecoPulseSeriesMapConstPtr>(inputPulses_);
//Output final pulses to frame
  frame->Put(outputName_,outputPulses);
  PushFrame(frame, "OutBox");

  return;
}

void PulseCore::Finish(){
  log_trace("PulseCore::Finish()");
}
