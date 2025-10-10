## FEATURE: Enhanced Boatdata 


This feature adds some extra elements to the central data model for essential boat data. 


### New Boat Data Elements
  
#### Compass Data 
- double RateOfTurn;            // Radians/second - Rate of change of the heading
- HeelAngle;              // Radians (+ = starboard, - = port), 
- PitchAngle                // Radians (+ = bow up, - = bow down) 
- Heave                     // 0.01 meters (1cm) - Vertical distance relative to the average sea level


// PGN 127250 - Vessel Heading (magnetic heading data)
// PGN 127251 - Rate of Turn
// PGN 127252 - Heave (vertical motion data)
// PGN 127257 - Attitude (pitch, roll, and yaw orientation)


#### Engine (NMEA 2000)
- double EngineRev
- double OilTemperature
- double AlternatorVoltage 

#### Saildrive (via 1-wire)
- bool SaildriveEngaged // true = engaged, false=disengaged

#### Battery Banks (via 1-wire)
- VoltageA, AmperageA, StateOfChargeA, ShoreChargerOnA, EngineChargerOnA
- VoltageB, AmperageB, StateOfChargeB, ShoreChargerOnB, EngineChargerOnB

#### Shore Power (via 1-wire)
- bool ShorePowerOn // true = on, false = off
- double Power // watt consumed




#### Paddle Wheel Data (Raw Sensor Data) =====


### Multiple Sources for same type of sensor data
There will be multiple sources / sensors for the GPS data and the Compass data, some from NMEA0183 and some from NMEA2000. 

The sources are prioritized, and only data from the highest priority source available will be used. If a higher priority source is not available, then the fall back is to use data from the next lower priority source. 

The default prioritization scheme is to rank sources by frequency of update, i.e. a source updating 10 times per second, has higher priority than one updating on once per second. 

There needs to be an interface for the user to override or fix the priority of the various sources, and this configuration must be persisted. 

### Outlier detection and removal
Sensor readings should be validated and invalid readings should be ignored. This includes readings that are clearly outliers. 








