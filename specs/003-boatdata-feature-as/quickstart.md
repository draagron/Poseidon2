# Quickstart: BoatData Integration Tests

**Feature**: BoatData - Centralized Marine Data Model
**Date**: 2025-10-06
**Purpose**: Executable integration test scenarios derived from acceptance criteria

## Overview
This document provides step-by-step integration test scenarios that validate the BoatData feature against the acceptance criteria defined in `spec.md`. Each scenario can be executed manually or automated in the test suite.

---

## Prerequisites

**Hardware** (for hardware tests):
- ESP32 board (SH-ESP32 or compatible)
- USB cable for serial monitoring
- Optional: NMEA0183/2000 simulator or real sensors

**Software**:
- PlatformIO installed
- Firmware built and uploaded
- Serial monitor or UDP listener on port 4444

**Test Mode**:
- Most scenarios can run with **mocked sensors** (unit/integration tests)
- Timing validation requires **hardware** (ESP32)

---

## Scenario 1: Single Source GPS Data

**Acceptance Criterion** (from spec.md):
> Given the system has one GPS sensor connected via NMEA0183, when the GPS sensor sends position data (latitude/longitude), then the system stores and makes available the current position data.

### Test Steps

1. **Setup**:
   ```cpp
   // In test: Create mock NMEA0183 GPS source
   ISensorUpdate* boatData = new BoatData();
   const char* sourceId = "GPS-NMEA0183";
   ```

2. **Action**: Send GPS position data
   ```cpp
   double lat = 40.7128;   // New York City latitude
   double lon = -74.0060;  // New York City longitude
   double cog = 1.571;     // 90° (π/2 rad), heading East
   double sog = 5.5;       // 5.5 knots

   bool accepted = boatData->updateGPS(lat, lon, cog, sog, sourceId);
   ```

3. **Assertions**:
   ```cpp
   assert(accepted == true);  // Update accepted

   GPSData gps = boatData->getGPSData();
   assert(gps.available == true);
   assert(fabs(gps.latitude - 40.7128) < 0.0001);
   assert(fabs(gps.longitude + 74.0060) < 0.0001);
   assert(fabs(gps.cog - 1.571) < 0.01);
   assert(fabs(gps.sog - 5.5) < 0.1);
   assert(gps.lastUpdate > 0);  // Timestamp recorded
   ```

4. **Expected Result**: ✅ GPS data stored and retrievable

**Test File**: `test/integration/test_single_gps_source.cpp`

---

## Scenario 2: Multi-Source GPS with Automatic Prioritization

**Acceptance Criterion** (from spec.md):
> Given the system has GPS-A (NMEA0183) updating at 1 Hz and GPS-B (NMEA2000) updating at 10 Hz, when both sensors are providing valid data, then the system uses GPS-B data (higher update frequency = higher priority).

### Test Steps

1. **Setup**: Register two GPS sources
   ```cpp
   ISourcePrioritizer* prioritizer = new SourcePrioritizer();
   ISensorUpdate* boatData = new BoatData();

   int gpsA_idx = prioritizer->registerSource("GPS-NMEA0183", SensorType::GPS, ProtocolType::NMEA0183);
   int gpsB_idx = prioritizer->registerSource("GPS-NMEA2000", SensorType::GPS, ProtocolType::NMEA2000);
   ```

2. **Action**: Simulate 10 seconds of GPS updates
   ```cpp
   // GPS-A: 1 Hz (1 update/second)
   for (int sec = 0; sec < 10; sec++) {
       boatData->updateGPS(40.0 + sec*0.001, -74.0, 0.0, 5.0, "GPS-NMEA0183");
       delay(1000);  // 1 second
   }

   // GPS-B: 10 Hz (10 updates/second)
   for (int sec = 0; sec < 10; sec++) {
       for (int i = 0; i < 10; i++) {
           boatData->updateGPS(40.0 + sec*0.001, -74.0, 0.0, 5.0, "GPS-NMEA2000");
           delay(100);  // 100ms
       }
   }

   // Trigger priority recalculation
   prioritizer->updatePriorities();
   ```

