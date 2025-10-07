# BoatData Contracts

**Feature**: BoatData - Centralized Marine Data Model
**Date**: 2025-10-06

## Overview
This directory contains C++ interface contracts for the BoatData feature. These are abstract interfaces that define the public API without implementation details.

## Contracts

### 1. IBoatDataStore.h
**Purpose**: Abstract interface for data storage access
**Methods**:
- `GPSData getGPSData()`
- `void setGPSData(const GPSData& data)`
- `CompassData getCompassData()`
- `void setCompassData(const CompassData& data)`
- `WindData getWindData()`
- `void setWindData(const WindData& data)`
- `SpeedData getSpeedData()`
- `void setSpeedData(const SpeedData& data)`
- `RudderData getRudderData()`
- `void setRudderData(const RudderData& data)`
- `DerivedData getDerivedData()`
- `void setDerivedData(const DerivedData& data)`
- `CalibrationData getCalibration()`
- `void setCalibration(const CalibrationData& data)`

**Usage**: Allows mocking of BoatData for unit tests

### 2. ISensorUpdate.h
**Purpose**: Interface for NMEA/1-Wire handlers to update sensor data
**Methods**:
- `bool updateGPS(double lat, double lon, double cog, double sog, const char* sourceId)`
- `bool updateCompass(double trueHdg, double magHdg, double variation, const char* sourceId)`
- `bool updateWind(double awa, double aws, const char* sourceId)`
- `bool updateSpeed(double heelAngle, double boatSpeed, const char* sourceId)`
- `bool updateRudder(double angle, const char* sourceId)`

**Return**: `true` if update accepted (valid), `false` if rejected (outlier/invalid)

**Usage**: Called by NMEA0183/NMEA2000/1-Wire message handlers

### 3. ICalibration.h
**Purpose**: Interface for calibration parameter access
**Methods**:
- `CalibrationParameters getCalibration()`
- `bool setCalibration(const CalibrationParameters& params)`
- `bool validateCalibration(const CalibrationParameters& params)`
- `bool loadFromFlash()`
- `bool saveToFlash(const CalibrationParameters& params)`

**Usage**: Used by CalibrationWebServer and CalculationEngine

### 4. ISourcePrioritizer.h
**Purpose**: Interface for multi-source management
**Methods**:
- `int registerSource(const char* sourceId, SensorType type, ProtocolType protocol)`
- `void updateSourceTimestamp(int sourceIndex, unsigned long timestamp)`
- `int getActiveSource(SensorType type)`
- `void setManualOverride(int sourceIndex)`
- `void clearManualOverride(SensorType type)`
- `bool isSourceStale(int sourceIndex, unsigned long currentTime)`

**Usage**: Manages source priority and failover logic

## Contract Tests

Each contract has corresponding contract tests in `/test/contract/`:

- `test_iboatdatastore_contract.cpp`
- `test_isensorupdate_contract.cpp`
- `test_icalibration_contract.cpp`
- `test_isourceprioritizer_contract.cpp`

Contract tests assert:
1. Interface methods callable
2. Data round-trips correctly (write → read)
3. Validation rules enforced
4. Error conditions handled

## Implementation

Concrete implementations:
- `src/components/BoatData.cpp` implements `IBoatDataStore`, `ISensorUpdate`
- `src/components/CalibrationManager.cpp` implements `ICalibration`
- `src/components/SourcePrioritizer.cpp` implements `ISourcePrioritizer`

Mock implementations for testing:
- `src/mocks/MockBoatDataStore.h`
- `src/mocks/MockCalibration.h`
- `src/mocks/MockSourcePrioritizer.h`

## Dependencies

Contracts depend only on:
- Standard C++ types (`double`, `bool`, `char*`)
- BoatData structure definitions (from `data-model.md`)
- No hardware dependencies (pure interfaces)

---

**Contracts Status**: ✅ Defined
**Next Step**: Implement contract tests (TDD - tests first, then implementation)
