#include "PulseCore/PulseCore.h"
#include <vector>
#include <map>
#include <cmath>
#include "dataclasses/I3MapOMKeyMask.h"
#include <boost/math/special_functions/fpclassify.hpp>
#include "icetray/OMKey.h"

I3_MODULE(PulseCore);

static bool PulseCompare(const string_om_map &m1, const string_om_map &m2)
{
//Whee! This is comparing I3RecoPulses from the string_om_map in the end
  if(isnan(m1.begin()->second.begin()->second.GetTime())) return false;     
  else if(isnan(m2.begin()->second.begin()->second.GetTime())) return true;     
  else return m1.begin()->second.begin()->second.GetTime() < m2.begin()->second.begin()->second.GetTime();
}

static std::map<double, double> StringCWTimeMean(string_om_map_vec stringPairs_, std::map<double,double> stringCharge){
  std::map<double,double> sTime;
  for(string_om_map_vec::const_iterator fIter=stringPairs_.begin();
      fIter!=stringPairs_.end(); fIter++){
    double dCharge = fIter->begin()->second.begin()->second.GetCharge();
    double sCharge = stringCharge[fIter->begin()->first];
    double tTime = fIter->begin()->second.begin()->second.GetTime();
    sTime[fIter->begin()->first] += dCharge*fIter->begin()->second.begin()->second.GetTime()/sCharge; 
  }
  return sTime;
}

static std::map<double, double> StringCWTimeVar(string_om_map_vec stringPairs_, std::map<double,double> mean, std::map<double,double> stringCharge){
  std::map<double,double> sDiff;
  for(string_om_map_vec::const_iterator fIter=stringPairs_.begin();
      fIter!=stringPairs_.end(); fIter++){
    double dCharge = fIter->begin()->second.begin()->second.GetCharge();
    double sCharge = stringCharge[fIter->begin()->first];
    double tTime = fIter->begin()->second.begin()->second.GetTime();
    double sMean = mean[fIter->begin()->first];
    sDiff[fIter->begin()->first] += dCharge*((tTime-sMean)*(tTime-sMean))/sCharge;
  }
  return sDiff;
}

void PulseCore::MakeStringInfo(I3RecoPulseSeriesMapConstPtr pulses, std::map<double, double> &stringCharge){
//Iterate over all DOMs in input pulse series 
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
          if((fabs(all_doms[domMerge.time.size()-1].GetTime()-all_p->GetTime())<fabs(all_doms[domMerge.time.size()+1].GetTime()-all_p->GetTime()) || domMerge.time.size()==(all_doms.size()-1)) && domMerge.time.size()!=0){
            domMerge.merged[domMerge.time.size()] = true;
            domMerge.merged[domMerge.time.size()-1] = true;
            domMerge.charge.push_back(all_p->GetCharge());
            domMerge.time.push_back(domMerge.time[domMerge.time.size()-1]);
            continue;
          }
//Low Charge Hit is closer to later hit
          else if((fabs(all_doms[domMerge.time.size()-1].GetTime()-all_p->GetTime())>=fabs(all_doms[domMerge.time.size()+1].GetTime()-all_p->GetTime()) || domMerge.time.size()==0) && domMerge.time.size()!=(all_doms.size()-1)){
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
    dpair tcPair;
    I3RecoPulse myPulse;
    for(unsigned int i=0;
        i<domMerge.time.size(); i++){
      tcPair.first+=domMerge.charge[i]*domMerge.time[i];
      tcPair.second+=domMerge.charge[i]; 
    }
//Create I3RecoPulse to hold my fake pulse. Make a map to the OMKey, then make another map to the string 
    myPulse.SetTime(tcPair.first/tcPair.second);
//    myPulse.SetCharge(round(tcPair.second));
    myPulse.SetCharge((tcPair.second));
    om_map myPMap;
    myPMap[key]=myPulse;
    string_om_map mySMap;
    mySMap[key.GetString()]=myPMap;
//Save total charge on string and push back map of maps with time, charge and omkey for this DOM
    stringCharge[key.GetString()]+=tcPair.second;
    stringPairs_.push_back(mySMap);
  }//End loop over pulses
}

