# Data Model: NMEA 0183 Data Handlers

**Feature**: 006-nmea-0183-handlers
**Date**: 2025-10-11
**Status**: Complete

## Overview

This feature introduces sentence parsing components for NMEA 0183 protocol, converting raw sentence strings to structured data that updates the existing BoatData centralized repository. No new data storage structures are needed - feature reuses existing BoatData types (GPSData, CompassData, RudderData from BoatDataTypes.h).

---

## Core Entities

### 1. NMEA0183Sentence (Conceptual)

**Description**: Raw NMEA 0183 sentence string received from Serial2

**Format**: `$<talker_id><message_code>,<field1>,<field2>,...*<checksum>\r\n`

**Examples**:
```
$APRSA,15.0,A*3C
$APHDM,045.5,M*2F
$VHGGA,123519,5230.5000,N,00507.0000,E,1,08,0.9,545.4,M,46.9,M,,*47
$VHRMC,123519,A,5230.5000,N,00507.0000,E,5.5,054.7,230394,003.1,W*6A
$VHVTG,054.7,T,057.9,M,5.5,N,10.2,K*48
```

**Fields**:
- Talker ID: 2 chars (AP=autopilot, VH=VHF)
- Message Code: 3 chars (RSA, HDM, GGA, RMC, VTG)
- Fields: Comma-separated values (vary by message type)
- Checksum: XOR of all characters between $ and *

**Validation**:
- Checksum must match calculated value
- Talker ID must be "AP" or "VH" (others ignored)
- Message code must be one of 5 supported types (others ignored)

**Lifecycle**: Ephemeral (not stored, processed immediately)

**Handled By**: tNMEA0183 class (NMEA0183 library)

---

### 2. ParsedSentenceData (Intermediate)

**Description**: Extracted field values from a single NMEA 0183 sentence after parsing

**Structures** (one per sentence type):

#### RSA (Rudder Sensor Angle)
```cpp
struct ParsedRSA {
    double rudderAngle;      // Degrees, positive=starboard, negative=port
    char status;             // 'A'=valid, 'V'=invalid
    const char* talkerId;    // Must be "AP"
};
```

#### HDM (Heading Magnetic)
```cpp
struct ParsedHDM {
    double magneticHeading;  // Degrees, 0-360
    const char* talkerId;    // Must be "AP"
};
```

#### GGA (GPS Fix Data)
```cpp
struct ParsedGGA {
    double latitude;         // Decimal degrees, positive=North
    double longitude;        // Decimal degrees, positive=East
    int fixQuality;          // 0=invalid, 1=GPS fix, 2=DGPS fix
    int numSatellites;       // Number of satellites in use
    const char* talkerId;    // Must be "VH"
};
```

#### RMC (Recommended Minimum Navigation)
```cpp
struct ParsedRMC {
    double latitude;         // Decimal degrees
    double longitude;        // Decimal degrees
    double sog;              // Speed over ground, knots
    double cog;              // Course over ground, degrees true
    double variation;        // Magnetic variation, degrees (E=positive, W=negative)
    char status;             // 'A'=valid, 'V'=invalid
    const char* talkerId;    // Must be "VH"
};
```

#### VTG (Track Made Good)
```cpp
struct ParsedVTG {
    double trueCOG;          // Course over ground, degrees true
    double magneticCOG;      // Course over ground, degrees magnetic
    double sog;              // Speed over ground, knots
    const char* talkerId;    // Must be "VH"
};
```

**Validation Rules**:
- Latitude: [-90.0, 90.0]
- Longitude: [-180.0, 180.0]
- Heading: [0.0, 360.0]
- Rudder angle: [-90.0, 90.0]
- Variation: [-30.0, 30.0]
- SOG: [0.0, 100.0]
- Status: 'A' (valid) or 'V' (invalid)

**Lifecycle**: Ephemeral (extracted, validated, converted, discarded)

**Handled By**: Parser functions (NMEA0183ParseRSA, NMEA0183ParseHDM, etc.)

---

### 3. BoatData Integration (Existing)

**Description**: Centralized marine data repository (already implemented in R005)

**Updated Fields**:

#### From RSA → RudderData
```cpp
struct RudderData {
    double steeringAngle;    // Radians, positive=starboard (UPDATED by HandleRSA)
    bool available;
    unsigned long lastUpdate;
};
```

