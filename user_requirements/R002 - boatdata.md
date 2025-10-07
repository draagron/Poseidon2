## FEATURE: Boatdata 


This feature adds a centralized data model for essential boat data. 

This includes raw sensor data received via NMEA0183 interface, NMEA2000 interface, or 1-Wire interface. 
It also includes data elements calcuated, or derived, from the raw sensor data. 

The specification of NMEA and 1-wire interfaces will be done separately, and these interfaces are not part of this feature. The boatdata feature must provide interfaces / methods for reading and updating boatdata from interface message handlers. 


### Boat Data Elements
#### GPS Data
-  Latitude;               // Decimal degrees, positive = North
-  Longitude;              // Decimal degrees, positive = East
-  COG;                    // Course over ground, radians, 0 to 2*PI (true)
-  SOG;                    // Speed over ground, knots
  

#### Compass Data 
- double TrueHeading;            // Radians, 0 to 2*PI
- double MagneticHeading;        // Radians, 0 to 2*PI
- double Variation;              // Magnetic variation, radians (+ = East, - = West)


#### Wind Vane (Raw Sensor Data) 
- ApparantWindAngle;      // AWA, Radians, -PI to PI (+ = starboard, - = port)
- ApparentWindSpeed;      // AWS, Knots
  
#### Paddle Wheel Data (Raw Sensor Data) =====
- HeelAngle;              // Radians (+ = starboard, - = port)
- MeasuredBoatSpeed;      // MSB, Knots, from paddle wheel sensor

#### Rudder Data 
- RudderSteeringAngle;    // Radians (+ = starboard, - = port)

#### Calibration Parameters  (user configurable)
- LeewayCalibrationFactor; // K factor from trial calibration run
- WindAngleOffset;        // Radians, calibration offset for masthead misalignment


#### Calculated Sailing Parameters
- AWA_Offset;       // Radians, AWA corrected for masthead offset (-PI to PI)
- AWA_Heel;         // Radians, AWA corrected for heel (-PI to PI)
- Leeway;           // Radians, leeway angle (+ or -)
- STW;              // Speed through water, knots (corrected for leeway)
- TWS;              // True wind speed, knots
- TWA;              // True wind angle, radians (-PI to PI)
- VMG;              // Velocity made good, knots
- WDIR;             // Wind Direction, Radians, 0 to 2*PI, magnetic (true wind direction)
- SOC;              // Speed of Current, Knots, speed of current
- DOC;              // Direction of Current, Radians, 0 to 2*PI, magnetic (direction current is flowing TO)

#### Diagnostics 
- NMEA0183MessageCount;  // Total NMEA 0183 messages received
- NMEA2000MessageCount;  // Total NMEA 2000 messages received
- ActisenseMessageCount; // Total Actisense messages received
- LastUpdate;            // millis() timestamp of last calculation update

### Multiple Sources for same type of sensor data
There will be multiple sources / sensors for the GPS data and the Compass data, some from NMEA0183 and some from NMEA2000. 

The sources are prioritized, and only data from the highest priority source available will be used. If a higher priority source is not available, then the fall back is to use data from the next lower priority source. 

The default prioritization scheme is to rank sources by frequency of update, i.e. a GPS source updating 10 times per second, has higher priority than one updating on once per second. 

There needs to be an interface for the user to override or fix the priority of the various sources, and this configuration must be persisted. 

### Outlier detection and removal
Sensor readings should be validated and invalid readings should be ignored. This includes readings that are clearly outliers. 

### Calculation of derived sailing parameters:

The "Calculations" folder in the examples folder contains a draft C code for calculation of derived sailing performance indicators from the original source data coming from actual sensors on the boat. 
The derived parameters should be recalculated every 200 ms. 


### User update of calibration parameters
The must be a simple web interface for updating the calibration parameters 'LeewayCalibrationFactor' and 'WindAngleOffset'.


### OTHER CONSIDERATIONS:

- Update Include README with instructions for setup.
- Include the project structure in the README.





