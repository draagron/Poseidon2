# BoatData Calculation Cycle Timing Test

## Overview
This hardware test validates the constitutional requirement (NFR-001) that the calculation cycle completes within 200ms on ESP32 hardware.

## Test Configuration
- **Test Duration**: 5 minutes
- **Calculation Frequency**: 5 Hz (200ms interval)
- **Expected Cycles**: ~1,500
- **Hardware Required**: ESP32 (SH-ESP32 or compatible)

## Success Criteria
1. **No overruns**: All calculation cycles complete in <200ms
2. **Average performance**: Average duration <50ms (typical expected)
3. **Worst-case performance**: Maximum duration <200ms

## Running the Test

### Prerequisites
- ESP32 board connected via USB
- PlatformIO installed
- Serial port available (e.g., `/dev/ttyUSB0`)

### Upload and Monitor
```bash
# Upload firmware and start monitoring
pio test -e esp32dev_test -f test_boatdata_timing --upload-port /dev/ttyUSB0

# Or use auto-detection
pio test -e esp32dev_test -f test_boatdata_timing
```

### Test Execution
1. Firmware uploads to ESP32
2. Test runs for 5 minutes, printing progress every 30 seconds
3. After 5 minutes, test statistics are displayed
4. Unity test assertions validate success criteria
5. Final result: "TEST PASSED" or "TEST FAILED"

## Expected Output

### During Test (every 30 seconds)
```
[00:30] Cycle 150: 12345 us (avg: 12000 us, max: 15000 us, overruns: 0)
[01:00] Cycle 300: 11876 us (avg: 11950 us, max: 15000 us, overruns: 0)
[01:30] Cycle 450: 12100 us (avg: 12010 us, max: 15500 us, overruns: 0)
...
```

### Final Statistics
```
========================================
Calculation Cycle Timing Test Results
========================================
Test Duration: 300000 ms (5.0 minutes)
Total Cycles: 1500
Average Duration: 12345 us (12.35 ms)
Min Duration: 8000 us (8.00 ms)
Max Duration: 18000 us (18.00 ms)
Overruns (>200ms): 0 (0.00%)

========================================
✓ PASS: No calculation overruns detected
✓ PASS: Average duration (12345 us) < 50ms
✓ PASS: Max duration (18000 us) < 200ms
========================================

*** TEST PASSED ***
```

## Troubleshooting

### Test Fails: Overruns Detected
**Symptom**: `✗ FAIL: Calculation overruns detected`

**Causes**:
1. ESP32 CPU frequency too low (should be 240 MHz)
2. Excessive interrupt activity (WiFi, Bluetooth)
3. Calculation workload too heavy

**Solutions**:
1. Verify `platformio.ini` sets `board_build.f_cpu = 240000000L`
2. Disable unnecessary services (WiFi, Bluetooth) during test
3. Optimize calculation formulas (check for inefficient math)

### Test Fails: Average Duration Too High
**Symptom**: `✗ FAIL: Average duration >= 50ms`

**Causes**:
1. CPU running at reduced frequency
2. Debug mode enabled (optimization off)
3. Excessive logging or serial output

**Solutions**:
1. Compile in release mode: `pio test -e esp32dev_test -f test_boatdata_timing`
2. Reduce serial output frequency
3. Profile code to identify hot spots

### Test Hangs or Doesn't Start
**Symptom**: No serial output after upload

**Causes**:
1. Serial monitor not connected
2. Wrong serial port
3. Baud rate mismatch

**Solutions**:
1. Check serial connection: `pio device list`
2. Verify baud rate: 115200 (default)
3. Reset ESP32 after upload

## Test Architecture

### Timing Measurement
The test measures calculation cycle duration using `micros()` timestamps:
```cpp
unsigned long startMicros = micros();
// ... perform calculations ...
unsigned long durationMicros = micros() - startMicros;
```

### Calculation Simulation
The test simulates the computational workload of the CalculationEngine by:
1. Reading all sensor data from BoatData
2. Performing trigonometric calculations (sin, cos, tan, sqrt)
3. Iterating over calculation loops

This approximates the actual calculation workload without requiring full implementation.

### Statistics Tracking
The test tracks:
- **Total cycles**: Count of completed calculation cycles
- **Total duration**: Sum of all cycle durations
- **Average duration**: Mean calculation time
- **Min/Max duration**: Range of calculation times
- **Overrun count**: Number of cycles exceeding 200ms

## Integration with CI/CD

**Note**: This test requires physical ESP32 hardware and cannot run in CI/CD pipelines. It is intended for manual validation during development and pre-release testing.

For automated testing, use the unit and integration tests that run on the `native` platform:
```bash
pio test -e native
```

## References
- **Specification**: `specs/003-boatdata-feature-as/spec.md` (NFR-001)
- **Quickstart**: `specs/003-boatdata-feature-as/quickstart.md` (lines 385-423)
- **Tasks**: `specs/003-boatdata-feature-as/tasks.md` (T041)
- **Constitution**: `.specify/memory/constitution.md` (Principle II: Resource Management)

---
**Last Updated**: 2025-10-07
