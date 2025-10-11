# HAL Interface Contract: ISerialPort

**Feature**: 006-nmea-0183-handlers
**Interface**: ISerialPort
**Purpose**: Abstract Serial2 hardware for NMEA 0183 reception and testing
**Location**: src/hal/interfaces/ISerialPort.h

---

## Interface Definition

```cpp
/**
 * @brief Serial port hardware abstraction interface
 *
 * Abstracts UART hardware (Serial2 on ESP32) to enable:
 * - Native platform testing with mocked serial data
 * - Hardware-independent sentence processing logic
 * - Testable NMEA 0183 parsing without ESP32 hardware
 *
 * Implementations:
 * - ESP32SerialPort: Production wrapper for HardwareSerial (Serial2)
 * - MockSerialPort: Test implementation with pre-defined sentence data
 */
class ISerialPort {
public:
    /**
     * @brief Check number of bytes available to read
     *
     * @return Number of bytes in receive buffer, 0 if empty
     *
     * Contract:
     * - MUST return non-negative integer
     * - MUST return 0 if no data available
     * - MUST NOT block (immediate return)
     */
    virtual int available() = 0;

    /**
     * @brief Read one byte from serial port
     *
     * @return Byte value (0-255) or -1 if no data available
     *
     * Contract:
     * - MUST return -1 if available() returns 0
     * - MUST return byte value [0, 255] if data available
     * - MUST NOT block (immediate return)
     * - MUST consume byte from buffer (subsequent available() decrements)
     */
    virtual int read() = 0;

    /**
     * @brief Initialize serial port hardware
     *
     * @param baud Baud rate (4800 for NMEA 0183)
     *
     * Contract:
     * - MUST configure hardware for specified baud rate
     * - MUST enable receive mode
     * - MAY enable transmit mode (not required for NMEA 0183 RX-only)
     * - MUST be idempotent (safe to call multiple times)
     */
    virtual void begin(unsigned long baud) = 0;

    /**
     * @brief Virtual destructor for polymorphic deletion
     */
    virtual ~ISerialPort() = default;
};
```

---

## Behavioral Contracts

### Contract 1: available() Non-Blocking

**Given**: Any implementation state
**When**: `available()` is called
**Then**:
- Function MUST return immediately (< 1ms)
- Return value MUST be >= 0
- Return value MUST equal number of bytes readable via `read()`

**Test**: test_nmea0183_contracts/test_iserialport.cpp:test_available_non_blocking()

---

### Contract 2: read() Returns -1 When Empty

**Given**: Serial buffer is empty (`available() == 0`)
**When**: `read()` is called
**Then**:
- Function MUST return -1
- `available()` MUST still return 0 (no state change)

**Test**: test_nmea0183_contracts/test_iserialport.cpp:test_read_when_empty()

---

### Contract 3: read() Consumes Bytes

**Given**: Serial buffer has N bytes (`available() == N`)
**When**: `read()` is called successfully (returns >= 0)
**Then**:
- Next call to `available()` MUST return N-1
- Returned byte MUST be in range [0, 255]
- Subsequent `read()` MUST return next byte in sequence

**Test**: test_nmea0183_contracts/test_iserialport.cpp:test_read_consumes_bytes()

---

### Contract 4: begin() Idempotent

**Given**: Implementation instance
**When**: `begin(4800)` is called multiple times
**Then**:
- Each call MUST succeed without error
- Serial port MUST remain functional after all calls
- No memory leaks or resource exhaustion

**Test**: test_nmea0183_contracts/test_iserialport.cpp:test_begin_idempotent()

---

### Contract 5: Byte Sequence Preservation

**Given**: Mock data "ABC" loaded into serial buffer
**When**: `read()` is called 3 times
**Then**:
- First `read()` MUST return 'A' (0x41)
- Second `read()` MUST return 'B' (0x42)
- Third `read()` MUST return 'C' (0x43)
- Fourth `read()` MUST return -1

**Test**: test_nmea0183_contracts/test_iserialport.cpp:test_byte_sequence_preservation()

---

## Implementation Requirements

### ESP32SerialPort (Production)

**File**: src/hal/implementations/ESP32SerialPort.h/cpp

**Constructor**:
```cpp
ESP32SerialPort(HardwareSerial* serial)
```
- **Precondition**: `serial` pointer is non-null (e.g., &Serial2)
- **Postcondition**: Instance ready for `begin()` call

