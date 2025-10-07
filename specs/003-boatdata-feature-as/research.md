# Research: BoatData Implementation Decisions

**Feature**: BoatData - Centralized Marine Data Model
**Date**: 2025-10-06
**Status**: Complete

## Overview
This document captures technical research and decisions for implementing the BoatData centralized data model. All critical clarifications were resolved in the spec clarification session (Session 2025-10-06). This research focuses on implementation details deferred to the planning phase.

---

## Decision 1: Concurrency Model

### Question
How should concurrent sensor updates from multiple protocols (NMEA0183, NMEA2000, 1-Wire) be synchronized with the 200ms calculation cycle?

### Options Considered

**Option A: ReactESP Single-Threaded Event Loop** (RECOMMENDED)
- All sensor updates and calculations run in main loop via ReactESP callbacks
- No concurrent access to BoatData structure
- NMEA handlers register callbacks that update BoatData in main thread
- Calculation engine runs as 200ms periodic callback

**Option B: RTOS Tasks with Mutex Protection**
- Separate FreeRTOS tasks for NMEA0183, NMEA2000, 1-Wire, calculations
- BoatData protected by mutex/semaphore
- Higher overhead, risk of priority inversion

**Option C: Interrupt-Driven with Critical Sections**
- Sensor data updated in ISRs
- Calculation engine runs in main loop
- Data protected by critical sections (disable interrupts)
- Risk of long interrupt latency

### Decision: Option A - ReactESP Single-Threaded Event Loop

**Rationale**:
1. **Constitutional Requirement**: Project constitution mandates ReactESP for asynchronous programming (Principle VI, Hardware Initialization Sequence)
2. **Simplicity**: No mutex/semaphore overhead, no deadlock risk
3. **Predictable Timing**: 200ms calculation cycle guaranteed by ReactESP onRepeat()
4. **Existing Pattern**: WiFi management foundation already uses this model successfully
5. **Testability**: Pure function calculations easy to unit test

**Implementation Strategy**:
- NMEA message handlers call BoatData update methods directly (same thread)
- ReactESP `app.onRepeat(200, calculateDerivedParameters)` for calculation cycle
- No blocking operations in callbacks (constitutional requirement: always-on)
- Sensor update timestamps tracked for stale detection (5-second threshold)

**Trade-offs**:
- Sensor updates must be fast (<10ms) to avoid delaying calculations
- No true parallel processing (acceptable: ESP32 single-core sufficient)
- Calculation overruns handled by skipping cycle (clarification decision)

**Alternatives Rejected**:
- **Option B**: Adds complexity without benefit; constitution mandates ReactESP
- **Option C**: Interrupt latency risk unacceptable for 200ms deadline

---

## Decision 2: Mathematical Formula Validation

### Question
Verify calculation formulas from `examples/Calculations/calc.cpp` for correctness, especially angle wraparound and singularity handling.

### Research Findings