3. **Assertions**:
   ```cpp
   int activeSource = prioritizer->getActiveSource(SensorType::GPS);
   assert(activeSource == gpsB_idx);  // GPS-B is active

   // Verify frequency calculation
   SensorSource source = prioritizer->getSource(gpsB_idx);
   assert(source.updateFrequency > 9.0 && source.updateFrequency < 11.0);  // ~10 Hz
   ```

4. **Expected Result**: ✅ GPS-B selected as active source (higher frequency)

**Test File**: `test/integration/test_multi_source_priority.cpp`

---

## Scenario 3: Source Failover

**Acceptance Criterion** (from spec.md):
> Given the system is using GPS-B (10 Hz) as the primary source, when GPS-B stops sending data, then the system automatically switches to GPS-A (1 Hz) as the fallback source.

### Test Steps

1. **Setup**: Start with GPS-B active (from Scenario 2)
   ```cpp
   // GPS-B is active from previous test
   int activeSource = prioritizer->getActiveSource(SensorType::GPS);
   assert(activeSource == gpsB_idx);
   ```

2. **Action**: GPS-B stops, GPS-A continues
   ```cpp
   // Continue GPS-A updates
   for (int sec = 0; sec < 7; sec++) {
       boatData->updateGPS(40.0, -74.0, 0.0, 5.0, "GPS-NMEA0183");
       delay(1000);
   }

   // GPS-B sends NO updates (simulating failure)
   // Wait >5 seconds (stale threshold)

   // Trigger stale detection
   prioritizer->checkStale(millis());
   ```

3. **Assertions**:
   ```cpp
   // GPS-B should be marked unavailable
   SensorSource sourceB = prioritizer->getSource(gpsB_idx);
   assert(sourceB.available == false);

   // GPS-A should become active
   int activeSource = prioritizer->getActiveSource(SensorType::GPS);
   assert(activeSource == gpsA_idx);

   // GPS data should still be available (from GPS-A)
   GPSData gps = boatData->getGPSData();
   assert(gps.available == true);
   ```

4. **Expected Result**: ✅ Automatic failover from GPS-B to GPS-A

**Test File**: `test/integration/test_source_failover.cpp`

---

## Scenario 4: User Priority Override

**Acceptance Criterion** (from spec.md):
> Given the system has auto-prioritized GPS-B over GPS-A, when the user manually sets GPS-A as the preferred source via configuration, then the system uses GPS-A regardless of update frequency.

### Test Steps

1. **Setup**: GPS-B is auto-prioritized (from Scenario 2)
   ```cpp
   assert(prioritizer->getActiveSource(SensorType::GPS) == gpsB_idx);
   ```

2. **Action**: User sets manual override via web API
   ```cpp
   // Simulate web API call: POST /api/source-override {"type":"GPS","sourceId":"GPS-NMEA0183"}
   prioritizer->setManualOverride(gpsA_idx);
   ```

3. **Assertions**:
   ```cpp
   // GPS-A should be active despite lower frequency
   int activeSource = prioritizer->getActiveSource(SensorType::GPS);
   assert(activeSource == gpsA_idx);

   // Verify manual override flag
   SensorSource sourceA = prioritizer->getSource(gpsA_idx);
   assert(sourceA.manualOverride == true);

   // GPS-B should still be available but not active
   SensorSource sourceB = prioritizer->getSource(gpsB_idx);
   assert(sourceB.available == true);
   assert(sourceB.active == false);
   ```

4. **Expected Result**: ✅ GPS-A active despite lower frequency

**Note**: Manual override is volatile (FR-013), resets to automatic on reboot.

**Test File**: `test/integration/test_manual_override.cpp`

---

## Scenario 5: Derived Parameter Calculation

**Acceptance Criterion** (from spec.md):
> Given the system has valid data from GPS, compass, wind vane, and speed sensor, when the calculation cycle triggers (every 200ms), then the system calculates and updates all derived parameters (TWS, TWA, STW, VMG, SOC, DOC, leeway).

### Test Steps