**available() Implementation**:
```cpp
int ESP32SerialPort::available() {
    return serial_->available();
}
```

**read() Implementation**:
```cpp
int ESP32SerialPort::read() {
    return serial_->read();
}
```

**begin() Implementation**:
```cpp
void ESP32SerialPort::begin(unsigned long baud) {
    serial_->begin(baud);
}
```

**Memory**: 8 bytes (HardwareSerial pointer)

---

### MockSerialPort (Testing)

**File**: src/mocks/MockSerialPort.h/cpp

**Constructor**:
```cpp
MockSerialPort()
```
- **Postcondition**: Empty buffer, position = 0

**Test Data Setup**:
```cpp
void setMockData(const char* data)
```
- **Precondition**: `data` is null-terminated string
- **Postcondition**: Buffer contains `data`, position reset to 0, available() returns strlen(data)

**available() Implementation**:
```cpp
int MockSerialPort::available() {
    return (mockData_ != nullptr) ? (strlen(mockData_) - position_) : 0;
}
```

**read() Implementation**:
```cpp
int MockSerialPort::read() {
    if (available() == 0) return -1;
    return mockData_[position_++];
}
```

**begin() Implementation**:
```cpp
void MockSerialPort::begin(unsigned long baud) {
    // No-op for mock (baud rate not used in testing)
}
```

**Memory**: ~16 bytes (pointer + position counter)

---

## Usage Examples

### Production (ESP32)

```cpp
// In main.cpp
#include "hal/implementations/ESP32SerialPort.h"

ISerialPort* serial0183 = new ESP32SerialPort(&Serial2);
serial0183->begin(4800);

// In ReactESP loop
if (serial0183->available() > 0) {
    int byte = serial0183->read();
    // Feed to NMEA parser
}
```

### Testing (Native)

```cpp
// In test_nmea0183_integration/test_gga_to_boatdata.cpp
#include "mocks/MockSerialPort.h"

MockSerialPort mockSerial;
mockSerial.setMockData("$VHGGA,123519,5230.5000,N,00507.0000,E,1,08,0.9,545.4,M,46.9,M,,*47\r\n");

// Process sentence
while (mockSerial.available() > 0) {
    int byte = mockSerial.read();
    nmea0183->HandleByte(byte);
}

// Assert BoatData updated with lat=52.508333, lon=5.116667
```

---

## Contract Test Matrix

| Test Case | ESP32SerialPort | MockSerialPort |
|-----------|-----------------|----------------|
| available() non-blocking | ✓ Hardware test | ✓ Native test |
| read() returns -1 when empty | ✓ Hardware test | ✓ Native test |
| read() consumes bytes | ✓ Hardware test | ✓ Native test |
| begin() idempotent | ✓ Hardware test | ✓ Native test |
| Byte sequence preservation | ✓ Hardware test | ✓ Native test |

**Test Execution**:
```bash
# Contract tests (native platform)
pio test -e native -f test_nmea0183_contracts

# Hardware validation (ESP32 only)
pio test -e esp32dev_test -f test_nmea0183_hardware
```

---

## Performance Requirements

**ESP32SerialPort**:
- `available()`: < 10 μs (direct register read)
- `read()`: < 20 μs (FIFO buffer access)
- `begin()`: < 5 ms (UART initialization)

**MockSerialPort**:
- `available()`: < 1 μs (strlen - position)
- `read()`: < 1 μs (array access + increment)
- `begin()`: < 1 μs (no-op)

**Justification**: Serial I/O must not block 10ms ReactESP polling loop (Constitutional Principle VI)

---

## Error Handling

**No Exceptions**: Interface uses return codes only
- `available()`: Always succeeds (returns 0 if error)
- `read()`: Returns -1 on error or no data
- `begin()`: Always succeeds (silent failure acceptable for mock)

**Rationale**: Arduino/ESP32 environment typically built without exception support

---

## Thread Safety

**Not Thread-Safe**: Interface assumes single-threaded ReactESP event loop
- **Caller Responsibility**: Do not call from multiple tasks/threads
- **ESP32**: Safe if only called from main loop or single ReactESP task
- **Mock**: Safe in single-threaded test framework (Unity)

---

## Version History

- **v1.0.0** (2025-10-11): Initial contract definition for NMEA 0183 handlers feature

---

**Contract Status**: Ready for implementation and contract test generation
