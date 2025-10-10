# Quickstart Validation Guide: Enhanced BoatData

**Feature**: Enhanced BoatData (R005)
**Date**: 2025-10-10
**Purpose**: Step-by-step validation of enhanced data structures and sensor integration

## Prerequisites

- ESP32 hardware with CAN bus (NMEA2000) and Serial2 (NMEA0183) connected
- Optional: 1-wire sensors on GPIO 4 (can simulate with mocks)
- PlatformIO environment configured
- WebSocket logger accessible at `ws://<device-ip>/logs`

## Test Scenarios

### Scenario 1: GPS Variation Field (FR-001, FR-009)

**Objective**: Verify magnetic variation moved from CompassData to GPSData

**Steps**:
1. Build and upload firmware:
   ```bash
   pio run --target upload
   ```

2. Connect to WebSocket logger:
   ```bash
   wscat -c ws://192.168.1.100/logs
   ```

3. Send NMEA0183 RMC sentence with variation via Serial2:
   ```
   $GPRMC,123519,A,4807.038,N,01131.000,E,022.4,084.4,230394,003.1,W*6A
   ```
   (Variation: 3.1° West = -0.054 radians)

4. Check WebSocket log for:
   ```json
   {"level":"DEBUG","message":"GPS variation updated: -0.054 rad"}
   ```

5. Verify data structure:
   ```cpp
   ASSERT_NEAR(boatData.gps.variation, -0.054, 0.001);
   ASSERT_TRUE(boatData.gps.available);
   ```

**Expected Result**: GPSData.variation populated, CompassData.variation removed (compilation error if accessed)

**Pass Criteria**: ✅ Variation stored in GPSData, old CompassData.variation not accessible

---

### Scenario 2: Compass Rate of Turn (FR-005)

**Objective**: Verify rateOfTurn field in CompassData from NMEA2000 PGN 127251

**Steps**:
1. Send NMEA2000 PGN 127251 (Rate of Turn) via CAN bus:
   ```cpp
   tN2kMsg N2kMsg;
   SetN2kPGN127251(N2kMsg, 0, 0.1);  // SID=0, rate=0.1 rad/s (turning right)
   nmea2000->SendMsg(N2kMsg);
   ```

2. Check WebSocket log:
   ```json
   {"level":"DEBUG","message":"Rate of turn updated: 0.1 rad/s"}
   ```

3. Verify data structure:
   ```cpp
   ASSERT_NEAR(boatData.compass.rateOfTurn, 0.1, 0.01);
   ASSERT_TRUE(boatData.compass.available);
   ```

**Expected Result**: CompassData.rateOfTurn populated with positive value (turning starboard)

**Pass Criteria**: ✅ Rate of turn stored correctly, sign convention verified (positive = starboard turn)

---

### Scenario 3: Heel/Pitch/Heave from PGN 127257 (FR-006, FR-007, FR-008)

**Objective**: Verify attitude data moved to CompassData, heelAngle removed from SpeedData

**Steps**:
1. Send NMEA2000 PGN 127257 (Attitude):
   ```cpp
   tN2kMsg N2kMsg;
   SetN2kPGN127257(N2kMsg, 0, 0.0, 0.15, 0.05);  // SID=0, yaw=0, pitch=0.15rad, roll=0.05rad
   nmea2000->SendMsg(N2kMsg);
   ```
   (Pitch: 0.15 rad = ~8.6°, Roll/Heel: 0.05 rad = ~2.9° starboard)

2. Check WebSocket log:
   ```json
   {"level":"DEBUG","message":"Compass attitude updated: heel=0.05, pitch=0.15, heave=0.0"}
   ```

3. Verify data structure:
   ```cpp
   ASSERT_NEAR(boatData.compass.heelAngle, 0.05, 0.01);
   ASSERT_NEAR(boatData.compass.pitchAngle, 0.15, 0.01);
   ASSERT_TRUE(boatData.compass.available);
   ```

4. Verify old SpeedData.heelAngle is gone (should not compile):
   ```cpp
   // This should cause compilation error:
   // double heel = boatData.speed.heelAngle;  // ERROR: no member 'heelAngle' in DSTData
   ```

