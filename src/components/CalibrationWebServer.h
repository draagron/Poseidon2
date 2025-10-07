/**
 * @file CalibrationWebServer.h
 * @brief Web server for calibration parameter management
 *
 * Provides HTTP API endpoints for calibration parameters:
 * - GET /api/calibration: Retrieve current calibration parameters
 * - POST /api/calibration: Update calibration parameters
 *
 * Uses ESPAsyncWebServer for non-blocking HTTP handling.
 * Integrates with CalibrationManager for persistence.
 *
 * @see specs/003-boatdata-feature-as/tasks.md T039
 * @version 1.0.0
 * @date 2025-10-07
 */

#ifndef CALIBRATION_WEB_SERVER_H
#define CALIBRATION_WEB_SERVER_H

#include <Arduino.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include "CalibrationManager.h"
#include "BoatData.h"
#include "../config.h"

/**
 * @brief Web server for calibration parameter API
 *
 * Provides RESTful HTTP endpoints for calibration management.
 * No authentication required (open access on private boat network, per FR-036).
 */
class CalibrationWebServer {
private:
    CalibrationManager* calibrationManager;
    BoatData* boatData;

    /**
     * @brief Handle GET /api/calibration
     *
     * Returns current calibration parameters in JSON format:
     * {
     *   "leewayKFactor": 1.0,
     *   "windAngleOffset": 0.0,
     *   "lastModified": 1234567890
     * }
     *
     * @param request AsyncWebServerRequest from ESPAsyncWebServer
     */
    void handleGetCalibration(AsyncWebServerRequest* request);

    /**
     * @brief Handle POST /api/calibration
     *
     * Updates calibration parameters from JSON body:
     * {
     *   "leewayKFactor": 0.65,
     *   "windAngleOffset": 0.087
     * }
     *
     * Validation:
     * - leewayKFactor > 0 (reject if <= 0)
     * - windAngleOffset in range [-2π, 2π]
     *
     * Returns 200 on success, 400 on validation failure.
     *
     * @param request AsyncWebServerRequest from ESPAsyncWebServer
     * @param json JSON data buffer
     * @param len Length of JSON data
     * @param index Current index (for chunked uploads)
     * @param total Total length (for chunked uploads)
     */
    void handlePostCalibration(AsyncWebServerRequest* request, uint8_t* json, size_t len, size_t index, size_t total);

public:
    /**
     * @brief Constructor
     *
     * @param calibMgr Calibration manager instance
     * @param boat BoatData instance (for atomic updates)
     */
    CalibrationWebServer(CalibrationManager* calibMgr, BoatData* boat);

    /**
     * @brief Register routes with existing web server
     *
     * Adds calibration endpoints to an existing AsyncWebServer instance.
     * Must be called before server->begin().
     *
     * @param server Existing AsyncWebServer instance (from ConfigWebServer)
     */
    void registerRoutes(AsyncWebServer* server);
};

#endif // CALIBRATION_WEB_SERVER_H
