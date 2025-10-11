#ifndef NMEA0183HANDLER_H
#define NMEA0183HANDLER_H

#include <NMEA0183.h>
#include "hal/interfaces/ISerialPort.h"
#include "components/BoatData.h"
#include "utils/WebSocketLogger.h"

/**
 * @file NMEA0183Handler.h
 * @brief NMEA 0183 sentence handler component
 *
 * Coordinates NMEA 0183 sentence reception, parsing, and BoatData integration.
 * Processes 5 sentence types from 2 talker IDs:
 * - AP (autopilot): RSA (rudder angle), HDM (magnetic heading)
 * - VH (VHF radio): GGA (GPS position), RMC (GPS + variation), VTG (COG/SOG + variation)
 *
 * Architecture:
 * - Non-blocking: Processes available sentences in <50ms per ReactESP cycle
 * - HAL abstracted: Uses ISerialPort for Serial2 access
 * - Stateless: No buffering beyond Serial2 receive FIFO
 * - Silent discard: Invalid/out-of-range sentences logged at DEBUG, not ERROR
 *
 * Usage:
 * @code
 * tNMEA0183 nmea0183;
 * ISerialPort* serialPort = new ESP32SerialPort(&Serial2);
 * NMEA0183Handler handler(&nmea0183, serialPort, boatData, &logger);
 * handler.init();
 *
 * // In ReactESP loop (10ms interval)
 * app.onRepeat(10, []() {
 *     handler.processSentences();
 * });
 * @endcode
 */
class NMEA0183Handler {
public:
    /**
     * @brief Constructor
     *
     * @param nmea0183 NMEA0183 library instance (for sentence parsing)
     * @param serialPort Serial port interface (ISerialPort wrapper for Serial2)
     * @param boatData BoatData repository (for sensor updates via ISensorUpdate)
     * @param logger WebSocket logger (for network debugging)
     */
    NMEA0183Handler(tNMEA0183* nmea0183, ISerialPort* serialPort,
                    BoatData* boatData, WebSocketLogger* logger);

    /**
     * @brief Initialize Serial2 and NMEA parser
     *
     * Starts Serial2 at 4800 baud (NMEA 0183 standard rate) and prepares parser.
     * Must be called before processSentences().
     */
    void init();

    /**
     * @brief Process pending NMEA sentences (called from ReactESP loop)
     *
     * Reads available bytes from Serial2, feeds to NMEA parser, and dispatches
     * complete sentences to handler functions. Non-blocking operation - processes
     * all available sentences or returns within 50ms budget (FR-027).
     *
     * Called every 10ms by ReactESP event loop.
     */
    void processSentences();

    /**
     * @brief Dispatch message to appropriate handler
     *
     * Called by static callback from NMEA0183 library. Looks up handler in
     * dispatch table and calls appropriate handler function.
     *
     * @param msg NMEA0183 message object
     */
    void dispatchMessage(const tNMEA0183Msg& msg);

private:
    tNMEA0183* nmea0183_;           ///< NMEA0183 library instance
    ISerialPort* serialPort_;       ///< Serial port interface
    BoatData* boatData_;            ///< BoatData repository
    WebSocketLogger* logger_;       ///< WebSocket logger

    /**
     * @brief Handler dispatch table entry
     *
     * Maps NMEA message codes to handler function pointers for efficient dispatch.
     */
    struct HandlerEntry {
        const char* messageCode;    ///< NMEA message code (e.g., "RSA", "HDM")
        void (NMEA0183Handler::*handler)(const tNMEA0183Msg&);  ///< Handler function
    };

    /// Handler dispatch table (5 supported message types)
    static const HandlerEntry handlers_[];
    static const size_t handlersCount_;

    // Sentence-specific handler functions

    /**
     * @brief Handle RSA (Rudder Sensor Angle) sentence from autopilot
     *
     * Extracts rudder angle, converts to radians, updates BoatData.RudderData.
     * Validates talker ID="AP", range ±90°, status='A'.
     *
     * @param msg NMEA0183 message object
     */
    void handleRSA(const tNMEA0183Msg& msg);

    /**
     * @brief Handle HDM (Heading Magnetic) sentence from autopilot
     *
     * Extracts magnetic heading, converts to radians, updates BoatData.CompassData.
     * Validates talker ID="AP", range [0°, 360°].
     *
     * @param msg NMEA0183 message object
     */
    void handleHDM(const tNMEA0183Msg& msg);

    /**
     * @brief Handle GGA (GPS Fix Data) sentence from VHF
     *
     * Extracts lat/lon in DDMM.MMMM format, converts to decimal degrees,
     * updates BoatData.GPSData. Validates talker ID="VH", fix quality > 0,
     * coordinate ranges.
     *
     * @param msg NMEA0183 message object
     */
    void handleGGA(const tNMEA0183Msg& msg);

    /**
     * @brief Handle RMC (Recommended Minimum Navigation) sentence from VHF
     *
     * Extracts lat/lon/COG/SOG/variation, converts units, updates
     * BoatData.GPSData and BoatData.CompassData. Validates talker ID="VH",
     * status='A', variation range ±30°.
     *
     * @param msg NMEA0183 message object
     */
    void handleRMC(const tNMEA0183Msg& msg);

    /**
     * @brief Handle VTG (Track Made Good) sentence from VHF
     *
     * Extracts true/magnetic COG and SOG, calculates variation from difference,
     * updates BoatData.GPSData and BoatData.CompassData. Validates talker ID="VH",
     * calculated variation range ±30°.
     *
     * @param msg NMEA0183 message object
     */
    void handleVTG(const tNMEA0183Msg& msg);
};

#endif // NMEA0183HANDLER_H