1. **Setup**: Provide complete sensor data
   ```cpp
   ISensorUpdate* boatData = new BoatData();

   // GPS data
   boatData->updateGPS(40.7128, -74.0060, 1.571, 6.0, "GPS-Test");  // COG=90°, SOG=6kts

   // Compass data
   boatData->updateCompass(1.571, 1.571, 0.0, "Compass-Test");  // Heading=90° (East), variation=0

   // Wind data
   boatData->updateWind(0.785, 12.0, "Wind-Test");  // AWA=45° starboard, AWS=12kts

   // Speed data
   boatData->updateSpeed(0.175, 5.5, "Speed-Test");  // Heel=10° starboard, boat speed=5.5kts
   ```

2. **Action**: Trigger calculation cycle
   ```cpp
   CalculationEngine engine;
   engine.calculate(boatData);  // 200ms cycle
   ```

3. **Assertions**: Verify all derived parameters calculated
   ```cpp
   DerivedData derived = boatData->getDerivedData();

   assert(derived.available == true);

   // AWA corrections
   assert(derived.awaOffset != 0.0);  // Corrected for masthead offset
   assert(derived.awaHeel != 0.0);    // Corrected for heel

   // Leeway and STW
   assert(derived.leeway != 0.0);     // Leeway calculated from heel and speed
   assert(derived.stw > 0.0);         // Speed through water > 0

   // True wind
   assert(derived.tws > 0.0);         // True wind speed calculated
   assert(fabs(derived.twa) < M_PI);  // True wind angle in range [-π, π]
   assert(derived.wdir >= 0.0 && derived.wdir <= 2*M_PI);  // Wind direction [0, 2π]

   // Performance
   assert(derived.vmg != 0.0);        // Velocity made good calculated

   // Current
   assert(derived.soc >= 0.0);        // Current speed >= 0
   assert(derived.doc >= 0.0 && derived.doc <= 2*M_PI);  // Current direction [0, 2π]

   // Timestamp
   assert(derived.lastUpdate > 0);
   ```

4. **Expected Result**: ✅ All 11 derived parameters calculated successfully

**Test File**: `test/integration/test_derived_calculation.cpp`

---

## Scenario 6: Calibration Parameter Update

**Acceptance Criterion** (from spec.md):
> Given the user accesses the web calibration interface, when the user updates the "Leeway Calibration Factor" from 0.5 to 0.65, then the system persists the new calibration value and uses it in subsequent calculations.

### Test Steps

1. **Setup**: Load default calibration (K=1.0)
   ```cpp
   ICalibration* calibMgr = new CalibrationManager(fileSystem);
   calibMgr->loadFromFlash();  // Loads defaults if file missing

   CalibrationParameters calib = calibMgr->getCalibration();
   assert(fabs(calib.leewayCalibrationFactor - 1.0) < 0.01);  // Default K=1.0
   ```

2. **Action**: Update via web API
   ```cpp
   // Simulate: POST /api/calibration {"leewayKFactor": 0.65}
   CalibrationParameters newCalib;
   newCalib.leewayCalibrationFactor = 0.65;
   newCalib.windAngleOffset = 0.0;

   bool valid = calibMgr->validateCalibration(newCalib);
   assert(valid == true);

   bool saved = calibMgr->setCalibration(newCalib);
   assert(saved == true);
   ```

3. **Assertions**: Verify persistence and immediate use
   ```cpp
   // Check in-memory value updated
   calib = calibMgr->getCalibration();
   assert(fabs(calib.leewayCalibrationFactor - 0.65) < 0.01);

   // Verify persisted to flash
   calibMgr = new CalibrationManager(fileSystem);  // Re-create instance
   calibMgr->loadFromFlash();
   calib = calibMgr->getCalibration();
   assert(fabs(calib.leewayCalibrationFactor - 0.65) < 0.01);  // Persisted

   // Verify next calculation uses new value
   CalculationEngine engine;
   engine.calculate(boatData);  // Uses K=0.65 in leeway formula
   DerivedData derived = boatData->getDerivedData();
   // (Leeway value will be different than with K=1.0)
   ```

4. **Expected Result**: ✅ Calibration persisted and applied immediately

**Test File**: `test/integration/test_calibration_update.cpp`

---

## Scenario 7: Outlier Detection

**Acceptance Criterion** (from spec.md):
> Given the GPS sensor has been reporting valid position data (latitude 40.7128°N), when the GPS sensor sends an invalid reading (latitude 200°N - outside valid range), then the system rejects the outlier and continues using the last valid position.

