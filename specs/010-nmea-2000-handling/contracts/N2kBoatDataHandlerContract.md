# Contract: N2kBoatDataHandler Message Router Class

**Version**: 1.0.0
**Date**: 2025-10-12
**Status**: Draft

## Overview

This contract defines the interface and behavior requirements for the `N2kBoatDataHandler` class, which routes incoming NMEA2000 PGN messages to appropriate handler functions. This class implements the `tNMEA2000::tMsgHandler` interface required by the NMEA2000 library.

## Class Declaration

```cpp
/**
 * @brief NMEA2000 message router class
 *
 * Routes incoming PGN messages to appropriate handler functions based on PGN number.
 * Implements tNMEA2000::tMsgHandler interface for NMEA2000 library integration.
 *
 * Memory footprint: 16 bytes (2 pointers + vtable)
 * Execution time: <0.1ms per message (simple switch statement)
 */
class N2kBoatDataHandler : public tNMEA2000::tMsgHandler {
private:
    BoatData* boatData;         ///< BoatData instance to update
    WebSocketLogger* logger;    ///< Logger for debug output

public:
    /**
     * @brief Constructor
     *
     * @param bd BoatData instance (must not be nullptr)
     * @param log WebSocket logger (must not be nullptr)
     *
     * @pre bd != nullptr
     * @pre log != nullptr
     */
    N2kBoatDataHandler(BoatData* bd, WebSocketLogger* log);

    /**
     * @brief Handle incoming NMEA2000 message
     *
     * Routes message to appropriate handler based on PGN number.
     * Unhandled PGNs are silently ignored (no log, no error).
     *
     * @param N2kMsg NMEA2000 message
     */
    void HandleMsg(const tN2kMsg &N2kMsg) override;
};
```

## Constructor Requirements

### Signature

```cpp
N2kBoatDataHandler(BoatData* bd, WebSocketLogger* log)
```

### Parameters

- **bd**: `BoatData*` (non-null pointer)
  - BoatData instance for all handler function calls
  - **Precondition**: Must not be nullptr
  - **Storage**: Stored as member variable `boatData`

- **log**: `WebSocketLogger*` (non-null pointer)
  - WebSocket logger for all handler function calls
  - **Precondition**: Must not be nullptr
  - **Storage**: Stored as member variable `logger`

### Behavior

```cpp
N2kBoatDataHandler::N2kBoatDataHandler(BoatData* bd, WebSocketLogger* log)
    : boatData(bd), logger(log) {
    // No additional initialization required
}
```

**Requirements**:
- ✅ Initialize member variables `boatData` and `logger`
- ✅ No heap allocation
- ✅ No I/O operations
- ✅ Execution time: <1μs (trivial initialization)

## HandleMsg() Method Requirements

### Signature

```cpp
void HandleMsg(const tN2kMsg &N2kMsg) override
```

### Parameters

- **N2kMsg**: `const tN2kMsg&` (read-only)
  - NMEA2000 message from CAN bus
  - Contains `PGN` field for routing
  - Passed to handler functions unchanged

### Behavior

```cpp
void N2kBoatDataHandler::HandleMsg(const tN2kMsg &N2kMsg) {
    switch (N2kMsg.PGN) {
        // GPS handlers
        case 129025: HandleN2kPGN129025(N2kMsg, boatData, logger); break;
        case 129026: HandleN2kPGN129026(N2kMsg, boatData, logger); break;
        case 129029: HandleN2kPGN129029(N2kMsg, boatData, logger); break;
        case 127258: HandleN2kPGN127258(N2kMsg, boatData, logger); break;

        // Compass handlers
        case 127250: HandleN2kPGN127250(N2kMsg, boatData, logger); break;
        case 127251: HandleN2kPGN127251(N2kMsg, boatData, logger); break;
        case 127252: HandleN2kPGN127252(N2kMsg, boatData, logger); break;
        case 127257: HandleN2kPGN127257(N2kMsg, boatData, logger); break;

        // DST handlers
        case 128267: HandleN2kPGN128267(N2kMsg, boatData, logger); break;
        case 128259: HandleN2kPGN128259(N2kMsg, boatData, logger); break;
        case 130316: HandleN2kPGN130316(N2kMsg, boatData, logger); break;

        // Engine handlers
        case 127488: HandleN2kPGN127488(N2kMsg, boatData, logger); break;
        case 127489: HandleN2kPGN127489(N2kMsg, boatData, logger); break;

        // Wind handlers
        case 130306: HandleN2kPGN130306(N2kMsg, boatData, logger); break;

        // All other PGNs silently ignored
        default: break;
    }
}
```

