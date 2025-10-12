# Contract: RegisterN2kHandlers() Function

**Version**: 1.0.0
**Date**: 2025-10-12
**Status**: Draft

## Overview

This contract defines the interface and behavior requirements for the `RegisterN2kHandlers()` function, which registers all NMEA2000 PGN handlers with the NMEA2000 library and attaches the message router to the NMEA2000 instance.

## Function Signature

```cpp
/**
 * @brief Register all PGN handlers with NMEA2000 library
 *
 * 1. Extends receive message list with all handled PGNs
 * 2. Creates N2kBoatDataHandler instance
 * 3. Attaches handler to NMEA2000 library
 * 4. Logs INFO message with PGN count and list
 *
 * @param nmea2000 NMEA2000 instance (must not be nullptr)
 * @param boatData BoatData instance for handlers (must not be nullptr)
 * @param logger WebSocket logger (must not be nullptr)
 *
 * @pre nmea2000->Open() has been called (CAN bus initialized)
 * @post All 13 PGNs added to receive message list
 * @post N2kBoatDataHandler attached to NMEA2000 library
 * @post INFO log message broadcasted with handler registration details
 */
void RegisterN2kHandlers(tNMEA2000* nmea2000, BoatData* boatData, WebSocketLogger* logger);
```

## Parameters

### Input Parameters

- **nmea2000**: `tNMEA2000*` (non-null pointer)
  - NMEA2000 library instance (tNMEA2000_esp32 or subclass)
  - **Precondition**: Must not be nullptr
  - **Precondition**: `nmea2000->Open()` must have been called
  - **Behavior**: If nullptr, function returns immediately without action

- **boatData**: `BoatData*` (non-null pointer)
  - BoatData instance for handler callbacks
  - **Precondition**: Must not be nullptr
  - **Behavior**: If nullptr, function returns immediately without action

- **logger**: `WebSocketLogger*` (non-null pointer)
  - WebSocket logger for handler callbacks and registration logging
  - **Precondition**: Must not be nullptr
  - **Behavior**: If nullptr, function returns immediately without action

## Behavior Requirements

### 1. Define PGN List

```cpp
const unsigned long PGNList[] = {
    // GPS (4 PGNs)
    129025,  // Position, Rapid Update
    129026,  // COG & SOG, Rapid Update
    129029,  // GNSS Position Data
    127258,  // Magnetic Variation

    // Compass (4 PGNs)
    127250,  // Vessel Heading
    127251,  // Rate of Turn
    127252,  // Heave
    127257,  // Attitude

    // DST (3 PGNs)
    128267,  // Water Depth
    128259,  // Speed (Water Referenced)
    130316,  // Temperature Extended Range

    // Engine (2 PGNs)
    127488,  // Engine Parameters, Rapid Update
    127489,  // Engine Parameters, Dynamic

    // Wind (1 PGN)
    130306,  // Wind Data

    // Terminator (required by NMEA2000 library)
    0
};
```

**Requirements**:
- ✅ Array declared as `const unsigned long[]`
- ✅ All 13 supported PGNs included
- ✅ Terminated with 0 (NMEA2000 library convention)
- ✅ Comments indicate PGN group and description

### 2. Extend Receive Messages

```cpp
nmea2000->ExtendReceiveMessages(PGNList);
```

**Requirements**:
- ✅ Call `ExtendReceiveMessages()` with PGN list
- ✅ Must be called BEFORE `AttachMsgHandler()`
- ✅ Tells NMEA2000 library to accept these PGNs (filters other PGNs at hardware level)

**NMEA2000 Library Behavior**:
- Configures CAN bus acceptance filters
- Only PGNs in list will trigger message reception
- Reduces CPU load by filtering unwanted messages at hardware level

### 3. Create Message Handler

```cpp
static N2kBoatDataHandler handler(boatData, logger);
```

**Requirements**:
- ✅ Use `static` keyword to ensure handler lifetime matches NMEA2000 instance
- ✅ Pass `boatData` and `logger` to constructor
- ✅ Handler must remain in scope for entire program lifetime

**Rationale**: NMEA2000 library stores pointer to handler. If handler goes out of scope when function returns, pointer becomes invalid (dangling pointer → undefined behavior).

### 4. Attach Message Handler

```cpp
nmea2000->AttachMsgHandler(&handler);
```

**Requirements**:
- ✅ Call `AttachMsgHandler()` with pointer to handler
- ✅ Must be called AFTER `ExtendReceiveMessages()`
- ✅ Must be called AFTER `nmea2000->Open()`

**NMEA2000 Library Behavior**:
- Stores pointer to handler in internal list
- Calls `handler->HandleMsg()` for each received PGN
- Multiple handlers can be attached (library calls all handlers for each message)