#### From HDM → CompassData
```cpp
struct CompassData {
    double trueHeading;      // Radians (NOT updated by HDM, calculated elsewhere)
    double magneticHeading;  // Radians (UPDATED by HandleHDM)
    double variation;        // Radians (UPDATED by HandleRMC/HandleVTG)
    bool available;
    unsigned long lastUpdate;
};
```

#### From GGA/RMC/VTG → GPSData
```cpp
struct GPSData {
    double latitude;         // Decimal degrees (UPDATED by HandleGGA, HandleRMC)
    double longitude;        // Decimal degrees (UPDATED by HandleGGA, HandleRMC)
    double cog;              // Radians, true (UPDATED by HandleRMC, HandleVTG)
    double sog;              // Knots (UPDATED by HandleRMC, HandleVTG)
    bool available;
    unsigned long lastUpdate;
};
```

**Update Method**: ISensorUpdate interface
```cpp
bool updateGPS(double lat, double lon, double cog, double sog, const char* sourceId);
bool updateCompass(double trueHdg, double magHdg, double variation, const char* sourceId);
bool updateRudder(double angle, const char* sourceId);
```

**Source IDs**:
- "NMEA0183-AP": Autopilot (RSA, HDM)
- "NMEA0183-VH": VHF radio (GGA, RMC, VTG)

**Priority**: Automatic via SourcePrioritizer (highest frequency wins)

**Reference**: src/types/BoatDataTypes.h, src/components/BoatData.h

---

## Component Architecture

### NMEA0183Handler (Coordinator)

**Responsibility**: Coordinate sentence reception, dispatch to parsers, update BoatData

**Dependencies**:
- ISerialPort (HAL): Read bytes from Serial2
- tNMEA0183 (library): Sentence parsing
- BoatData (existing): Data storage via ISensorUpdate
- WebSocketLogger (existing): Network debugging

**Public Interface**:
```cpp
class NMEA0183Handler {
public:
    /**
     * @brief Constructor
     * @param nmea0183 NMEA0183 library instance
     * @param serialPort Serial port interface (Serial2 adapter)
     * @param boatData BoatData repository
     * @param logger WebSocket logger for debugging
     */
    NMEA0183Handler(tNMEA0183* nmea0183, ISerialPort* serialPort,
                    BoatData* boatData, WebSocketLogger* logger);

    /**
     * @brief Initialize Serial2 and NMEA parser
     */
    void init();

    /**
     * @brief Process pending NMEA sentences (called from ReactESP loop)
     * Reads from serial port, parses sentences, dispatches to handlers
     */
    void processSentences();

private:
    tNMEA0183* nmea0183_;
    ISerialPort* serialPort_;
    BoatData* boatData_;
    WebSocketLogger* logger_;

    // Handler dispatch table
    struct HandlerEntry {
        const char* messageCode;
        void (NMEA0183Handler::*handler)(const tNMEA0183Msg&);
    };
    static const HandlerEntry handlers_[];

    // Sentence-specific handlers
    void handleRSA(const tNMEA0183Msg& msg);
    void handleHDM(const tNMEA0183Msg& msg);
    void handleGGA(const tNMEA0183Msg& msg);
    void handleRMC(const tNMEA0183Msg& msg);
    void handleVTG(const tNMEA0183Msg& msg);
};
```

**Behavior**:
1. `processSentences()` called every 10ms by ReactESP loop
2. Reads available bytes from `serialPort_`
3. Feeds bytes to `nmea0183_->ParseMessages()`
4. Library calls back to handler for each complete sentence
5. Handler validates talker ID, parses fields, converts units
6. Calls BoatData ISensorUpdate methods
7. Logs DEBUG on success, silent on failure (per FR-024/FR-025)

**File Location**: src/components/NMEA0183Handler.h/cpp

---

### ISerialPort (HAL Interface)

**Responsibility**: Abstract Serial2 hardware for testing

**Public Interface**:
```cpp
class ISerialPort {
public:
    /**
     * @brief Check number of bytes available to read
     * @return Number of bytes in receive buffer
     */
    virtual int available() = 0;

    /**
     * @brief Read one byte from serial port
     * @return Byte value (0-255) or -1 if no data available
     */
    virtual int read() = 0;

    /**
     * @brief Initialize serial port
     * @param baud Baud rate (4800 for NMEA 0183)
     */
    virtual void begin(unsigned long baud) = 0;

    virtual ~ISerialPort() = default;
};
```

