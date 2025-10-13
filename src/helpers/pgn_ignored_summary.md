# Ignored NMEA2000 PGNs - Summary

This file lists all unique PGNs that are ignored by the current NMEA2000Handlers implementation.
Total unique PGNs: 24

## Proprietary Single-Frame PGNs (65280-65535)

These PGNs are manufacturer-specific and their exact purpose depends on the device transmitting them.

**65280** - Proprietary: Manufacturer Specific (Single Frame)
- Range: 65280-65535 are manufacturer proprietary single-frame PGNs
- Usage varies by manufacturer (e.g., Furuno: Heave, Xantrex: Device Control Status)

**65350** - Proprietary: Manufacturer Specific (Single Frame)
- Most common proprietary PGN in this log (appears ~300 times)
- Purpose unknown without manufacturer documentation

**65396** - Proprietary: Manufacturer Specific (Single Frame)
- Another proprietary single-frame message
- Purpose unknown without manufacturer documentation

## System Management PGNs

**126992** - System Time
- Provides date and time information for the NMEA2000 network
- Used for time synchronization across devices

**126993** - Heartbeat
- Indicates that a device is alive and operational
- Periodic message sent by network devices

## Navigation PGNs

**128275** - Distance Log
- Cumulative distance traveled through water
- Used for trip and total distance tracking

**129283** - Cross Track Error
- Distance off intended course line
- Used for navigation and autopilot systems

**129284** - Navigation Data
- Waypoint information, bearing, distance to destination
- Route navigation information

**129285** - Navigation - Route/WP Information
- Route and waypoint detailed information
- Used by chartplotters and navigation systems

**129540** - GNSS Sats in View
- Information about visible GPS satellites
- Satellite signal strength, elevation, azimuth

**129539** - GNSS DOPs (Dilution of Precision)
- GPS accuracy metrics (HDOP, VDOP, TDOP)
- Quality indicators for GPS fix

## AIS (Automatic Identification System) PGNs

**129038** - AIS Class A Position Report
- Position, speed, and course from commercial vessels (Class A AIS)
- High-rate position updates from large ships

**129039** - AIS Class B Position Report
- Position, speed, and course from recreational vessels (Class B AIS)
- Lower-rate position updates from smaller boats

**129041** - AIS Aids to Navigation (AtoN) Report
- Position and information about navigation aids (buoys, beacons, etc.)
- Transmitted by virtual or physical AtoN devices

**129793** - AIS UTC and Date Report
- Time and date information from AIS base stations
- ITU-R M.1371 message 4 (Base Station Report) and message 11 (UTC/Date Response)

**129794** - AIS Class A Static and Voyage Related Data
- Vessel details: name, callsign, dimensions, destination
- Voyage information from commercial vessels

**129809** - AIS Safety Related Broadcast Message
- Safety-related text messages broadcast to all vessels
- Used for navigational warnings and safety information

**129810** - AIS Class B 'CS' Static Data Report, Part B
- Vessel identification and characteristics for Class B AIS
- Static information from recreational vessels

## Environmental PGNs

**130310** - Wind Data (Note: conflicts with implemented PGN 130306)
- **WARNING**: This may be a duplicate or variant of PGN 130306 (Wind Data) which is already handled
- Verify device documentation - may be proprietary extension

**130311** - Environmental Parameters (DEPRECATED)
- **DEPRECATED** - Use specific environmental PGNs instead
- Legacy message for environmental data (temperature, humidity, pressure)

## Proprietary Fast-Packet PGNs

**130822** - Proprietary: Navico/B&G/Simrad/Lowrance
- Manufacturer-specific message from Navico brands
- Appears frequently (22 occurrences in log)
- Purpose unknown without Navico documentation

**130824** - Proprietary: Maretron Annunciator
- Maretron-specific message for annunciator/alarm systems
- Used by Maretron instrumentation

**130934** - Proprietary: Manufacturer Specific (Fast Packet)
- Range: 130816-131071 are manufacturer proprietary fast-packet PGNs
- Purpose unknown without manufacturer documentation

**130935** - Proprietary: Manufacturer Specific (Fast Packet)
- Another proprietary fast-packet message
- Purpose unknown without manufacturer documentation

---

## Analysis Summary

### Most Frequent Ignored PGNs (from log sample):
1. **PGN 65350** - ~300 occurrences (proprietary single-frame)
2. **PGN 65280** - ~100 occurrences (proprietary single-frame)
3. **PGN 130822** - ~22 occurrences (Navico proprietary)
4. **PGN 129284** - ~15 occurrences (Navigation Data)
5. **PGN 129539** - ~15 occurrences (GNSS DOPs)

### PGN Categories:
- **Proprietary PGNs**: 7 (65280, 65350, 65396, 130822, 130824, 130934, 130935)
- **AIS PGNs**: 7 (129038, 129039, 129041, 129793, 129794, 129809, 129810)
- **Navigation PGNs**: 5 (128275, 129283, 129284, 129285, 129540)
- **Environmental PGNs**: 2 (130310, 130311)
- **System PGNs**: 2 (126992, 126993)
- **GPS Quality PGNs**: 1 (129539)

### Recommendations:

1. **Consider implementing if relevant to your boat:**
   - **PGN 129283** (Cross Track Error) - if using autopilot with route navigation
   - **PGN 129284** (Navigation Data) - if using chartplotter with waypoint navigation
   - **PGN 126992** (System Time) - for accurate timestamping
   - **PGN 129540** (GNSS Sats in View) - for GPS quality monitoring

2. **AIS PGNs (129038-129810):**
   - Only implement if you have AIS receiver and want collision avoidance
   - These are vessel traffic messages, not boat sensor data

3. **Proprietary PGNs (65xxx, 130822, 130824, 130934, 130935):**
   - Cannot be implemented without manufacturer documentation
   - If you identify the device transmitting PGN 65350, consult its manual
   - PGN 130822 suggests Navico/B&G/Simrad/Lowrance device on your network

4. **DEPRECATED PGN 130311:**
   - Do not implement - use specific environmental PGNs instead

5. **PGN 130310 Warning:**
   - Verify this is not conflicting with your implemented PGN 130306 (Wind Data)
   - May be proprietary variant or incorrect PGN number in device

---

**Generated**: 2025-10-12
**Source Log**: src/helpers/pgn_ignored.log (42-55 seconds sample)
**References**:
- NMEA2000 Library Documentation: https://ttlappalainen.github.io/NMEA2000/list_msg.html
- CANboat PGN Database: https://github.com/canboat/canboat
