# Contract: TalkerIdLookup

**Component**: `src/utils/TalkerIdLookup.h` / `.cpp`
**Purpose**: Static lookup table mapping NMEA0183 talker IDs to device type descriptions
**Type**: Stateless utility (pure functions, PROGMEM data)

## Responsibilities

1. Map NMEA0183 talker IDs (2-character strings) to device type descriptions
2. Provide "Unknown NMEA0183 Device" fallback for unrecognized talker IDs
3. Store lookup table in PROGMEM (flash memory) to conserve RAM

## Public Interface

```cpp
/**
 * @brief Get device type description from NMEA0183 talker ID
 *
 * @param talkerId 2-character talker ID (e.g., "AP", "GP", "VH")
 * @return Device type description (e.g., "Autopilot", "GPS Receiver") or "Unknown NMEA0183 Device"
 *
 * @pre talkerId is null-terminated string (length ≥ 2)
 * @note Returns static string (no allocation), valid for program lifetime
 * @note Case-sensitive (NMEA0183 talker IDs are uppercase by standard)
 * @note Thread-safe (read-only data)
 */
const char* getTalkerDescription(const char* talkerId);
```

## Behavior Contracts

### BC-1: Common Talker IDs
**Requirement**: Lookup table includes ≥15 common NMEA0183 talker IDs

**Test Data** (minimum required entries):
| Talker ID | Device Type | Reference |
|-----------|-------------|-----------|
| AP | Autopilot | Steering control |
| GP | GPS Receiver | Position/navigation |
| HC | Heading Compass | Magnetic heading |
| VH | VHF Radio | Communication |
| VW | Wind Sensor | Wind speed/direction |
| SD | Depth Sounder | Water depth |
| II | Integrated Instrumentation | Generic |
| YX | Transducer | Multi-sensor |
| EC | Electronic Chart System | Chartplotter |
| CD | Digital Selective Calling | VHF DSC |
| GL | GLONASS Receiver | GPS alternative |
| GN | GNSS Receiver | Multi-constellation |
| BD | BeiDou Receiver | Chinese GNSS |
| GA | Galileo Receiver | European GNSS |
| AI | AIS | Automatic Identification |

**Test**:
```cpp
TEST_CASE("getTalkerDescription returns correct descriptions for known IDs") {
    REQUIRE(strcmp(getTalkerDescription("AP"), "Autopilot") == 0);
    REQUIRE(strcmp(getTalkerDescription("GP"), "GPS Receiver") == 0);
    REQUIRE(strcmp(getTalkerDescription("VH"), "VHF Radio") == 0);
    // ... test all 15+ entries
}
```

### BC-2: Unknown Talker IDs
**Requirement**: Unrecognized talker IDs return "Unknown NMEA0183 Device"

**Test**:
```cpp
TEST_CASE("getTalkerDescription returns Unknown fallback for unrecognized IDs") {
    REQUIRE(strcmp(getTalkerDescription("ZZ"), "Unknown NMEA0183 Device") == 0);
    REQUIRE(strcmp(getTalkerDescription("XX"), "Unknown NMEA0183 Device") == 0);
}
```

### BC-3: Case Sensitivity
**Requirement**: Lookup is case-sensitive (NMEA0183 standard uses uppercase)

**Test**:
```cpp
TEST_CASE("getTalkerDescription is case-sensitive") {
    REQUIRE(strcmp(getTalkerDescription("AP"), "Autopilot") == 0);  // Uppercase OK
    REQUIRE(strcmp(getTalkerDescription("ap"), "Unknown NMEA0183 Device") == 0);  // Lowercase not found
}
```

### BC-4: Performance
**Requirement**: Lookup completes in <20μs (linear search for ~15 entries)

**Test**:
```cpp
TEST_CASE("getTalkerDescription completes in <20 microseconds") {
    unsigned long start = micros();
    getTalkerDescription("AP");
    unsigned long duration = micros() - start;
    REQUIRE(duration < 20);
}
```

### BC-5: Memory Usage
**Requirement**: Lookup table stored in PROGMEM (flash), not RAM

**Implementation**:
```cpp
const TalkerEntry PROGMEM talkerTable[] = {
    {"AP", "Autopilot"},
    {"GP", "GPS Receiver"},
    // ...
};
```

## Error Handling

### E-1: Null Pointer
**Scenario**: `talkerId == nullptr`

**Behavior**: Undefined (caller responsible for valid input per precondition)

**Rationale**: Performance-critical utility, no runtime checks

### E-2: Short String
**Scenario**: `talkerId` length < 2 characters

**Behavior**: Comparison fails, returns "Unknown NMEA0183 Device"

**Test**:
```cpp
TEST_CASE("getTalkerDescription handles short strings gracefully") {
    REQUIRE(strcmp(getTalkerDescription("A"), "Unknown NMEA0183 Device") == 0);
    REQUIRE(strcmp(getTalkerDescription(""), "Unknown NMEA0183 Device") == 0);
}
```

## Data Source

**Reference**: NMEA0183 talker IDs defined in IEC 61162-1 standard:
- Official: IEC 61162-1:2016 standard (paid document)
- Community: https://gpsd.gitlab.io/gpsd/NMEA.html (open reference)

**Maintenance**: Rarely updated (NMEA0183 is legacy standard, new talker IDs infrequent).

---

**Contract Version**: 1.0
**Last Updated**: 2025-10-13