**Implementations**:
- **ESP32SerialPort** (hal/implementations/): Wraps HardwareSerial (Serial2)
- **MockSerialPort** (mocks/): Provides pre-defined sentence strings for testing

**File Location**:
- Interface: src/hal/interfaces/ISerialPort.h
- ESP32: src/hal/implementations/ESP32SerialPort.h/cpp
- Mock: src/mocks/MockSerialPort.h/cpp

---

### SentenceParsers (Utility Functions)

**Responsibility**: Extract fields from NMEA 0183 sentences

**Custom Parser** (RSA not in library):
```cpp
/**
 * @brief Parse RSA (Rudder Sensor Angle) sentence
 * @param msg NMEA0183 message object
 * @param rudderAngle Output: rudder angle in degrees
 * @return true if successfully parsed and talker ID is "AP"
 */
bool NMEA0183ParseRSA(const tNMEA0183Msg& msg, double& rudderAngle);
```

**Library Parsers** (existing in NMEA0183 library):
- `NMEA0183ParseHDM(msg, heading)` - Heading Magnetic
- `NMEA0183ParseGGA(msg, lat, lon, time, ...)` - GPS Fix
- `NMEA0183ParseRMC(msg, lat, lon, cog, sog, ...)` - Recommended Minimum
- `NMEA0183ParseVTG(msg, trueCOG, magCOG, sog, ...)` - Track Made Good

**File Location**: src/utils/NMEA0183Parsers.h/cpp

---

### UnitConverter (Utility)

**Responsibility**: Convert NMEA 0183 units to BoatData units

**Public Interface**:
```cpp
class UnitConverter {
public:
    /**
     * @brief Convert degrees to radians
     * @param degrees Angle in degrees
     * @return Angle in radians
     */
    static double degreesToRadians(double degrees);

    /**
     * @brief Convert NMEA lat/lon format (DDMM.MMMM) to decimal degrees
     * @param degreesMinutes NMEA format (e.g., 5230.5000 = 52°30.5')
     * @param hemisphere 'N', 'S', 'E', or 'W'
     * @return Decimal degrees (positive = N/E, negative = S/W)
     */
    static double nmeaCoordinateToDecimal(double degreesMinutes, char hemisphere);

    /**
     * @brief Calculate magnetic variation from true and magnetic COG
     * @param trueCOG True course over ground (degrees)
     * @param magCOG Magnetic course over ground (degrees)
     * @return Variation in degrees (positive = East, negative = West)
     */
    static double calculateVariation(double trueCOG, double magCOG);

    /**
     * @brief Normalize angle to [0, 2π] range
     * @param radians Angle in radians
     * @return Normalized angle in [0, 2π]
     */
    static double normalizeAngle(double radians);
};
```

**File Location**: src/utils/UnitConverter.h/cpp

---

## Data Flow Diagrams

### Flow 1: RSA Sentence Processing (Autopilot → Rudder Angle)

```
Serial2 (4800 baud)
    ↓ (bytes)
ESP32SerialPort (ISerialPort)
    ↓ (bytes via available()/read())
tNMEA0183::ParseMessages()
    ↓ (validates checksum, extracts fields)
NMEA0183Handler::handleRSA()
    ↓ (validates talker ID = "AP")
NMEA0183ParseRSA()
    ↓ (extracts rudderAngle in degrees)
UnitConverter::degreesToRadians()
    ↓ (converts to radians)
BoatData::updateRudder(angle, "NMEA0183-AP")
    ↓ (validates range, updates if priority allows)
RudderData::steeringAngle
```

**Timing**: <15ms per sentence (50ms budget per FR-027)

---

### Flow 2: GGA Sentence Processing (VHF → GPS Position)

```
Serial2 (4800 baud)
    ↓
ESP32SerialPort
    ↓
tNMEA0183::ParseMessages()
    ↓
NMEA0183Handler::handleGGA()
    ↓ (validates talker ID = "VH")
NMEA0183ParseGGA() (library)
    ↓ (extracts lat/lon in DDMM.MMMM format)
UnitConverter::nmeaCoordinateToDecimal()
    ↓ (converts to decimal degrees)
BoatData::updateGPS(lat, lon, 0, 0, "NMEA0183-VH")
    ↓ (validates range, checks priority)
GPSData::latitude, longitude
```