### Test Steps

1. **Setup**: Establish valid GPS data
   ```cpp
   ISensorUpdate* boatData = new BoatData();

   // Send valid GPS data
   bool accepted = boatData->updateGPS(40.7128, -74.0060, 0.0, 5.0, "GPS-Test");
   assert(accepted == true);

   GPSData gps = boatData->getGPSData();
   double lastValidLat = gps.latitude;
   assert(fabs(lastValidLat - 40.7128) < 0.0001);
   ```

2. **Action**: Send invalid (outlier) data
   ```cpp
   // Attempt to update with invalid latitude (200°N > 90°N max)
   accepted = boatData->updateGPS(200.0, -74.0060, 0.0, 5.0, "GPS-Test");
   ```

3. **Assertions**: Verify rejection and retention
   ```cpp
   // Update should be rejected
   assert(accepted == false);

   // GPS data should retain last valid value
   gps = boatData->getGPSData();
   assert(fabs(gps.latitude - lastValidLat) < 0.0001);  // Unchanged
   assert(gps.available == true);  // Still available

   // Verify logging (check UDP logger output or diagnostic counter)
   DiagnosticData diag = boatData->getDiagnostics();
   // (Rejection should be logged for diagnostics)
   ```

4. **Expected Result**: ✅ Outlier rejected, last valid value retained

**Test File**: `test/integration/test_outlier_rejection.cpp`

---

## Hardware-Specific Test: 200ms Calculation Cycle Timing

**Purpose**: Validate that the calculation cycle completes within 200ms on ESP32 hardware (NFR-001).

### Test Steps

1. **Setup**: Upload firmware with timing instrumentation
   ```cpp
   void loop() {
       app.tick();  // ReactESP event loop

       // Timing measurement for calculation cycle
       if (calculationTriggered) {
           unsigned long startTime = micros();
           calculateDerivedParameters();
           unsigned long duration = micros() - startTime;

           // Log duration
           Serial.printf("Calculation duration: %lu us\n", duration);

           // Assert within 200ms limit
           if (duration > 200000) {  // 200ms = 200,000 microseconds
               Serial.println("ERROR: Calculation overrun!");
           }
       }
   }
   ```

2. **Action**: Run firmware for 5 minutes, monitor serial output

3. **Assertions**:
   - All calculation cycles complete in <200ms
   - No "Calculation overrun!" errors logged
   - Average duration <50ms (typical expected)

4. **Expected Result**: ✅ 200ms deadline met consistently

**Test File**: `test/test_boatdata_timing/test_main.cpp` (hardware test, ESP32 required)

---

## Quickstart Execution Summary

| Scenario | Test Type | Hardware Required | Expected Duration |
|----------|-----------|-------------------|-------------------|
| 1. Single GPS | Unit | No (mocked) | <1 second |
| 2. Multi-Source Priority | Integration | No (mocked) | ~10 seconds |
| 3. Source Failover | Integration | No (mocked) | ~7 seconds |
| 4. Manual Override | Integration | No (mocked) | <1 second |
| 5. Derived Calculation | Integration | No (mocked) | <1 second |
| 6. Calibration Update | Integration | No (mocked filesystem) | <1 second |
| 7. Outlier Rejection | Unit | No (mocked) | <1 second |
| 8. Timing Validation | Hardware | **Yes (ESP32)** | 5 minutes |

**Total Automated Test Time** (Scenarios 1-7): ~20 seconds
**Total Manual Test Time** (Scenario 8): 5 minutes

---

## Running Tests

### Unit Tests (No Hardware)
```bash
pio test -e native -f test_single_gps_source
pio test -e native -f test_outlier_rejection
```

### Integration Tests (No Hardware)
```bash
pio test -e native -f test_multi_source_priority
pio test -e native -f test_source_failover
pio test -e native -f test_derived_calculation
pio test -e native -f test_calibration_update
```

### Hardware Tests (ESP32 Required)
```bash
pio test -e esp32dev_test -f test_boatdata_timing
```

---

**Quickstart Status**: ✅ Complete
**Next Step**: Implement tests (TDD - tests first, then implementation to make them pass)
