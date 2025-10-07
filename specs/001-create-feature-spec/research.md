# Research: WiFi Network Management Foundation

## Technical Context Decisions

### Storage Technology for WiFi Configuration
**Decision**: SPIFFS (SPI Flash File System) or LittleFS
**Rationale**:
- Both are supported by ESP32 Arduino framework
- Provide persistent file storage across reboots
- Allow simple file read/write operations for plain text format
- LittleFS is more robust (wear leveling, power-fail safety) - preferred
- SPIFFS is mature and well-documented - fallback option

**Alternatives Considered**:
- EEPROM: Limited size (512 bytes typical), requires manual byte manipulation
- Preferences library: Key-value only, awkward for multi-network config
- SD Card: Hardware dependency, adds complexity, not always available

**Selected**: LittleFS for production, with SPIFFS compatibility

---

### WiFi Connection Management Pattern
**Decision**: ReactESP event-driven with WiFi.begin() async pattern
**Rationale**:
- Per constitution requirement (Always-On Operation), services run independently
- ReactESP already required for async programming (see constitution)
- WiFi.onEvent() callbacks available in ESP32 Arduino
- Allows non-blocking connection attempts with timeout handling
- Fits existing reference implementation pattern (`examples/poseidongw/`)

**Alternatives Considered**:
- Blocking WiFi.begin() with delay(): Violates FR-011 (services must not block)
- Custom threading: Unnecessary complexity, RTOS overhead
- WiFiMulti library: Doesn't support priority ordering or custom timeout

**Selected**: ReactESP event loop + WiFi async events

---

### Configuration File Validation
**Decision**: Regex-free state machine parser
**Rationale**:
- Plain text format (SSID,password per line) is simple to parse
- Regex libraries add flash overhead on ESP32
- State machine approach: read line, split on comma, validate components
- Validation rules:
  - Max 3 lines
  - Each line: non-empty SSID, comma separator, non-empty password
  - SSID: 1-32 characters (WiFi spec limit)
  - Password: 8-63 characters for WPA2 (or empty for open networks)

**Alternatives Considered**:
- JSON parsing: ArduinoJson library adds ~30KB flash, overkill for simple format
- CSV library: Unnecessary dependency for 3-line files
- Regex: PCRE not available, custom regex adds complexity

**Selected**: Custom line-by-line parser with string manipulation

---

### Reboot Mechanism
**Decision**: ESP.restart() with 5-second delay
**Rationale**:
- ESP.restart() is Arduino-standard restart function
- 5-second delay prevents rapid reboot loops (edge case in spec)
- Allows UDP logging of restart reason before reboot
- Gives time for other services to log final state

**Alternatives Considered**:
- Immediate restart: Risk of boot loops if persistent issue
- Hardware watchdog only: Less controlled, no logging opportunity
- Longer delay (30s+): Unnecessarily slow recovery

**Selected**: ESP.restart() with 5-second pre-restart delay

---

### Connection Status Indication
**Decision**: Shared state variable + UDP broadcast + optional OLED
**Rationale**:
- Shared state (enum): WiFiStatus {DISCONNECTED, CONNECTING, CONNECTED, FAILED}
- UDP broadcast: Aligns with Network-Based Debugging (Principle V)
- OLED optional: Reference hardware has OLED on I2C Bus 2 (see GPIO config)
- LED: GPIO 2 built-in LED for basic visual feedback

**Alternatives Considered**:
- Serial output only: Violates constitution (serial ports reserved for NMEA)
- Callback pattern only: Harder for other services to query current state
- Web server status page: Requires WiFi to already be connected (chicken-egg)

**Selected**: Enum state + UDP broadcast + LED blink patterns

---

### Missing/Corrupted Configuration Handling
**Decision**: Factory default fallback + error logging
**Rationale**:
- On missing file: Log error via UDP (if prev WiFi connected), create empty config, reboot
- On corrupted file: Log parse errors, attempt to use valid lines, reboot if none valid
- Factory default: Empty config triggers reboot loop until user uploads valid config
- Alternative: Hardcoded fallback network (less flexible, security risk)

**Selected**: Fail-safe mode with error logging, manual recovery required

---

### File Upload Mechanism
**Decision**: HTTP POST endpoint via ESPAsyncWebServer
**Rationale**:
- ESPAsyncWebServer is pre-approved library (see constitution)
- Simple multipart/form-data file upload handler
- Works when WiFi connected (bootstrapping: need one working network first)
- Can also serve simple web UI for config editing

**Alternatives Considered**:
- Serial upload: Violates serial port usage policy
- FTP server: Additional dependency, more complex
- Bluetooth: Not all ESP32 variants, adds complexity

**Selected**: HTTP POST /upload endpoint + web form

---

## Technology Stack Summary

**Language**: C++ (C++14)
**Platform**: ESP32 (Arduino framework via PlatformIO)
**Storage**: LittleFS (persistent flash filesystem)
**Async Framework**: ReactESP (event loops)
**Web Server**: ESPAsyncWebServer (config file upload)
**Networking**: WiFi.h (ESP32 Arduino Core)
**Logging**: UDP broadcast (port 4444)
**Display**: Adafruit_SSD1306 (optional status display)

**Performance Goals**:
- Connection attempt timeout: 30 seconds (specified)
- Reboot delay: 5 seconds (prevents rapid cycling)
- File read time: <100ms (small files)
- Parse time: <50ms (3 lines max)

**Constraints**:
- Max 3 WiFi networks (memory: ~200 bytes total)
- Plain text format only
- Non-blocking service initialization
- Flash storage for persistence

**Scale**: Single-device embedded system, 24/7 operation