### 5. Log Registration Success

```cpp
logger->broadcastLog(LogLevel::INFO, "NMEA2000", "HANDLERS_REGISTERED",
    F("{\"count\":13,\"pgns\":[129025,129026,129029,127258,127250,127251,127252,127257,128267,128259,130316,127488,127489,130306]}"));
```

**Requirements**:
- ✅ Log INFO level (visible in production builds)
- ✅ Include handler count (13)
- ✅ Include full PGN list in JSON format
- ✅ Use F() macro for string literal (save RAM)

**Rationale**: Confirms handler registration during startup. Useful for debugging if PGNs not being received.

## Call Sequence in main.cpp

### Initialization Order

```cpp
void setup() {
    // ... WiFi, I2C, OLED, Serial2 initialization ...

    // STEP 5: NMEA2000 CAN bus initialization
    tNMEA2000 *nmea2000 = new tNMEA2000_esp32(CAN_TX_PIN, CAN_RX_PIN);
    nmea2000->SetProductInformation(...);
    nmea2000->SetDeviceInformation(...);
    nmea2000->SetMode(tNMEA2000::N2km_ListenAndNode);
    nmea2000->EnableForward(false);
    nmea2000->Open();  // ← MUST be called before RegisterN2kHandlers()

    // STEP 6: Register message handlers
    RegisterN2kHandlers(nmea2000, boatData, &logger);  // ← THIS FUNCTION

    // STEP 7: ReactESP event loops
    // ...
}
```

**Requirements**:
- ✅ Call AFTER `nmea2000->Open()`
- ✅ Call AFTER BoatData initialization
- ✅ Call AFTER WebSocketLogger initialization
- ✅ Call BEFORE ReactESP event loops start

## Error Handling

### Nullptr Parameters

```cpp
void RegisterN2kHandlers(tNMEA2000* nmea2000, BoatData* boatData, WebSocketLogger* logger) {
    // Precondition check
    if (nmea2000 == nullptr || boatData == nullptr || logger == nullptr) {
        return;  // Fail silently - no logging possible if logger is nullptr
    }

    // ... continue with registration ...
}
```

**Requirements**:
- ✅ Check all three pointers for nullptr
- ✅ Return immediately if any pointer is nullptr
- ✅ Do not log error (logger may be nullptr)

**Rationale**: Fail-safe behavior. System can still operate without NMEA2000 handlers (graceful degradation).

### NMEA2000 Not Opened

**Scenario**: `RegisterN2kHandlers()` called before `nmea2000->Open()`

**Behavior**: NMEA2000 library behavior undefined. May crash or silently ignore handler.

**Prevention**:
- ✅ Document precondition in function header comment
- ✅ Enforce correct call order in main.cpp setup()
- ✅ Optional: Check `nmea2000->GetN2kMode()` to verify Open() was called

```cpp
// Optional precondition check
if (nmea2000->GetN2kMode() == tNMEA2000::N2km_Off) {
    logger->broadcastLog(LogLevel::ERROR, "NMEA2000", "NOT_OPENED",
        F("{\"reason\":\"RegisterN2kHandlers called before Open()\"}"));
    return;
}
```

## Memory Requirements

### Stack Usage

```cpp
const unsigned long PGNList[] = { ... };  // 14 * 4 bytes = 56 bytes
static N2kBoatDataHandler handler(...);   // 16 bytes (static, not on stack)
─────────────────────────────────────────
Total stack: ~60 bytes
```

**Requirements**:
- ✅ Stack usage <100 bytes
- ✅ PGN list on stack (local variable)
- ✅ Handler NOT on stack (static variable)

### Heap Usage

**Requirements**:
- ✅ No heap allocation (malloc/new)
- ✅ Handler created with static storage (not new N2kBoatDataHandler)

## Performance Requirements

### Execution Time

**Maximum Execution Time**: 10ms

**Breakdown**:
- `ExtendReceiveMessages()`: ~5ms (configures CAN filters)
- Handler construction: <1μs (trivial initialization)
- `AttachMsgHandler()`: <1μs (adds pointer to list)
- Logging: ~1ms (WebSocket broadcast)
- **Total**: ~6ms

**Rationale**: Called once during startup. Acceptable delay before entering main loop.

## Testing Requirements

### Unit Tests (test_nmea2000_units/)

- ✅ Function called with valid parameters → Handler registered successfully
- ✅ Function called with nullptr nmea2000 → No crash, immediate return
- ✅ Function called with nullptr boatData → No crash, immediate return
- ✅ Function called with nullptr logger → No crash, immediate return
- ✅ INFO log message includes correct PGN count (13)
- ✅ INFO log message includes correct PGN list