**Requirements**:
- ✅ Use switch statement on `N2kMsg.PGN` for routing
- ✅ Call appropriate handler function for each supported PGN
- ✅ Pass `N2kMsg`, `boatData`, and `logger` to handler functions
- ✅ Silently ignore unsupported PGNs (no log, no error)
- ✅ Execution time: <0.1ms per message (simple switch, no complex logic)

## Supported PGN List

### GPS Messages (4 PGNs)

| PGN    | Handler Function | Description |
|--------|-----------------|-------------|
| 129025 | `HandleN2kPGN129025` | Position, Rapid Update |
| 129026 | `HandleN2kPGN129026` | COG & SOG, Rapid Update |
| 129029 | `HandleN2kPGN129029` | GNSS Position Data |
| 127258 | `HandleN2kPGN127258` | Magnetic Variation |

### Compass Messages (4 PGNs)

| PGN    | Handler Function | Description |
|--------|-----------------|-------------|
| 127250 | `HandleN2kPGN127250` | Vessel Heading |
| 127251 | `HandleN2kPGN127251` | Rate of Turn |
| 127252 | `HandleN2kPGN127252` | Heave |
| 127257 | `HandleN2kPGN127257` | Attitude |

### DST Messages (3 PGNs)

| PGN    | Handler Function | Description |
|--------|-----------------|-------------|
| 128267 | `HandleN2kPGN128267` | Water Depth |
| 128259 | `HandleN2kPGN128259` | Speed (Water Referenced) |
| 130316 | `HandleN2kPGN130316` | Temperature Extended Range |

### Engine Messages (2 PGNs)

| PGN    | Handler Function | Description |
|--------|-----------------|-------------|
| 127488 | `HandleN2kPGN127488` | Engine Parameters, Rapid Update |
| 127489 | `HandleN2kPGN127489` | Engine Parameters, Dynamic |

### Wind Messages (1 PGN)

| PGN    | Handler Function | Description |
|--------|-----------------|-------------|
| 130306 | `HandleN2kPGN130306` | Wind Data |

**Total**: 13 supported PGNs

## Memory Requirements

### Static Allocation

```cpp
sizeof(N2kBoatDataHandler) = 16 bytes
```

**Components**:
- `boatData` pointer: 8 bytes
- `logger` pointer: 8 bytes
- vtable pointer: (shared with parent class, no additional overhead)

**Requirements**:
- ✅ No heap allocation
- ✅ No dynamic memory
- ✅ No std::vector, std::map, or other containers
- ✅ Statically allocated instance (see usage pattern)

### Usage Pattern

```cpp
// Declare as static to ensure lifetime matches NMEA2000 instance
static N2kBoatDataHandler handler(boatData, logger);
nmea2000->AttachMsgHandler(&handler);
```

**Rationale**: Handler must remain in scope for lifetime of NMEA2000 library. Static allocation ensures handler is not destroyed when `RegisterN2kHandlers()` function returns.

## Performance Requirements

### Execution Time

**Maximum Execution Time**: 0.1ms per message

**Breakdown**:
- Switch statement: <1μs (simple integer comparison)
- Function call overhead: ~1μs
- Handler function execution: ~0.5ms (see HandlerFunctionContract.md)
- **Total**: <0.6ms per message

**Rationale**: Must not block CAN interrupt or ReactESP event loop.