void PulseCore::SelectStrings(std::map<double, double> stringCharge, std::vector<OMKey> &selKeys){
  std::vector<double> selStrings;
  std::map<double,double>::const_iterator max_iter = max_element(stringCharge.begin(),stringCharge.end(),MapValueComp()); 
//Get pulses for first nStrings_ strings
  unsigned int i=0;
  unsigned int sk_size = selKeys.size();
  string_om_map_vec holdPairs = stringPairs_;
  while(selStrings.size()<nStrings_){
    double firstCharge = 0;
    double cMean = 0;
    double cStdDev = 0;
    for(string_om_map_vec::const_iterator fIter=stringPairs_.begin();
      fIter!=stringPairs_.end(); fIter++){
//Only look at strings with charge > cCharge p.e
      if(round(fIter->begin()->second.begin()->second.GetCharge())<cCharge_) continue;

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

//Add DOMS with time outside mTime_ multiples of the charge weighted standard deviation of the pulse times to skipKeys, so they are not added
      OMKey skipKey; 
      if(mTime_!=0  && selStrings.size()==(i+1) && fIter->begin()->first==selStrings[i]){
        double dTime = fIter->begin()->second.begin()->second.GetTime();
        double pDev = sMean_[fIter->begin()->first]-(mTime_*sqrt(fabs(sVar_[fIter->begin()->first])));
        double nDev = sMean_[fIter->begin()->first]+(mTime_*sqrt(fabs(sVar_[fIter->begin()->first])));
        if(dTime<pDev || dTime>nDev){
          skipKey = fIter->begin()->second.begin()->first;
//TODO: This doesn't quite do the right thing. Want something that would let me do a percentage of the charge and a time range, so I need to know what the charge is after I reject the time outliers
//          stringCharge[fIter->begin()->first]-=fIter->begin()->second.begin()->second.GetCharge();
        }
//Skip if the time is outside a standard deviation from the string with the most charge
        double dev_fact=timeWindow_;
//        timeWindow_>sVar_[max_iter->first] ? dev_fact = timeWindow_ : dev_fact = sVar_[max_iter->first];
        double pMDev = sMean_[max_iter->first]-dev_fact;
        double nMDev = sMean_[max_iter->first]+dev_fact;
        if(dTime<pMDev || dTime>nMDev){
          skipKey = fIter->begin()->second.begin()->first;
        }
      }
//Keep DOMs on string with the first pCharge_ percent of charge if they are not in skipKeys
      if(selStrings.size()==(i+1) && fIter->begin()->first==selStrings[i]){
          if(firstCharge<pCharge_*stringCharge[fIter->begin()->first]){
            firstCharge+=fIter->begin()->second.begin()->second.GetCharge();
            if(skipKey==fIter->begin()->second.begin()->first) continue; 
            selKeys.push_back(fIter->begin()->second.begin()->first);
          }
      }
      else continue;
    }
//Don't keep strings if none of the DOMs were selected
    if(selKeys.size()==sk_size && stringPairs_.size()>1){
      selStrings.pop_back();
      stringPairs_.erase(stringPairs_.begin());
    }
    else if(stringPairs_.size()<=1) break;
    else{
      i+=1;
      sk_size = selKeys.size();
      stringPairs_.clear();
      stringPairs_=holdPairs;
    }
  }  
  holdPairs.clear();
  selStrings.clear();
}

