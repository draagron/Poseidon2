## FEATURE: Enhanced Boatdata 


This feature adds some extra elements to the central data model for essential boat data and it relocates other data elements. 


### Changes to existing data structures 

Changes to some of the existing data structures. Changes are marked with tripple stars *** <<description>> ***
ONLY THE CHANGED FIELDS or STRUCTURES ARE MENTIONED. NOT MENTIONED, MEANS NO CHANGE.

// =============================================================================
// SENSOR DATA STRUCTURES
// =============================================================================

/**
 * @brief GPS sensor data
 *
 * Raw GPS data from NMEA0183/NMEA2000 sources.
 * Units: decimal degrees, radians, knots
 */
struct GPSData {
   
    double variation;          ///< Magnetic variation, Radians, positive = East, negative = West  *** add, relocated from CompassData ***

};


struct CompassData {

    double variation;          ///< Magnetic variation, Radians, positive = East, negative = West  *** Move to GPSData ***
    
    double RateOfTurn;          // radians/second - Rate of change of the heading.  *** Added ***
    double HeelAngle;           // Radians (+ = starboard, - = port), *** RELOCATED FROM SpeedData,***
    double PitchAngle           // Radians (+ = bow up, - = bow down) *** Added ***
    double Heave                // Meters, Vertical distance relative to the average sea level *** Added ***

};


/**
 * @brief DST sensor data (depth, speed, temperature)  *** UPDATED Name of Structure***
 *
 * Measured depth, water speed, temperature *** UPDATED ***
 * Units: meters, m/s, celcius *** UPDATED ***
 */
struct DSTData { *** RENAME from SpeedData to DSTData - Depth, Speed, Temperature ***
    double depth;              // meters, Depth below waterline   *** Added ***
    double seaTemperature      // celcius, temperature of sea water *** Added ***
    double heelAngle;          ///< radians, range [-PI/2, PI/2], positive = starboard, negative = port.  *** Relocate to CompassData; ***
    double measuredBoatSpeed;  ///< MSB, m/s, from paddle wheel. *** Updated units ***

};


### New data structures: 

#### EngineData (NMEA 2000)
- double EngineRev
- double OilTemperature
- double AlternatorVoltage 

#### SaildriveData (via 1-wire)
- bool SaildriveEngaged // true = engaged, false=disengaged

#### BatteryData (via 1-wire)
- VoltageA, AmperageA, StateOfChargeA, ShoreChargerOnA, EngineChargerOnA
- VoltageB, AmperageB, StateOfChargeB, ShoreChargerOnB, EngineChargerOnB

#### ShorePowerData (via 1-wire)
- bool ShorePowerOn // true = on, false = off
- double Power // watt consumed




## ONLY UPDATE THE BOATDATA STRUCTURES
Do not implement functionality to acquire the sensor data from NMEA2000, NMEA0183 or 1-Wire. The data acquisition will be added separately later. 
Do not implement any backward compatibility. Just need the code to reflect the current requirements. 

