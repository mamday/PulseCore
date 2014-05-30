#include "PulseCore/PulseCore.h"
#include <vector>
#include <map>
#include <cmath>
#include "dataclasses/I3MapOMKeyMask.h"
#include "icetray/OMKey.h"

//I3RecoPulseSeriesMap MergeSplits(I3RecoPulseSeriesMap& inMap){

//} 

I3_MODULE(PulseCore);

//TODO: Move these to .h
typedef std::pair<double, double> dpair;

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
  std::map<double, double> stringCharge;
  std::map<double, I3RecoPulseSeriesMap> stringPairs;
//Iterate over all DOMs in input pulse series 
  std::cout<<"Begin"<<std::endl;
  I3RecoPulseSeriesMapConstPtr pulses = frame->Get<I3RecoPulseSeriesMapConstPtr>(inputPulses_);
  for(I3RecoPulseSeriesMap::const_iterator pinfo = pulses->begin();
      pinfo!= pulses->end(); pinfo++){
    OMKey key = pinfo->first;
    std::vector<I3RecoPulse> all_doms = pinfo->second;
//Goofy struct - Probably want a class, TODO
    MergePulses domMerge;
    std::vector<bool> all_merged;
    all_merged.reserve(all_doms.size());
//TODO: Make a simple function to do this
    for(unsigned int i=0; i<all_doms.size(); i++){
      all_merged.push_back(false);
    }
    domMerge.merged=all_merged;
//Iterate over pulses on each DOM
    for(std::vector<I3RecoPulse>::const_iterator all_p = all_doms.begin();
        all_p!= all_doms.end(); all_p++){
      std::cout<<key<<" "<<all_p->GetTime()<<std::endl;
//Case where there is only one pulse on a DOM
      if(all_doms.size()==1){
        domMerge.merged[domMerge.time.size()] = true;
        domMerge.charge.push_back(all_p->GetCharge());
        domMerge.time.push_back(all_p->GetTime());
        continue;
      }
//If ATWD information is available
//TODO: Deal with edge case that there is a 0.5 charge pulse next to an FADC pulse
      if(all_p->GetFlags() & I3RecoPulse::ATWD){
        if(all_p->GetCharge()<0.5){
//Low Charge Hit is closer to earlier hit/
          if(fabs(all_doms[domMerge.time.size()-1].GetTime()-all_p->GetTime())<fabs(all_doms[domMerge.time.size()+1].GetTime()-all_p->GetTime()) || domMerge.time.size()==(all_doms.size()-1)){
            domMerge.merged[domMerge.time.size()] = true;
            domMerge.merged[domMerge.time.size()-1] = true;
            domMerge.charge.push_back(all_p->GetCharge());
            domMerge.time.push_back(domMerge.time[domMerge.time.size()-1]);
            continue;
          }
//Low Charge Hit is closer to later hit
          else if(fabs(all_doms[domMerge.time.size()-1].GetTime()-all_p->GetTime())>=fabs(all_doms[domMerge.time.size()+1].GetTime()-all_p->GetTime()) || domMerge.time.size()==0){
            domMerge.merged[domMerge.time.size()] = true;
            domMerge.merged[domMerge.time.size()+1] = true;
            domMerge.charge.push_back(all_p->GetCharge());
            domMerge.time.push_back(all_p->GetTime());
            continue;
          }
        }//End 0.5 charge 
//Get information for >0.5 pulses dependent on if they are merged with 0.5 charge pulses
        if(domMerge.merged[domMerge.time.size()]){
          domMerge.charge.push_back(all_p->GetCharge());
          domMerge.time.push_back(domMerge.time[domMerge.time.size()-1]);
        }
        else{
          domMerge.charge.push_back(all_p->GetCharge());
          domMerge.time.push_back(all_p->GetTime());
        }
      }//End ATWD
//If there is no ATWD information, keep the first time, keep all the charge
      else{
        double b_iter = domMerge.time.size();
        for(unsigned int i=b_iter; 
            i<all_doms.size(); i++){
          domMerge.time.push_back(all_p->GetTime());
          domMerge.charge.push_back(all_doms[i].GetCharge());
          domMerge.merged[i]=true;
        }
        break;
      }
    }//End loop over DOMs
//Loop over the pulses again and deal with the unmerged pulses
    int stupid_iter = 0;
    for(std::vector<I3RecoPulse>::const_iterator all_p = all_doms.begin();
        all_p!= all_doms.end(); all_p++){
      if(!domMerge.merged[stupid_iter]){
        domMerge.charge[stupid_iter]=all_p->GetCharge();
        domMerge.time[stupid_iter]=all_p->GetTime();
        domMerge.merged[stupid_iter]=true;
      }
      stupid_iter+=1;
    }
//    for(unsigned int i = 0;
//        i<domMerge.time.size(); i++){
//      if(domMerge.time.size()>1){
//      if(!(domMerge.merged[i])){
//        std::cout<<"Not Tested"<<std::endl;
//        std::cout<<domMerge.charge[i]<<" "<<domMerge.time[i]<<std::endl;
//      }
//      else if(domMerge.merged[i]){
//        std::cout<<"Tested"<<std::endl;
//        std::cout<<domMerge.charge[i]<<" "<<domMerge.time[i]<<std::endl;
//      }
//    } 
//  }
//Get rounded total charge and charge weighted time for the DOM
//TODO: Make this a function
    dpair tcPair;
    I3RecoPulse myPulse;
    for(unsigned int i=0;
        i<domMerge.time.size(); i++){
      tcPair.first+=domMerge.charge[i]*domMerge.time[i];
      tcPair.second+=domMerge.charge[i]; 
    }
//Create I3RecoPulse to hold my fake pulse
    myPulse.SetTime(tcPair.first/tcPair.second);
    myPulse.SetCharge(round(tcPair.second));
//Save total charge on string and I3RecoPulseSeriesMap with time, charge and omkey for this DOM
    stringCharge[key.GetString()]+=tcPair.second;
    stringPairs[key.GetString()][key].push_back(myPulse);
  }//End loop over pulses
  for(std::map<double,I3RecoPulseSeriesMap>::const_iterator sIter=stringPairs.begin();
      sIter!=stringPairs.end(); sIter++){ 
      std::cout<<sIter->first<<std::endl;
    for(I3RecoPulseSeriesMap::const_iterator rIter=sIter->second.begin();
        rIter!=sIter->second.end(); rIter++) std::cout<<rIter->first<<" "<<rIter->second[0]<<std::endl;
//    std::cout<<"String: "<<sIter->first<<" Charge: "<<sIter->second<<std::endl;
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