### Integration Tests (test_nmea2000_integration/)

- ✅ Handler receives messages for all 13 supported PGNs
- ✅ Handler does not receive messages for unsupported PGNs
- ✅ Multiple calls to RegisterN2kHandlers() do not cause errors (handler re-attached)

### Mock Testing Pattern

```cpp
class MockNMEA2000 : public tNMEA2000 {
public:
    std::vector<unsigned long> extendedPGNs;
    tNMEA2000::tMsgHandler* attachedHandler = nullptr;

    void ExtendReceiveMessages(const unsigned long *PGNList) override {
        for (int i = 0; PGNList[i] != 0; i++) {
            extendedPGNs.push_back(PGNList[i]);
        }
    }

    void AttachMsgHandler(tNMEA2000::tMsgHandler *handler) override {
        attachedHandler = handler;
    }
};

TEST(RegisterN2kHandlers, RegistersAllPGNs) {
    MockNMEA2000 nmea2000;
    BoatData boatData(...);
    WebSocketLogger logger;

    RegisterN2kHandlers(&nmea2000, &boatData, &logger);

    // Verify all 13 PGNs were extended
    ASSERT_EQ(nmea2000.extendedPGNs.size(), 13);
    ASSERT_NE(std::find(nmea2000.extendedPGNs.begin(), nmea2000.extendedPGNs.end(), 129025),
              nmea2000.extendedPGNs.end());
    // ... verify other PGNs ...

    // Verify handler was attached
    ASSERT_NE(nmea2000.attachedHandler, nullptr);
}
```

## Example Implementation

```cpp
/**
 * @file NMEA2000Handlers.cpp
 * @brief NMEA2000 handler registration and message routing
 */

#include "NMEA2000Handlers.h"
#include "N2kBoatDataHandler.h"

void RegisterN2kHandlers(tNMEA2000* nmea2000, BoatData* boatData, WebSocketLogger* logger) {
    // Precondition checks
    if (nmea2000 == nullptr || boatData == nullptr || logger == nullptr) {
        return;
    }

    // Define list of all handled PGNs
    const unsigned long PGNList[] = {
        // GPS (4 PGNs)
        129025,  // Position, Rapid Update
        129026,  // COG & SOG, Rapid Update
        129029,  // GNSS Position Data
        127258,  // Magnetic Variation

        // Compass (4 PGNs)
        127250,  // Vessel Heading
        127251,  // Rate of Turn
        127252,  // Heave
        127257,  // Attitude

        // DST (3 PGNs)
        128267,  // Water Depth
        128259,  // Speed (Water Referenced)
        130316,  // Temperature Extended Range

        // Engine (2 PGNs)
        127488,  // Engine Parameters, Rapid Update
        127489,  // Engine Parameters, Dynamic

        // Wind (1 PGN)
        130306,  // Wind Data

        // Terminator (required by NMEA2000 library)
        0
    };

    // Extend receive message list (configures CAN bus filters)
    nmea2000->ExtendReceiveMessages(PGNList);

    // Create and attach message handler (static ensures lifetime)
    static N2kBoatDataHandler handler(boatData, logger);
    nmea2000->AttachMsgHandler(&handler);

    // Log registration success
    logger->broadcastLog(LogLevel::INFO, "NMEA2000", "HANDLERS_REGISTERED",
        F("{\"count\":13,\"pgns\":[129025,129026,129029,127258,127250,127251,127252,127257,128267,128259,130316,127488,127489,130306]}"));
}
```

## Compliance Checklist

Before marking RegisterN2kHandlers() function as complete, verify:

- [ ] Function signature matches contract: `void RegisterN2kHandlers(tNMEA2000*, BoatData*, WebSocketLogger*)`
- [ ] Checks for nullptr parameters (nmea2000, boatData, logger)
- [ ] Defines PGN list with all 13 supported PGNs
- [ ] PGN list terminated with 0
- [ ] Calls `ExtendReceiveMessages()` with PGN list
- [ ] Creates handler instance with `static` keyword
- [ ] Calls `AttachMsgHandler()` with handler pointer
- [ ] Logs INFO level message with PGN count and list
- [ ] INFO log uses F() macro for string literal
- [ ] Function called AFTER `nmea2000->Open()` in main.cpp
- [ ] Function called AFTER BoatData and WebSocketLogger initialization
- [ ] Execution time <10ms
- [ ] Stack usage <100 bytes
- [ ] No heap allocation
- [ ] Unit tests written and passing
- [ ] Integration tests written and passing

## Version History

- **1.0.0** (2025-10-12): Initial contract definition