---

### Flow 3: VTG Sentence Processing (VHF → COG/SOG + Variation)

```
Serial2
    ↓
ESP32SerialPort
    ↓
tNMEA0183::ParseMessages()
    ↓
NMEA0183Handler::handleVTG()
    ↓ (validates talker ID = "VH")
NMEA0183ParseVTG() (library)
    ↓ (extracts trueCOG, magCOG, sog)
UnitConverter::calculateVariation(trueCOG, magCOG)
    ↓ (validates variation in [-30°, 30°])
UnitConverter::degreesToRadians()
    ↓ (converts COG and variation)
BoatData::updateGPS(..., cog, sog, "NMEA0183-VH")
BoatData::updateCompass(..., variation, "NMEA0183-VH")
    ↓
GPSData::cog, sog
CompassData::variation
```

---

## State Management

**No Persistent State**: Feature is stateless (sentence processing is event-driven)

**Transient State**:
- Serial receive buffer: 64 bytes (HardwareSerial default)
- NMEA sentence buffer: 82 bytes max (NMEA 0183 spec)
- Parsed data structures: Stack-allocated during handler execution

**Concurrency**: Single-threaded ReactESP event loop (no locking required)

**Error Recovery**: None needed (malformed sentences silently discarded, next sentence processed)

---

## Validation Rules Summary

| Field | Source Sentence | Range | Action on Violation |
|-------|----------------|-------|---------------------|
| Latitude | GGA, RMC | [-90.0, 90.0] | Silent discard (FR-026) |
| Longitude | GGA, RMC | [-180.0, 180.0] | Silent discard (FR-026) |
| Rudder Angle | RSA | [-90.0, 90.0] | Silent discard (FR-026) |
| Heading | HDM | [0.0, 360.0] | Silent discard (FR-026) |
| Variation | RMC, VTG | [-30.0, 30.0] | Silent discard (FR-026) |
| SOG | RMC, VTG | [0.0, 100.0] | Silent discard (FR-026) |
| Checksum | All | Must match | Silent discard (FR-024) |
| Talker ID | All | "AP" or "VH" | Silent ignore (FR-006) |
| Message Code | All | RSA/HDM/GGA/RMC/VTG | Silent ignore (FR-007) |

---

## Memory Allocation

**Static Allocation** (Constitutional Principle II compliance):

| Component | Size | Justification |
|-----------|------|---------------|
| NMEA0183Handler instance | 32 bytes | Pointers + state flags |
| Sentence buffer (in tNMEA0183) | 82 bytes | NMEA 0183 max sentence length |
| ESP32SerialPort | 8 bytes | HardwareSerial pointer |
| Handler dispatch table | 40 bytes | 5 entries × 8 bytes/entry |
| **Total** | **162 bytes** | 0.05% of 320KB ESP32 RAM ✓ |

**No Heap Allocation**: All structures stack-allocated during sentence processing

**Flash Impact**: ~15KB (parsers, handlers, utilities)

---

## Dependencies

**External**:
- NMEA0183 library (ttlappalainen/NMEA0183)
- Arduino Core ESP32 (HardwareSerial)
- ReactESP (event loop integration)

**Internal**:
- BoatData (R005): ISensorUpdate interface
- WebSocketLogger: Network debugging
- Serial2 hardware: UART2 (RX=GPIO25, TX=GPIO27)

**Initialization Order**:
1. Serial2.begin(4800) via ESP32SerialPort
2. NMEA0183Handler initialization
3. ReactESP loop registration: `app.onRepeat(10, processSentences)`

---

## Extension Points

**Future Sentence Types**: Add handler entry to dispatch table
```cpp
{"MWV", &NMEA0183Handler::handleMWV},  // Wind data
```

**Future Talker IDs**: Add to talker ID filter logic
```cpp
if (strcmp(msg.Sender(), "AP") != 0 &&
    strcmp(msg.Sender(), "VH") != 0 &&
    strcmp(msg.Sender(), "II") != 0) {  // Integrated Instruments
    return;
}
```

**Custom Parsers**: Follow NMEA0183ParseRSA pattern (return bool, extract via Field() method)

---

**Data Model Complete**: Ready for contract generation (Phase 1 continuation)
