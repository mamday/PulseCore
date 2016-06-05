#!/usr/bin/env python

#Blatantly steal from myself
import icecube
from icecube import neutrinoflux, static_twc, SeededRTCleaning, TopologicalSplitter, NoiseEngine, dataclasses, clast, dataio, icetray

from icecube import PulseCore

from icecube import common_variables

import numpy, math

from I3Tray import *

geofile = dataio.I3File(sys.argv[1])
gFrame = geofile.pop_frame()
gFrame = geofile.pop_frame()

geometry_ = gFrame["I3Geometry"]

def PCVertCut(frame):
  if(frame.Has("String1Pulses")):
    cogPos = common_variables.hit_statistics.calculate_cog(geometry_,frame["String1Pulses"].apply(frame))
    cut_diff = cogPos - dataclasses.I3Position(29.3,52.6,cogPos.z)
    cut_diff_mag = cut_diff.magnitude
    if(cut_diff_mag<180 and cogPos.z>-500 and cogPos.z<-180):
      return True
    else:
      return False
  else:
    return False

tray = I3Tray()

tray.Add("I3Reader",FilenameList = [sys.argv[1],sys.argv[2]])

tray.AddModule('I3StaticTWC<I3RecoPulseSeries>', 'L2_static-twc',
               InputResponse     =  'OfflinePulses',        # ! Input response
               OutputResponse    =  'OfflinePulses_TW',           # ! Output response
               TriggerConfigIDs  =  [1011],                       # ! Trigger Config ID
               TriggerName       =  'I3TriggerHierarchy',         # Default
               FirstTriggerOnly  =  True,                         # Only take the time window around the first trigger
               WindowMinus       =  4000.0*I3Units.ns ,           # ! Backward time window for SMT3 Trigger
               WindowPlus        =  6000.0*I3Units.ns             # ! Forward time window  for SMT3 Trigger
               )

tray.AddModule('I3SeededRTHitMaskingModule', 'L2_sRTCleaning',
               InputResponse    = 'OfflinePulses_TW',
               OutputResponse   = 'OfflinePulses_sRT',
               RTRadius         = 200,
               RTTime           = 700,
               DeepCoreRTRadius         = 100,
               DeepCoreRTTime           = 400,
               MaxIterations    = 3,
               Seeds            = 'HLCcore',
               HLCCoreThreshold = 2
               )

tray.Add("PulseCore",
         InputPulses="OfflinePulses_sRT",
         OutputName="String3Pulses")

tray.Add("PulseCore",
         InputPulses="OfflinePulses_sRT",
         OutputName="String1Pulses",
         NStrings=1)

tray.Add(PCVertCut)

tray.Add("I3Writer",
  DropOrphanStreams = [icetray.I3Frame.DAQ,icetray.I3Frame.Geometry,icetray.I3Frame.Calibration,icetray.I3Frame.DetectorStatus],
  filename='/data/ana/LE/Background/NuMu/'+sys.argv[3])

tray.Execute()