**Expected Result**: Heel/pitch/heave in CompassData, old SpeedData.heelAngle inaccessible

**Pass Criteria**: ✅ Attitude data in CompassData, SpeedData migration complete

---

### Scenario 4: DSTData Rename and Extension (FR-002, FR-010, FR-011, FR-012)

**Objective**: Verify SpeedData renamed to DSTData with depth and seaTemperature added

**Steps**:
1. Send NMEA2000 PGN 128267 (Water Depth):
   ```cpp
   tN2kMsg N2kMsg;
   SetN2kPGN128267(N2kMsg, 0, 12.5, 0.3);  // SID=0, depth=12.5m, offset=0.3m
   nmea2000->SendMsg(N2kMsg);
   ```

2. Send NMEA2000 PGN 128259 (Speed):
   ```cpp
   SetN2kPGN128259(N2kMsg, 0, 2.5);  // SID=0, speed=2.5 m/s
   nmea2000->SendMsg(N2kMsg);
   ```

3. Send NMEA2000 PGN 130316 (Temperature):
   ```cpp
   SetN2kPGN130316(N2kMsg, 0, 0, N2kts_SeaTemperature, 288.15);  // SID=0, temp=288.15K (15°C)
   nmea2000->SendMsg(N2kMsg);
   ```

4. Check WebSocket log:
   ```json
   {"level":"DEBUG","message":"DST data updated: depth=12.5m, speed=2.5m/s, temp=15.0C"}
   ```

5. Verify data structure:
   ```cpp
   ASSERT_NEAR(boatData.dst.depth, 12.5, 0.1);
   ASSERT_NEAR(boatData.dst.measuredBoatSpeed, 2.5, 0.1);
   ASSERT_NEAR(boatData.dst.seaTemperature, 15.0, 0.5);
   ASSERT_TRUE(boatData.dst.available);
   ```

**Expected Result**: DSTData structure populated, Kelvin→Celsius conversion applied

**Pass Criteria**: ✅ DSTData accessible, temperature converted correctly, backward compatibility typedef works

---

### Scenario 5: Engine Data from PGN 127488/127489 (FR-013 to FR-016)

**Objective**: Verify EngineData structure captures engine telemetry

**Steps**:
1. Send NMEA2000 PGN 127488 (Engine Parameters, Rapid):
   ```cpp
   tN2kMsg N2kMsg;
   SetN2kPGN127488(N2kMsg, 0, 1800);  // EngineInstance=0, RPM=1800
   nmea2000->SendMsg(N2kMsg);
   ```

2. Send NMEA2000 PGN 127489 (Engine Parameters, Dynamic):
   ```cpp
   SetN2kPGN127489(N2kMsg, 0, 80.0, 14.2);  // EngineInstance=0, oilTemp=80C, alternatorVoltage=14.2V
   nmea2000->SendMsg(N2kMsg);
   ```

3. Check WebSocket log:
   ```json
   {"level":"DEBUG","message":"Engine data updated: RPM=1800, oilTemp=80.0C, altVolt=14.2V"}
   ```

4. Verify data structure:
   ```cpp
   ASSERT_NEAR(boatData.engine.engineRev, 1800, 10);
   ASSERT_NEAR(boatData.engine.oilTemperature, 80.0, 1.0);
   ASSERT_NEAR(boatData.engine.alternatorVoltage, 14.2, 0.1);
   ASSERT_TRUE(boatData.engine.available);
   ```

**Expected Result**: EngineData structure populated with RPM, temperature, voltage

**Pass Criteria**: ✅ Engine telemetry captured, validation applied (RPM [0, 6000])

---

### Scenario 6: Saildrive Status from 1-Wire (FR-017, FR-018)

**Objective**: Verify SaildriveData structure captures engagement status

**Steps**:
1. Simulate 1-wire digital sensor read (or use real sensor):
   ```cpp
   // In test: Use MockOneWireSensors
   MockOneWireSensors sensors;
   sensors.setSaildriveEngaged(true);
   ```

2. Trigger saildrive polling:
   ```cpp
   pollSaildriveStatus();  // ReactESP event loop function
   ```

