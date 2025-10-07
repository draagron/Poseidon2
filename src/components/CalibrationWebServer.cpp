/**
 * @file CalibrationWebServer.cpp
 * @brief Implementation of calibration web server
 *
 * @see CalibrationWebServer.h
 */

#include "CalibrationWebServer.h"
#include <math.h>

CalibrationWebServer::CalibrationWebServer(CalibrationManager* calibMgr, BoatData* boat)
    : calibrationManager(calibMgr), boatData(boat) {
}

void CalibrationWebServer::registerRoutes(AsyncWebServer* server) {
    if (server == nullptr) {
        return;
    }

    // GET /api/calibration - Retrieve current calibration
    server->on("/api/calibration", HTTP_GET, [this](AsyncWebServerRequest* request) {
        this->handleGetCalibration(request);
    });

    // POST /api/calibration - Update calibration parameters
    server->on("/api/calibration", HTTP_POST,
        [](AsyncWebServerRequest* request) {
            // Send response (actual handling done in body callback)
            request->send(200, "application/json", "{\"status\":\"success\",\"message\":\"Calibration updated\"}");
        },
        nullptr,  // No file upload handler
        [this](AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) {
            this->handlePostCalibration(request, data, len, index, total);
        }
    );
}

void CalibrationWebServer::handleGetCalibration(AsyncWebServerRequest* request) {
    // Get current calibration
    CalibrationParameters calib = calibrationManager->getCalibration();

    // Build JSON response
    StaticJsonDocument<256> doc;
    doc["leewayKFactor"] = calib.leewayCalibrationFactor;
    doc["windAngleOffset"] = calib.windAngleOffset;
    doc["valid"] = calib.valid;
    doc["lastModified"] = calib.lastModified;

    String response;
    serializeJson(doc, response);

    request->send(200, "application/json", response);
}

void CalibrationWebServer::handlePostCalibration(AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) {
    // Only process on first chunk (index == 0) and when we have all data
    if (index != 0) {
        return;  // Wait for complete data
    }

    // Parse JSON
    StaticJsonDocument<256> doc;
    DeserializationError error = deserializeJson(doc, data, len);

    if (error) {
        request->send(400, "application/json",
            "{\"status\":\"error\",\"message\":\"Invalid JSON\"}");
        return;
    }

    // Extract parameters
    if (!doc.containsKey("leewayKFactor") || !doc.containsKey("windAngleOffset")) {
        request->send(400, "application/json",
            "{\"status\":\"error\",\"message\":\"Missing required fields: leewayKFactor, windAngleOffset\"}");
        return;
    }

    double kFactor = doc["leewayKFactor"];
    double windOffset = doc["windAngleOffset"];

    // Validate K factor (must be > 0)
    if (kFactor <= 0.0) {
        request->send(400, "application/json",
            "{\"status\":\"error\",\"message\":\"leewayKFactor must be greater than 0\"}");
        return;
    }

    // Validate wind offset (must be in range [-2π, 2π])
    const double MAX_WIND_OFFSET = 2.0 * M_PI;
    if (windOffset < -MAX_WIND_OFFSET || windOffset > MAX_WIND_OFFSET) {
        request->send(400, "application/json",
            "{\"status\":\"error\",\"message\":\"windAngleOffset must be in range [-2π, 2π]\"}");
        return;
    }

    // Create new calibration parameters
    CalibrationParameters newCalib;
    newCalib.leewayCalibrationFactor = kFactor;
    newCalib.windAngleOffset = windOffset;
    newCalib.valid = true;
    newCalib.version = 1;
    newCalib.lastModified = millis() / 1000;

    // Validate using CalibrationManager
    if (!calibrationManager->validateCalibration(newCalib)) {
        request->send(400, "application/json",
            "{\"status\":\"error\",\"message\":\"Calibration validation failed\"}");
        return;
    }

    // Apply atomically to BoatData (during calculation cycle pause)
    // Note: In ReactESP single-threaded model, this is atomic by design
    // Convert CalibrationParameters to CalibrationData for BoatData
    CalibrationData boatCalib;
    boatCalib.leewayCalibrationFactor = newCalib.leewayCalibrationFactor;
    boatCalib.windAngleOffset = newCalib.windAngleOffset;
    boatCalib.loaded = true;
    boatData->setCalibration(boatCalib);

    // Persist to flash
    if (!calibrationManager->saveToFlash(newCalib)) {
        request->send(500, "application/json",
            "{\"status\":\"error\",\"message\":\"Failed to save calibration to flash\"}");
        return;
    }

    // Success response
    StaticJsonDocument<256> responseDoc;
    responseDoc["status"] = "success";
    responseDoc["message"] = "Calibration updated and saved";
    responseDoc["leewayKFactor"] = kFactor;
    responseDoc["windAngleOffset"] = windOffset;

    String response;
    serializeJson(responseDoc, response);

    request->send(200, "application/json", response);
}