**Source**: `examples/Calculations/calc.cpp` (lines 82-194)
**Reference**: [sailboatinstruments.blogspot.com](http://sailboatinstruments.blogspot.com) (blog posts cited in comments)

### Formulas Validated

**1. AWA Offset Correction** (lines 83-87):
```cpp
awa_offset = awa_measured + offset;
if (awa_offset > 180.0) awa_offset -= 360.0;
else if (awa_offset < -180.0) awa_offset += 360.0;
```
✅ **Correct**: Handles wraparound for angles in range [-180, 180]

**2. AWA Heel Correction** (lines 90-108):
```cpp
double tan_awa = tan(awa_offset * DEG_TO_RAD);
if (isnan(tan_awa))
   awa_heel = awa_offset;  // Singularity: wind directly ahead/astern
else {
   double cos_heel = cos(heel * DEG_TO_RAD);
   awa_heel = atan(tan_awa / cos_heel) * RAD_TO_DEG;
   // Quadrant correction
   if (awa_offset >= 0.0) {
      if (awa_offset > 90.0) awa_heel += 180.0;
   } else {
      if (awa_offset < -90.0) awa_heel -= 180.0;
   }
}
```
✅ **Correct**: Handles singularity (tan(±90°) → ±∞), quadrant ambiguity resolved

**3. Leeway Calculation** (lines 111-123):
```cpp
if (meas_boat_speed == 0.0
 || (awa_heel > 0.0 && heel > 0.0)
 || (awa_heel < 0.0 && heel < 0.0))
    leeway = 0.0;  // No leeway: stopped or wind/heel same side
else {
   leeway = K * heel / (meas_boat_speed * meas_boat_speed);
   // Limit for very low speeds
   if (leeway > 45.0) leeway = 45.0;
   else if (leeway < -45.0) leeway = -45.0;
}
```
✅ **Correct**: Prevents divide-by-zero, physics-based limits (±45° max)

**4. True Wind Speed (TWS)** (lines 132-137):
```cpp
double cartesian_awa = (270.0 - awa_heel) * DEG_TO_RAD;
double aws_x = aws * cos(cartesian_awa);
double aws_y = aws * sin(cartesian_awa);
double tws_x = aws_x + lateral_speed;  // lateral_speed = stw * sin(leeway)
double tws_y = aws_y + meas_boat_speed;
tws = sqrt(tws_x * tws_x + tws_y * tws_y);
```
✅ **Correct**: Vector addition in Cartesian coordinates, boat motion compensated

**5. True Wind Angle (TWA)** (lines 140-159):
```cpp
double twa_cartesian = atan2(tws_y, tws_x);
if (isnan(twa_cartesian)) {  // Singularity: zero wind
   if (tws_y < 0.0) twa = 180.0;
   else twa = 0.0;
} else {
   twa = 270.0 - twa_cartesian * RAD_TO_DEG;
   // Normalize to [-180, 180]
   if (awa_heel >= 0.0) twa = fmod(twa, 360.0);
   else twa -= 360.0;
   if (twa > 180.0) twa -= 360.0;
   else if (twa < -180.0) twa += 360.0;
}
```
✅ **Correct**: atan2 handles all quadrants, singularity handled, angle normalized

**6. VMG (Velocity Made Good)** (line 162):
```cpp
vmg = stw * cos((-twa + leeway) * DEG_TO_RAD);
```
✅ **Correct**: Component of STW in wind direction

**7. Wind Direction (WDIR)** (lines 165-169):
```cpp
wdir = heading + twa;
if (wdir > 360.0) wdir -= 360.0;
else if (wdir < 0.0) wdir += 360.0;
```
✅ **Correct**: Converts boat-relative TWA to absolute magnetic direction

**8. Current Speed/Direction (SOC/DOC)** (lines 172-194):
```cpp
double cog_mag = cog + variation;
double alpha = (90.0 - (heading + leeway)) * DEG_TO_RAD;
double gamma = (90.0 - cog_mag) * DEG_TO_RAD;
double curr_x = sog * cos(gamma) - stw * cos(alpha);
double curr_y = sog * sin(gamma) - stw * sin(alpha);
soc = sqrt(curr_x * curr_x + curr_y * curr_y);

double doc_cartesian = atan2(curr_y, curr_x);
if (isnan(doc_cartesian)) {
   if (curr_y < 0.0) doc = 180.0;
   else doc = 0.0;
} else {
   doc = 90.0 - doc_cartesian * RAD_TO_DEG;
   if (doc > 360.0) doc -= 360.0;
   else if (doc < 0.0) doc += 360.0;
}
```
✅ **Correct**: Vector subtraction (GPS velocity - water velocity), singularity handled

### Decision: Adopt Validated Formulas from calc.cpp

**Implementation Notes**:
1. **Convert to radians internally**: Store all angles in radians (FR-041), formulas use radians throughout
2. **Preserve singularity handling**: Keep isnan() checks for divide-by-zero cases
3. **Preserve wraparound normalization**: Critical for angle calculations
4. **Add unit tests**: Test each formula with known inputs/outputs
5. **Add edge case tests**: Zero wind, zero speed, extreme heel angles, wraparound boundaries

**Reference Blog Posts** (for future validation):
- AWA calibration: `sailboatinstruments.blogspot.com/2011/10/new-wind-vane-calibration.html`
- Masthead offset: `sailboatinstruments.blogspot.com/2011/02/corrections-to-apparent-wind-angle.html`
- Leeway calibration: `sailboatinstruments.blogspot.com/2011/02/leeway-calibration.html`
- Speed calibration: `sailboatinstruments.blogspot.com/2011/01/boat-and-wind-speed-calibration.html`

---

## Decision 3: Outlier Detection Algorithm

### Question
Define quantitative threshold for "significantly deviate" in FR-016 (outlier detection).

### Research Context
**Marine Sensor Characteristics**:
- GPS: Typically ±5m horizontal accuracy (SA off), occasional multipath spikes
- Compass: ±2° accuracy, magnetic interference can cause ±20° spikes
- Wind speed: ±5% accuracy, gusts can cause rapid changes (valid, not outliers)
- Boat speed: ±3% accuracy, wave slap can cause ±10% spikes

**Challenge**: Distinguish outliers from rapid but valid changes (e.g., tacking, gusts)

### Options Considered

**Option A: Fixed Threshold (Simple)**
- GPS: Reject if >100m from last valid position (at typical boat speeds)
- Compass: Reject if >30° from last valid heading
- Wind/Speed: Reject if >50% change from last valid reading
- Pros: Simple, predictable, no state required
- Cons: May reject valid rapid changes

**Option B: Moving Average Filter**
- Track last N readings, reject if outside ±3σ (3 standard deviations)
- Pros: Adapts to sensor noise characteristics
- Cons: Requires history buffer, higher memory usage

**Option C: Rate-of-Change Limit**
- Reject if change exceeds physically possible rate
  - GPS: >50 knots speed implies bad reading
  - Compass: >180°/sec implies bad reading (max turn rate)
- Pros: Physics-based, catches impossible changes
- Cons: May allow cumulative drift

**Option D: Hybrid: Range Check + Rate-of-Change**
- Step 1: Range check (e.g., lat [-90, 90], speed ≥ 0)
- Step 2: Rate-of-change check (max Δ per update interval)
- Pros: Catches both impossible values and impossible changes
- Cons: More complex logic

### Decision: Option D - Hybrid Range Check + Rate-of-Change

**Rationale**:
1. **Range checks** (FR-014): Catch sensor errors (e.g., lat=200°N)
2. **Rate-of-change checks** (FR-016): Catch noise spikes while allowing valid rapid changes
3. **Constitutional compliance**: Static allocation (no history buffer required)
4. **Marine domain expertise**: Physics-based limits reflect real boat dynamics

**Implementation Thresholds**:

| Data Type | Range Check | Rate-of-Change Limit (per second) |
|-----------|-------------|-----------------------------------|
| Latitude | [-90°, 90°] | 0.1° (~6nm @ equator, 360 knots max) |
| Longitude | [-180°, 180°] | 0.1° (~6nm @ mid-lat) |
| COG | [0°, 360°] | 180° (max turn rate: half circle/sec) |
| SOG | [0, 100 knots] | 10 knots/sec (max acceleration) |
| Heading | [0°, 360°] | 180° (as COG) |
| Variation | [-180°, 180°] | (static, no rate check) |
| AWA | [-180°, 180°] | 360° (tacking can flip instantly) |
| AWS | [0, 100 knots] | 30 knots/sec (gusts valid) |
| Heel | [-90°, 90°] | 45°/sec (max roll rate) |
| Boat Speed | [0, 50 knots] | 5 knots/sec (max acceleration) |
| Rudder Angle | [-90°, 90°] | 60°/sec (max helm rate) |

**Edge Cases**:
- **First reading**: No previous value → skip rate-of-change check, only range check
- **Stale previous reading** (>5s old): Treat as "first reading" (no rate check)
- **Wraparound**: Heading change 359° → 1° is 2°, not 358° (use angle subtraction utility)

**Logging**:
- Range violations: ERROR level (sensor malfunction)
- Rate violations: WARN level (noise spike, may self-correct)
- Include rejected value in log for diagnosis

**Alternatives Rejected**:
- **Option A**: Too simplistic, may reject valid maneuvers
- **Option B**: Violates static allocation principle (history buffer)
- **Option C**: Misses range errors (e.g., lat=200° but slow change)

---

## Implementation Checklist

Based on research decisions:

- [ ] Implement ReactESP event loop integration (200ms onRepeat)
- [ ] Port calculation formulas from calc.cpp to CalculationEngine
- [ ] Add angle wraparound utility functions (normalize to [-π, π] and [0, 2π])
- [ ] Implement DataValidator with range checks (FR-014)
- [ ] Implement DataValidator with rate-of-change checks (FR-016)
- [ ] Add unit tests for all calculation formulas (known inputs → expected outputs)
- [ ] Add unit tests for edge cases (singularities, wraparound, zero values)
- [ ] Add unit tests for outlier detection (boundary conditions)
- [ ] Document sensor update callback signature for NMEA handlers
- [ ] Document concurrency model in CLAUDE.md

---

## References

1. **Constitutional Requirements**:
   - `.specify/memory/constitution.md` (v1.1.0)
   - ReactESP library: https://github.com/mairas/ReactESP
   - Principle VI: Always-On Operation

2. **Calculation Formulas**:
   - `examples/Calculations/calc.cpp` (lines 82-194)
   - Sailboat Instruments Blog: http://sailboatinstruments.blogspot.com

3. **Marine Sensor Specifications**:
   - GPS accuracy: NMEA0183/NMEA2000 standards
   - Compass accuracy: Marine gyrocompass specifications
   - Wind sensor accuracy: Typical masthead unit specs

4. **Existing Codebase Patterns**:
   - `src/main.cpp` (WiFi foundation): ReactESP event loop example (lines 247-256)
   - `src/components/WiFiManager.cpp`: Timeout management with ReactESP
   - `src/utils/WebSocketLogger.h`: Logging patterns

---

**Research Status**: ✅ Complete
**Next Phase**: Phase 1 - Design & Contracts (data-model.md, contracts/, quickstart.md)