3. Check WebSocket log:
   ```json
   {"level":"DEBUG","message":"Saildrive engaged: true"}
   ```

4. Verify data structure:
   ```cpp
   ASSERT_TRUE(boatData.saildrive.saildriveEngaged);
   ASSERT_TRUE(boatData.saildrive.available);
   ```

**Expected Result**: SaildriveData.saildriveEngaged reflects sensor state

**Pass Criteria**: ✅ 1-wire sensor read, availability flag set correctly

---

### Scenario 7: Battery Monitoring from 1-Wire (FR-019 to FR-025)

**Objective**: Verify BatteryData structure captures dual battery banks

**Steps**:
1. Simulate 1-wire analog sensor reads:
   ```cpp
   MockOneWireSensors sensors;
   BatteryMonitorData battA = {12.6, 5.2, 85.0, false, true, true};  // V, A (charging), SOC%, shore=off, engine=on
   sensors.setBatteryA(battA);
   ```

2. Trigger battery polling:
   ```cpp
   pollBatteryStatus();
   ```

3. Check WebSocket log:
   ```json
   {"level":"DEBUG","message":"Battery A: 12.6V, 5.2A (charging), SOC=85%"}
   ```

4. Verify data structure:
   ```cpp
   ASSERT_NEAR(boatData.battery.voltageA, 12.6, 0.1);
   ASSERT_NEAR(boatData.battery.amperageA, 5.2, 0.1);  // Positive = charging
   ASSERT_NEAR(boatData.battery.stateOfChargeA, 85.0, 1.0);
   ASSERT_TRUE(boatData.battery.engineChargerOnA);
   ASSERT_FALSE(boatData.battery.shoreChargerOnA);
   ASSERT_TRUE(boatData.battery.available);
   ```

5. Verify sign convention (discharging):
   ```cpp
   BatteryMonitorData battDischarging = {12.2, -15.3, 65.0, false, false, true};
   sensors.setBatteryA(battDischarging);
   pollBatteryStatus();
   ASSERT_LT(boatData.battery.amperageA, 0);  // Negative = discharging
   ```

**Expected Result**: BatteryData captures voltage, current (signed), SOC, charger status

**Pass Criteria**: ✅ Dual battery monitoring, sign convention correct (+ = charge, - = discharge)

---

### Scenario 8: Shore Power Monitoring from 1-Wire (FR-026 to FR-028)

**Objective**: Verify ShorePowerData structure captures connection and power draw

**Steps**:
1. Simulate shore power connected:
   ```cpp
   MockOneWireSensors sensors;
   ShorePowerData shoreData = {true, 1200.0, true};  // Connected, 1200W draw
   sensors.setShorePower(shoreData);
   ```

2. Trigger shore power polling:
   ```cpp
   pollShorePowerStatus();
   ```

3. Check WebSocket log:
   ```json
   {"level":"DEBUG","message":"Shore power: ON, 1200W"}
   ```

4. Verify data structure:
   ```cpp
   ASSERT_TRUE(boatData.shorePower.shorePowerOn);
   ASSERT_NEAR(boatData.shorePower.power, 1200.0, 10.0);
   ASSERT_TRUE(boatData.shorePower.available);
   ```

**Expected Result**: ShorePowerData reflects connection status and power consumption

**Pass Criteria**: ✅ Shore power monitoring functional, validation applied ([0, 5000]W)

---

### Scenario 9: Validation and Clamping (FR-033, FR-034)

**Objective**: Verify out-of-range values are clamped and logged

**Steps**:
1. Send invalid pitch angle (90° = π/2, exceeds ±30° limit):
   ```cpp
   tN2kMsg N2kMsg;
   SetN2kPGN127257(N2kMsg, 0, 0.0, M_PI/2, 0.0);  // pitch=90° (invalid)
   nmea2000->SendMsg(N2kMsg);
   ```

2. Check WebSocket log for warning:
   ```json
   {"level":"WARN","message":"Pitch angle out of range: 1.571 rad, clamped to 0.524 rad"}
   ```