void PulseCore::I3RecoPulseSeriesMapFromKeys(std::vector<OMKey> selKeys, I3RecoPulseSeriesMapConstPtr pulses, I3RecoPulseSeriesMap &selMap){
//Get selMap that contains only I3RecoPulses from pulses with OMKeys in vector
  for(I3RecoPulseSeriesMap::const_iterator pinfo = pulses->begin();
      pinfo!= pulses->end(); pinfo++){
    OMKey key = pinfo->first;
    I3RecoPulseSeries all_doms = pinfo->second;
    I3RecoPulseSeries selPulse;
    for(std::vector<I3RecoPulse>::const_iterator all_p = all_doms.begin();
        all_p!= all_doms.end(); all_p++){
      for(std::vector<OMKey>::const_iterator oIter=selKeys.begin();
          oIter!=selKeys.end(); oIter++){
        if(key==(*oIter)){
          selPulse.push_back((*all_p));
        }
      }
    }
    if(!(selPulse.empty())) selMap[key]=selPulse;
  }

}

PulseCore::~PulseCore(){}

PulseCore::PulseCore(const I3Context& ctx) : I3ConditionalModule(ctx)
{
  AddOutBox("OutBox");

  inputPulses_ = "";
  outputName_ = "PulseCorePulses";
  nStrings_ = 3;
  pCharge_ = 0.66;
  mTime_ = 0.;
  timeWindow_ = 10000.;
  cCharge_ = 2;
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
  AddParameter("MultipleTime",
               "Multiple of charge weighted standard deviation of string pulse times from charge weighted average to keep on each string",
                mTime_);
  AddParameter("TimeWindow",
               "Time window around string with maximum charge",
                timeWindow_);
  AddParameter("CutCharge",
               "Minimum p.e. on each final selected string",
                cCharge_);
  return;
}

void PulseCore::Configure(){
  GetParameter("InputPulses", inputPulses_);
  GetParameter("OutputName", outputName_);
  GetParameter("NStrings", nStrings_);
  GetParameter("CutCharge", cCharge_);
  GetParameter("PercentCharge", pCharge_);
  GetParameter("MultipleTime", mTime_);
  GetParameter("TimeWindow", timeWindow_);
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
//Get input pulses
  I3RecoPulseSeriesMapConstPtr pulses = frame->Get<I3RecoPulseSeriesMapConstPtr>(inputPulses_);

//  log_info("Begin %d" , (*pulses).size());
  std::map<double, double> stringCharge;
//Get charge on each string with reasonable times based on understanding of pulses on each DOM
  MakeStringInfo(pulses, stringCharge);
  if(mTime_!=0){
    sMean_ = StringCWTimeMean(stringPairs_,stringCharge);
    sVar_ = StringCWTimeVar(stringPairs_, sMean_, stringCharge);
  }
//Sort by pulse time
  std::sort(stringPairs_.begin(),stringPairs_.end(),PulseCompare);

//Select strings based on criteria
  std::vector<OMKey> selKeys;
  SelectStrings(stringCharge,selKeys);

//Loop through the pulses again and add the pulses to the mask if the OMKey passed the previous requirements
//TODO: Make this a function
  I3RecoPulseSeriesMap selMap;
  I3RecoPulseSeriesMapFromKeys(selKeys, pulses, selMap);
  
//Sanitize the output
  I3RecoPulseSeriesMapMaskPtr myMask = boost::make_shared<I3RecoPulseSeriesMapMask>(*frame,inputPulses_,selMap);

  selKeys.clear();
  stringCharge.clear();
  stringPairs_.clear();
  selMap.clear();
//  std::cout<<(*pulses).size()<<" "<<(*testMap).size()<<std::endl;

//Case where there are no pulses after coring
  I3RecoPulseSeriesMapConstPtr testMap = myMask->Apply(*frame);
  if((*testMap).size()==0){
    log_warn("No pulses in core");
    PushFrame(frame, "OutBox");
    return;
  }

//Output final pulses to frame
  frame->Put(outputName_,myMask);
  PushFrame(frame, "OutBox");

  return;
}

void PulseCore::Finish(){
  log_trace("PulseCore::Finish()");
}
