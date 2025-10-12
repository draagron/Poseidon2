## NMEA 0183 message handling

Implement handlers for NMEA 0183 sentences as source for select BoatData elements. 

For all sentences, except the RSA sentence, use the https://github.com/ttlappalainen/NMEA0183 library for parsing the sentences. 

The RSA sentence is oddly enough not implemented in the https://github.com/ttlappalainen/NMEA0183 library. Implement a parser for RSA similar to the structure used for other messages in the library, and use perhaps the example code in examples/poseidongw/src/NMEA0183Handlers.cpp for inspiration. 

Implement any necessary conversion of units of measure. The src/types/BoatDataTypes.h defines the target units of measure. 

The NMEA 0183 handling should only cover sentences from the following 2 devices: 

Device | NMEA Talker ID 
Autopilot | AP
VHF | VH

Only the following sentences should be handled: 

APRSA
APHDM

VHVTG
VHGGA
VHRMC

### Mapping to BoatData

The NMEA sentences should be mapped to BoatData elements as follows:

Talker ID |  Sentences  |  BoatData Elelement  | Notes  

AP  |  RSA  |  RudderData/steeringAngle   | Implement new parser, similar to the one in examples/poseidongw/src/NMEA0183Handlers.cpp. 

AP  |  HDM  |  CompassData/magneticHeading | Use existing parser from https://github.com/ttlappalainen/NMEA0183/blob/master/NMEA0183Messages.h

VH  | VTG   |  GPSData/cog, GPSData/sog , GPSData/variation | "VTG gives COG true in radians, SOG in m/s, and COG magnetic in radians. Calculate the variation as difference between the two COG data elements". Use parser from https://github.com/ttlappalainen/NMEA0183/blob/master/NMEA0183Messages.h. Also update the CompassData/trueHeading based on CompassData/trueHeading and variation.


VH  | GGA |  GPSData/latitude, GPSData/longitude | Use parser from https://github.com/ttlappalainen/NMEA0183/blob/master/NMEA0183Messages.h

VH  | RMC |  GPSData/latitude, GPSData/longitude , GPSData/cog, GPSData/sog, GPSData/variation | Use parser from https://github.com/ttlappalainen/NMEA0183/blob/master/NMEA0183Messages.h. Also update the CompassData/trueHeading based on CompassData/trueHeading and variation.



### OUT OF SCOPE ###
Any NMEA 0183 sentence, like XDR
### Implementation Considerations

Sentence Priority:
For overlapping data (like position from multiple sources), establish priority: RMC > GGA > GLL for GPS data, HDG > HDM/HDT for heading data.

### Highest frequence source wins
Remember that highest frequency source for a given data element should be priorities. E.g. if a NMEA 2000 source is available for a data element, it will typically transmit at a higher frequency than NMEA 0183 source, and hence the NMEA 2000 source should be used. 

### Library and example code
Use the https://github.com/ttlappalainen/NMEA0183 library for the implementation. 
Use the examples/poseidongw/src/NMEA0183Handlers.cpp and examples/poseidongw/src/NMEA0183Handlers.h as inspiration but try to follow same style as used for NMEA 2000. 