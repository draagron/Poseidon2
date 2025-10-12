## NMEA 2000 message handling

Implement handlers for NMEA 2000 messages as source for select BoatData elements. 

For all messages, use the https://github.com/ttlappalainen/NMEA2000 library for parsing the messages, in particular make sure to study the message header file https://github.com/ttlappalainen/NMEA2000/blob/master/src/N2kMessages.h 

NMEA 2000 message handlers are implemented in the source files src/components/NMEA2000Handlers.cpp and src/components/NMEA2000Handlers.h, but you need to double check that the all the necessary PNGs are included and that any required unit conversions are included and correct. 


### NMEA2000 PGN to Sensor Data Structure Mapping

Use the following mapping specification of NMEA2000 messages to the BoatData elements. Double check that existing implementation in src/components/NMEA2000Handlers.cpp and src/components/NMEA2000Handlers.h are complete and correct, and any anything missing. 

#### GPSData Structure
Primary PGNs:
	•	PGN 129025: Position, Rapid Update
	•	`latitude` ← `Latitude` (double, degrees)
	•	`longitude` ← `Longitude` (double, degrees)
	•	PGN 129026: COG & SOG, Rapid Update
	•	`cog` ← `COG` (double, radians)
	•	`sog` ← `SOG` (double, m/s) - Note: Convert to knots
	•	PGN 129258: Magnetic Variation
	•	`variation` ← `Variation` (double, radians)
Alternative/Enhanced:
	•	PGN 129029: GNSS Position Data - More comprehensive GPS data with quality metrics

#### CompassData Structure
Primary PGNs:
	•	PGN 127250: Vessel Heading
	•	`trueHeading` ← `Heading` (when `ref` = N2khr_true)
	•	`magneticHeading` ← `Heading` (when `ref` = N2khr_magnetic)
	•	PGN 127251: Rate of Turn
	•	`rateOfTurn` ← `RateOfTurn` (double, radians/second)
	•	PGN 127257: Attitude
	•	`heelAngle` ← `Roll` (double, radians)
	•	`pitchAngle` ← `Pitch` (double, radians)
	•	PGN 127252: Heave
	•	`heave` ← `Heave` (double, meters)

#### WindData Structure
Primary PGN:
	•	PGN 130306: Wind Data
	•	`apparentWindAngle` ← `WindAngle` (when `WindReference` = N2kWind_Apparent)
	•	`apparentWindSpeed` ← `WindSpeed` (double, m/s) - Note: Convert to knots

#### DSTData Structure
Primary PGNs:
	•	PGN 128267: Water Depth
	•	`depth` ← `DepthBelowTransducer` + `Offset` (when offset represents waterline)
	•	PGN 128259: Boat Speed, Water Referenced
	•	`measuredBoatSpeed` ← `WaterReferenced` (double, m/s)
	•	PGN 130316: Temperature, Extended Range (recommended over deprecated versions)
	•	`seaTemperature` ← `ActualTemperature` (when `TempSource` = N2kts_SeaTemperature) - Note: Convert from Kelvin to Celsius

#### EngineData Structure
Primary PGNs:
	•	PGN 127488: Engine Parameters Rapid
	•	`engineRev` ← `EngineSpeed` (double, RPM)
	•	PGN 127489: Engine Parameters Dynamic
	•	`oilTemperature` ← `EngineOilTemp` (double, Kelvin) - Note: Convert to Celsius
	•	`alternatorVoltage` ← `AltenatorVoltage` (double, volts)


#### Important Implementation Notes
##### Unit Conversions Required
Temperature conversions:
	•	NMEA2000 uses Kelvin, your structures use Celsius
	•	Use library function: `KelvinToC(value)` or `value - 273.15`
Speed conversions:
	•	NMEA2000 uses m/s, your structures use knots for wind/GPS speeds
	•	Use library function: `msToKnots(value)` or `value * 1.9438444924406047516198704103672`
Angular measurements:
	•	NMEA2000 uses radians (matches your structures)
	•	Library provides `RadToDeg()` and `DegToRad()` if needed
##### Data Availability Handling
The library uses special constants for unavailable
	•	`N2kDoubleNA` for double values
	•	`N2kInt8NA` for int8_t values
	•	`N2kUInt8NA`, `N2kUInt16NA`, `N2kUInt32NA` for unsigned integers
Check for these values before using data in your structures.


### Highest frequence source wins
Remember that highest frequency source for a given data element should be priorities. E.g. if a NMEA 2000 source is available for a data element, it will typically transmit at a higher frequency than NMEA 0183 source, and hence the NMEA 2000 source should be used. 