### Throughput

**Typical Load**: 100 messages/second
- Total CPU time: 60ms/second (6% utilization)
- Acceptable with ReactESP architecture

**Peak Load**: 1000 messages/second
- Total CPU time: 600ms/second (60% utilization)
- Still acceptable (system remains responsive)

## Error Handling Requirements

### Nullptr Handling

**Scenario**: Constructor called with nullptr parameter

**Behavior**:
```cpp
// Precondition check in constructor (optional)
N2kBoatDataHandler::N2kBoatDataHandler(BoatData* bd, WebSocketLogger* log)
    : boatData(bd), logger(log) {
    // No additional checks required - handler functions check for nullptr
}
```

**Rationale**: Handler functions already check for nullptr (see HandlerFunctionContract.md). Constructor can assume valid pointers if called correctly from `RegisterN2kHandlers()`.

### Unsupported PGN Handling

**Scenario**: Message received with PGN not in switch statement

**Behavior**:
```cpp
default: break;  // Silently ignore unsupported PGNs
```

**Requirements**:
- ✅ No log message (avoid log spam from unknown devices)
- ✅ No error counter increment
- ✅ System continues normal operation

**Rationale**: NMEA2000 bus may have many devices broadcasting hundreds of PGN types. Only log/process PGNs explicitly handled by this gateway.

## Integration with NMEA2000 Library

### Attachment to NMEA2000 Instance

```cpp
void RegisterN2kHandlers(tNMEA2000* nmea2000, BoatData* boatData, WebSocketLogger* logger) {
    // ... ExtendReceiveMessages(PGNList) ...

    // Create handler instance (static ensures lifetime)
    static N2kBoatDataHandler handler(boatData, logger);

    // Attach to NMEA2000 library
    nmea2000->AttachMsgHandler(&handler);

    // ... logging ...
}
```

**Requirements**:
- ✅ Use `static` keyword to ensure handler lifetime
- ✅ Attach via `nmea2000->AttachMsgHandler(&handler)`
- ✅ Attach AFTER `nmea2000->Open()` called
- ✅ Attach AFTER `nmea2000->ExtendReceiveMessages(PGNList)` called

### Message Flow

```
1. CAN bus interrupt (hardware) → tNMEA2000_esp32::CANMsgIn()
2. NMEA2000 library buffer → tNMEA2000::ParseMessages()
3. Message dispatcher → tNMEA2000::HandleMsg()
4. Attached handler → N2kBoatDataHandler::HandleMsg()  ← THIS CLASS
5. PGN routing → HandleN2kPGN<NUMBER>()
6. BoatData update → boatData->set<DataType>()
```

**Latency**: <2ms from CAN interrupt to BoatData update

## Testing Requirements

### Unit Tests (test_nmea2000_units/)

- ✅ Constructor stores boatData and logger pointers correctly
- ✅ HandleMsg() routes each supported PGN to correct handler function
- ✅ HandleMsg() silently ignores unsupported PGNs (no log, no error)
- ✅ Handler functions called with correct parameters (N2kMsg, boatData, logger)

### Integration Tests (test_nmea2000_integration/)

- ✅ Handler attached to NMEA2000 library successfully
- ✅ Real PGN messages routed to correct handlers
- ✅ Multiple messages processed in sequence without errors
- ✅ Execution time <0.1ms per message

### Mock Testing Pattern

```cpp
// Mock handler function to verify routing
std::map<unsigned long, int> callCounts;

void MockHandleN2kPGN129025(const tN2kMsg &N2kMsg, BoatData* boatData, WebSocketLogger* logger) {
    callCounts[129025]++;
}

// Test case
TEST(N2kBoatDataHandler, RoutesToCorrectHandler) {
    // Create test message with PGN 129025
    tN2kMsg testMsg;
    testMsg.PGN = 129025;

    // Create handler and call HandleMsg
    N2kBoatDataHandler handler(&boatData, &logger);
    handler.HandleMsg(testMsg);

    // Verify handler function was called
    ASSERT_EQ(callCounts[129025], 1);
}
```