3. Verify clamping:
   ```cpp
   ASSERT_LE(boatData.compass.pitchAngle, M_PI/6);  // Clamped to 30°
   ASSERT_FALSE(boatData.compass.available);  // Marked invalid
   ```

4. Send negative depth (sensor above water):
   ```cpp
   SetN2kPGN128267(N2kMsg, 0, -2.5, 0.0);  // Invalid depth
   nmea2000->SendMsg(N2kMsg);
   ```

5. Verify rejection:
   ```cpp
   ASSERT_EQ(boatData.dst.depth, 0.0);  // Set to zero
   ASSERT_FALSE(boatData.dst.available);  // Marked invalid
   ```

**Expected Result**: Invalid values clamped/rejected, availability flags set false, warnings logged

**Pass Criteria**: ✅ Validation active, out-of-range values handled gracefully

---

### Scenario 10: Memory Footprint Validation

**Objective**: Verify total memory footprint within Constitutional limits

**Steps**:
1. Build firmware and check output:
   ```bash
   pio run | grep "RAM:"
   ```

2. Verify BoatDataStructure size:
   ```cpp
   size_t structSize = sizeof(BoatDataStructure);
   Serial.printf("BoatDataStructure size: %d bytes\n", structSize);
   ASSERT_LE(structSize, 600);  // Target: ~560 bytes
   ```

3. Check stack usage per task:
   ```cpp
   UBaseType_t watermark = uxTaskGetStackHighWaterMark(NULL);
   Serial.printf("Stack high water mark: %d bytes\n", watermark * 4);
   ASSERT_GE(watermark * 4, 1024);  // At least 1KB headroom
   ```

**Expected Result**: Total structure ≤600 bytes, stack usage within 8KB limit

**Pass Criteria**: ✅ Memory footprint acceptable (<0.2% RAM increase from v1.0.0)

---

## Acceptance Criteria Summary

| Scenario | Requirement | Status |
|----------|------------|--------|
| 1 | GPS variation in GPSData (FR-001, FR-009) | ⬜ Not Tested |
| 2 | Compass rate of turn (FR-005) | ⬜ Not Tested |
| 3 | Heel/pitch/heave in CompassData (FR-006-008) | ⬜ Not Tested |
| 4 | DSTData structure (FR-002, FR-010-012) | ⬜ Not Tested |
| 5 | Engine telemetry (FR-013-016) | ⬜ Not Tested |
| 6 | Saildrive status (FR-017-018) | ⬜ Not Tested |
| 7 | Battery monitoring (FR-019-025) | ⬜ Not Tested |
| 8 | Shore power monitoring (FR-026-028) | ⬜ Not Tested |
| 9 | Validation and clamping (FR-033-034) | ⬜ Not Tested |
| 10 | Memory footprint | ⬜ Not Tested |

**Feature Complete When**: All scenarios pass ✅

---

## Troubleshooting

### Common Issues

1. **Compilation errors about missing fields**:
   - Check BoatDataTypes.h updated to v2.0.0
   - Verify SpeedData → DSTData migration complete

2. **1-wire sensors return `available=false`**:
   - Check GPIO 4 bus initialization in main.cpp
   - Verify sensor device addresses match configuration
   - Check pullup resistor on 1-wire bus (4.7kΩ required)

3. **NMEA2000 PGNs not received**:
   - Verify CAN bus termination (120Ω at both ends)
   - Check `nmea2000->ParseMessages()` called in ReactESP loop
   - Monitor CAN bus with `candump can0` on Linux

4. **WebSocket logger not responding**:
   - Verify WiFi connected: Check serial output for IP address
   - Test WebSocket endpoint: `wscat -c ws://<ip>/logs`
   - Check ESPAsyncWebServer running in main.cpp

---

## Next Steps After Validation

1. Run full integration test suite:
   ```bash
   pio test -e native -f test_boatdata_*
   ```

2. Hardware validation tests (ESP32 required):
   ```bash
   pio test -e esp32dev -f test_boatdata_hardware
   ```

3. Performance profiling:
   - Measure ReactESP event loop latency
   - Verify no calculation overruns (DiagnosticData.calculationOverruns == 0)

4. Create pull request with validation results and screenshots
