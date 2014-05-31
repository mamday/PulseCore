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
typedef std::vector<std::map<double, std::map<OMKey, I3RecoPulse> > > string_om_map_vec;
typedef std::map<double, std::map<OMKey, I3RecoPulse> > string_om_map;
typedef std::map<OMKey, I3RecoPulse> om_map;

static bool PulseCompare(const string_om_map &m1, const string_om_map &m2)
{
//Whee! This is comparing I3RecoPulses from the string_om_map in the end
  return m1.begin()->second.begin()->second.GetTime() < m2.begin()->second.begin()->second.GetTime();
}

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
  nStrings_ = 3;
  pCharge_ = 0.68;
  AddParameter("InputPulses",
               "Name of the input I3RecoPulseSeriesMap",
                inputPulses_);
  AddParameter("OutputName",
               "Name of the output",
                outputName_);
  AddParameter("NStrings",
               "Number of strings to keep in the final pulse series",
                nStrings_);
  AddParameter("PercentCharge",
               "Percent of total charge to keep on each string",
                pCharge_);
  return;
}

void PulseCore::Configure(){
  GetParameter("InputPulses", inputPulses_);
  GetParameter("OutputName", outputName_);
  GetParameter("NStrings", nStrings_);
  GetParameter("PercentCharge", pCharge_);
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
//Whee! Something that is iterable for sorting...
  string_om_map_vec stringPairs;
//Iterate over all DOMs in input pulse series 
//TODO: Make this a function
  I3RecoPulseSeriesMapConstPtr pulses = frame->Get<I3RecoPulseSeriesMapConstPtr>(inputPulses_);
  for(I3RecoPulseSeriesMap::const_iterator pinfo = pulses->begin();
      pinfo!= pulses->end(); pinfo++){
    OMKey key = pinfo->first;
    I3RecoPulseSeries all_doms = pinfo->second;
//Goofy struct TODO
    MergePulses domMerge;
    std::vector<bool> all_merged;
    all_merged.reserve(all_doms.size());
//TODO: Make a simple function to intitialze the vector
    for(unsigned int i=0; i<all_doms.size(); i++){
      all_merged.push_back(false);
    }
    domMerge.merged=all_merged;
//Iterate over pulses on each DOM
//TODO: Something better than using domMerge.time.size() to keep track of where I am in the pulse series
    for(I3RecoPulseSeries::const_iterator all_p = all_doms.begin();
        all_p!= all_doms.end(); all_p++){
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
//TODO: Something better than stupid_iter
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

//Get rounded total charge and charge weighted time for the DOM
//TODO: Make this a function
    dpair tcPair;
    I3RecoPulse myPulse;
    for(unsigned int i=0;
        i<domMerge.time.size(); i++){
      tcPair.first+=domMerge.charge[i]*domMerge.time[i];
      tcPair.second+=domMerge.charge[i]; 
    }
//Create I3RecoPulse to hold my fake pulse. Make a map to the OMKey, then make another map to the string 
    myPulse.SetTime(tcPair.first/tcPair.second);
    myPulse.SetCharge(round(tcPair.second));
    om_map myPMap;
    myPMap[key]=myPulse;
    string_om_map mySMap;
    mySMap[key.GetString()]=myPMap;
//Save total charge on string and push back map of maps with time, charge and omkey for this DOM
    stringCharge[key.GetString()]+=tcPair.second;
    stringPairs.push_back(mySMap);
  }//End loop over pulses
//Sort by pulse time
  std::sort(stringPairs.begin(),stringPairs.end(),PulseCompare);

//TODO: Make this a function
//  I3RecoPulseSeriesMapMask myMask;
  std::cout<<"Begin"<<std::endl;
  std::vector<double> selStrings;
  for(unsigned int i=0; i<nStrings_; i++){
    double firstCharge = 0;
    for(string_om_map_vec::const_iterator fIter=stringPairs.begin();
      fIter!=stringPairs.end(); fIter++){
      //std::cout<<"Pairs "<<fIter->size()<<std::endl;
//Only look at strings with charge > 2 p.e
      std::cout<<"Before String: "<<fIter->begin()->first<<" Time: "<<fIter->begin()->second.begin()->second.GetTime()<<" Charge: "<<fIter->begin()->second.begin()->second.GetCharge()<<std::endl;
      if(fIter->begin()->second.begin()->second.GetCharge()<3) continue;
//Get the first nString_ string numbers
      if(selStrings.empty()){
        selStrings.push_back(fIter->begin()->first);
      }
      else{
        if(selStrings.size()==i){
          bool prevString = false;
          for(unsigned int j=0; j<i; j++){
            if(fIter->begin()->first==selStrings[j]) prevString=true;
          }
          if(!prevString) selStrings.push_back(fIter->begin()->first);
          else continue;
        }
      }

//        if(firstString==0){
//          firstString = sIter->first;
//          selStrings.push_back(firstString);
//        }
//        else{ 
//          if(*find(selStrings.begin(),selStrings.end(),sIter->first)>0){
//            std::cout<<"Found: "<<i<<" "<<sIter->first<<" "<<*find(selStrings.begin(),selStrings.end(),sIter->first)<<std::endl;
//            continue;
//          }
//          else{
//            firstString = sIter->first;
//            selStrings.push_back(firstString);
//          }
//    }
//        std::cout<<"First: "<<i<<" "<<selStrings.size()<<" "<<firstString<<std::endl;


      if(selStrings.size()==(i+1) && fIter->begin()->first==selStrings[i]){
        std::cout<<i<<" "<<fIter->begin()->first<<" "<<fIter->begin()->second.begin()->second.GetTime()<<" "<<fIter->begin()->second.begin()->second.GetCharge()<<std::endl;
//          while(firstCharge<pCharge_*stringCharge[sIter->first]){
//            firstCharge+=sIter->second.begin()->second.GetCharge();
//            std::cout<<sIter->first<<" "<<firstCharge<<" "<<pCharge_<<" "<<sIter->second.begin()->second.GetCharge()<<std::endl;
//          }
      }
      else continue;
//Keep information only for the first nStrings_ strings
      std::cout<<" String: "<<fIter->begin()->first<<" Time: "<<fIter->begin()->second.begin()->second.GetTime()<<" Charge: "<<fIter->begin()->second.begin()->second.GetCharge()<<std::endl;
    }
  }  


//  for(std::vector<std::map<double,std::map<OMKey, I3RecoPulse> > >::iterator sIter=stringPairs.begin();
//      sIter!=stringPairs.end(); sIter++){ 
//    for(std::map<OMKey,I3RecoPulse>::iterator rIter=sIter->second.begin();
//     rIter!=sIter->second.end(); rIter++){
//      std::cout<<" String: "<<sIter->begin()->second.begin()->first<<" Charge: "<<sIter->begin()->second.begin()->second.GetCharge()<<" Time: "<<sIter->begin()->second.begin()->second.GetTime()<<std::endl;
//  }
//  }


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