## Example Implementation

```cpp
// N2kBoatDataHandler.h
#ifndef N2K_BOAT_DATA_HANDLER_H
#define N2K_BOAT_DATA_HANDLER_H

#include <NMEA2000.h>
#include "../components/BoatData.h"
#include "../utils/WebSocketLogger.h"

class N2kBoatDataHandler : public tNMEA2000::tMsgHandler {
private:
    BoatData* boatData;
    WebSocketLogger* logger;

public:
    N2kBoatDataHandler(BoatData* bd, WebSocketLogger* log);
    void HandleMsg(const tN2kMsg &N2kMsg) override;
};

#endif // N2K_BOAT_DATA_HANDLER_H
```

```cpp
// N2kBoatDataHandler.cpp
#include "N2kBoatDataHandler.h"
#include "NMEA2000Handlers.h"  // Handler function declarations

N2kBoatDataHandler::N2kBoatDataHandler(BoatData* bd, WebSocketLogger* log)
    : boatData(bd), logger(log) {
    // No additional initialization
}

void N2kBoatDataHandler::HandleMsg(const tN2kMsg &N2kMsg) {
    switch (N2kMsg.PGN) {
        // GPS handlers (4 PGNs)
        case 129025: HandleN2kPGN129025(N2kMsg, boatData, logger); break;
        case 129026: HandleN2kPGN129026(N2kMsg, boatData, logger); break;
        case 129029: HandleN2kPGN129029(N2kMsg, boatData, logger); break;
        case 127258: HandleN2kPGN127258(N2kMsg, boatData, logger); break;

        // Compass handlers (4 PGNs)
        case 127250: HandleN2kPGN127250(N2kMsg, boatData, logger); break;
        case 127251: HandleN2kPGN127251(N2kMsg, boatData, logger); break;
        case 127252: HandleN2kPGN127252(N2kMsg, boatData, logger); break;
        case 127257: HandleN2kPGN127257(N2kMsg, boatData, logger); break;

        // DST handlers (3 PGNs)
        case 128267: HandleN2kPGN128267(N2kMsg, boatData, logger); break;
        case 128259: HandleN2kPGN128259(N2kMsg, boatData, logger); break;
        case 130316: HandleN2kPGN130316(N2kMsg, boatData, logger); break;

        // Engine handlers (2 PGNs)
        case 127488: HandleN2kPGN127488(N2kMsg, boatData, logger); break;
        case 127489: HandleN2kPGN127489(N2kMsg, boatData, logger); break;

        // Wind handlers (1 PGN)
        case 130306: HandleN2kPGN130306(N2kMsg, boatData, logger); break;

        // Silently ignore all other PGNs
        default: break;
    }
}
```

## Compliance Checklist

Before marking the N2kBoatDataHandler class as complete, verify:

- [ ] Class inherits from `tNMEA2000::tMsgHandler`
- [ ] Constructor accepts `BoatData*` and `WebSocketLogger*` parameters
- [ ] Constructor stores parameters as member variables
- [ ] Constructor uses no heap allocation
- [ ] `HandleMsg()` method overrides base class method
- [ ] `HandleMsg()` uses switch statement on `N2kMsg.PGN`
- [ ] All 13 supported PGNs routed to correct handler functions
- [ ] Handler functions called with `(N2kMsg, boatData, logger)` parameters
- [ ] Unsupported PGNs silently ignored (default case)
- [ ] Class size ≤16 bytes (verified with sizeof())
- [ ] Execution time <0.1ms per message (profiled)
- [ ] Unit tests written and passing
- [ ] Integration tests written and passing
- [ ] Handler instance declared as static in RegisterN2kHandlers()
- [ ] Handler attached via `AttachMsgHandler()` after `Open()` and `ExtendReceiveMessages()`

## Version History

- **1.0.0** (2025-10-12): Initial contract definition
