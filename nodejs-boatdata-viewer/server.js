#!/usr/bin/env node

/**
 * Node.js WebSocket Proxy Server for ESP32 BoatData
 *
 * Connects to ESP32 WebSocket endpoint and relays data to multiple browser clients.
 * Provides HTTP server for serving dashboard HTML.
 */

const express = require('express');
const WebSocket = require('ws');
const http = require('http');
const fs = require('fs');
const path = require('path');

// Load configuration
let config;
try {
    // Try local config first (for overrides)
    if (fs.existsSync('./config.local.json')) {
        config = JSON.parse(fs.readFileSync('./config.local.json', 'utf8'));
        console.log('✓ Loaded config.local.json');
    } else {
        config = JSON.parse(fs.readFileSync('./config.json', 'utf8'));
        console.log('✓ Loaded config.json');
    }
} catch (error) {
    console.error('✗ Failed to load configuration:', error.message);
    process.exit(1);
}

// Override with environment variables if provided
if (process.env.ESP32_IP) config.esp32.ip = process.env.ESP32_IP;
if (process.env.PORT) config.server.port = parseInt(process.env.PORT);

// Create Express app
const app = express();
const server = http.createServer(app);

// Serve static files from public directory
app.use(express.static('public'));

// API endpoint for configuration status
app.get('/api/config', (req, res) => {
    res.json({
        esp32: {
            ip: config.esp32.ip,
            port: config.esp32.port,
            boatdata: {
                connected: esp32Connected,
                lastMessageTime: lastMessageTime ? new Date(lastMessageTime).toISOString() : null,
                clients: browserClients.size
            },
            sourceStats: {
                connected: esp32SourceStatsConnected,
                lastMessageTime: lastSourceStatsMessageTime ? new Date(lastSourceStatsMessageTime).toISOString() : null,
                clients: sourceStatsClients.size
            }
        },
        server: {
            port: config.server.port,
            totalClients: browserClients.size + sourceStatsClients.size,
            uptime: Math.floor((Date.now() - serverStartTime) / 1000)
        }
    });
});

// WebSocket Server for /boatdata (for browser clients)
const wss = new WebSocket.Server({ server, path: '/boatdata' });

// WebSocket Server for /source-stats (for browser clients)
const wssSourceStats = new WebSocket.Server({ server, path: '/source-stats' });

// Track connected browser clients
const browserClients = new Set();
const sourceStatsClients = new Set();

// ESP32 WebSocket client state for /boatdata
let esp32Client = null;
let esp32Connected = false;
let reconnectTimer = null;
let lastMessageTime = null;

// ESP32 WebSocket client state for /source-stats
let esp32SourceStatsClient = null;
let esp32SourceStatsConnected = false;
let reconnectSourceStatsTimer = null;
let lastSourceStatsMessageTime = null;

const serverStartTime = Date.now();

// Handle browser client connections
wss.on('connection', (ws, req) => {
    const clientId = `${req.socket.remoteAddress}:${req.socket.remotePort}`;
    browserClients.add(ws);

    console.log(`[BROWSER] Client connected: ${clientId} (total: ${browserClients.size})`);

    // Send connection status to new client
    sendStatusUpdate(ws);

    // Handle browser client disconnect
    ws.on('close', () => {
        browserClients.delete(ws);
        console.log(`[BROWSER] Client disconnected: ${clientId} (total: ${browserClients.size})`);
    });

    ws.on('error', (error) => {
        console.error(`[BROWSER] Client error: ${clientId}:`, error.message);
        browserClients.delete(ws);
    });
});

// Handle source stats browser client connections
wssSourceStats.on('connection', (ws, req) => {
    const clientId = `${req.socket.remoteAddress}:${req.socket.remotePort}`;
    sourceStatsClients.add(ws);

    console.log(`[SOURCE-STATS] Client connected: ${clientId} (total: ${sourceStatsClients.size})`);

    // Send connection status to new client
    sendSourceStatsStatusUpdate(ws);

    // Handle browser client disconnect
    ws.on('close', () => {
        sourceStatsClients.delete(ws);
        console.log(`[SOURCE-STATS] Client disconnected: ${clientId} (total: ${sourceStatsClients.size})`);
    });

    ws.on('error', (error) => {
        console.error(`[SOURCE-STATS] Client error: ${clientId}:`, error.message);
        sourceStatsClients.delete(ws);
    });
});

// Send status update to client(s)
function sendStatusUpdate(client = null) {
    const statusMessage = JSON.stringify({
        type: 'status',
        esp32Connected: esp32Connected,
        esp32Ip: config.esp32.ip,
        timestamp: Date.now()
    });

    if (client) {
        // Send to specific client
        if (client.readyState === WebSocket.OPEN) {
            client.send(statusMessage);
        }
    } else {
        // Broadcast to all clients
        broadcastToBrowsers(statusMessage);
    }
}

// Broadcast message to all connected browser clients
function broadcastToBrowsers(message) {
    let successCount = 0;
    let failCount = 0;

    browserClients.forEach((client) => {
        if (client.readyState === WebSocket.OPEN) {
            try {
                client.send(message);
                successCount++;
            } catch (error) {
                console.error('[BROADCAST] Failed to send to client:', error.message);
                failCount++;
            }
        } else {
            failCount++;
        }
    });

    // Only log if there are clients and failures
    if (failCount > 0) {
        console.log(`[BROADCAST] Sent to ${successCount}/${browserClients.size} clients (${failCount} failed)`);
    }
}

// Send status update to source stats client(s)
function sendSourceStatsStatusUpdate(client = null) {
    const statusMessage = JSON.stringify({
        type: 'status',
        esp32Connected: esp32SourceStatsConnected,
        esp32Ip: config.esp32.ip,
        timestamp: Date.now()
    });

    if (client) {
        // Send to specific client
        if (client.readyState === WebSocket.OPEN) {
            client.send(statusMessage);
        }
    } else {
        // Broadcast to all source stats clients
        broadcastToSourceStatsClients(statusMessage);
    }
}

// Broadcast message to all connected source stats browser clients
function broadcastToSourceStatsClients(message) {
    let successCount = 0;
    let failCount = 0;

    sourceStatsClients.forEach((client) => {
        if (client.readyState === WebSocket.OPEN) {
            try {
                client.send(message);
                successCount++;
            } catch (error) {
                console.error('[SOURCE-STATS] Failed to send to client:', error.message);
                failCount++;
            }
        } else {
            failCount++;
        }
    });

    // Only log if there are clients and failures
    if (failCount > 0) {
        console.log(`[SOURCE-STATS] Sent to ${successCount}/${sourceStatsClients.size} clients (${failCount} failed)`);
    }
}

// Connect to ESP32 WebSocket
function connectToESP32() {
    if (esp32Client) {
        // Close existing connection
        esp32Client.terminate();
        esp32Client = null;
    }

    const esp32Url = `ws://${config.esp32.ip}:${config.esp32.port}${config.esp32.wsPath}`;
    console.log(`[ESP32] Connecting to ${esp32Url}...`);

    try {
        esp32Client = new WebSocket(esp32Url, {
            handshakeTimeout: 5000,
            perMessageDeflate: false
        });

        esp32Client.on('open', () => {
            esp32Connected = true;
            console.log(`[ESP32] ✓ Connected to ${config.esp32.ip}`);

            // Notify all browser clients
            sendStatusUpdate();
        });

        esp32Client.on('message', (data) => {
            lastMessageTime = Date.now();

            // Relay message to all browser clients
            broadcastToBrowsers(data.toString());
        });

        esp32Client.on('close', () => {
            esp32Connected = false;
            console.log('[ESP32] ✗ Connection closed');

            // Notify browser clients
            sendStatusUpdate();

            // Schedule reconnect
            scheduleReconnect();
        });

        esp32Client.on('error', (error) => {
            console.error('[ESP32] Connection error:', error.message);
            esp32Connected = false;

            // Don't schedule reconnect here - let 'close' event handle it
        });

    } catch (error) {
        console.error('[ESP32] Failed to create WebSocket:', error.message);
        scheduleReconnect();
    }
}

// Schedule reconnection attempt
function scheduleReconnect() {
    if (reconnectTimer) {
        clearTimeout(reconnectTimer);
    }

    console.log(`[ESP32] Reconnecting in ${config.server.reconnectInterval / 1000} seconds...`);

    reconnectTimer = setTimeout(() => {
        connectToESP32();
    }, config.server.reconnectInterval);
}

// Connect to ESP32 Source Stats WebSocket
function connectToESP32SourceStats() {
    if (esp32SourceStatsClient) {
        // Close existing connection
        esp32SourceStatsClient.terminate();
        esp32SourceStatsClient = null;
    }

    const sourceStatsPath = config.esp32.sourceStatsPath || '/source-stats';
    const esp32Url = `ws://${config.esp32.ip}:${config.esp32.port}${sourceStatsPath}`;
    console.log(`[SOURCE-STATS] Connecting to ${esp32Url}...`);

    try {
        esp32SourceStatsClient = new WebSocket(esp32Url, {
            handshakeTimeout: 5000,
            perMessageDeflate: false
        });

        esp32SourceStatsClient.on('open', () => {
            esp32SourceStatsConnected = true;
            console.log(`[SOURCE-STATS] ✓ Connected to ${config.esp32.ip}`);

            // Notify all browser clients
            sendSourceStatsStatusUpdate();
        });

        esp32SourceStatsClient.on('message', (data) => {
            lastSourceStatsMessageTime = Date.now();

            // Relay message to all browser clients
            broadcastToSourceStatsClients(data.toString());
        });

        esp32SourceStatsClient.on('close', () => {
            esp32SourceStatsConnected = false;
            console.log('[SOURCE-STATS] ✗ Connection closed');

            // Notify browser clients
            sendSourceStatsStatusUpdate();

            // Schedule reconnect
            scheduleSourceStatsReconnect();
        });

        esp32SourceStatsClient.on('error', (error) => {
            console.error('[SOURCE-STATS] Connection error:', error.message);
            esp32SourceStatsConnected = false;

            // Don't schedule reconnect here - let 'close' event handle it
        });

    } catch (error) {
        console.error('[SOURCE-STATS] Failed to create WebSocket:', error.message);
        scheduleSourceStatsReconnect();
    }
}

// Schedule source stats reconnection attempt
function scheduleSourceStatsReconnect() {
    if (reconnectSourceStatsTimer) {
        clearTimeout(reconnectSourceStatsTimer);
    }

    console.log(`[SOURCE-STATS] Reconnecting in ${config.server.reconnectInterval / 1000} seconds...`);

    reconnectSourceStatsTimer = setTimeout(() => {
        connectToESP32SourceStats();
    }, config.server.reconnectInterval);
}

// Graceful shutdown
function shutdown() {
    console.log('\n[SERVER] Shutting down gracefully...');

    // Clear reconnect timers
    if (reconnectTimer) {
        clearTimeout(reconnectTimer);
    }
    if (reconnectSourceStatsTimer) {
        clearTimeout(reconnectSourceStatsTimer);
    }

    // Close ESP32 connections
    if (esp32Client) {
        esp32Client.close();
    }
    if (esp32SourceStatsClient) {
        esp32SourceStatsClient.close();
    }

    // Close all browser connections
    browserClients.forEach((client) => {
        client.close(1000, 'Server shutting down');
    });
    sourceStatsClients.forEach((client) => {
        client.close(1000, 'Server shutting down');
    });

    // Close HTTP server
    server.close(() => {
        console.log('[SERVER] HTTP server closed');
        process.exit(0);
    });

    // Force exit after 5 seconds if graceful shutdown fails
    setTimeout(() => {
        console.error('[SERVER] Forced shutdown after timeout');
        process.exit(1);
    }, 5000);
}

// Handle process signals
process.on('SIGINT', shutdown);
process.on('SIGTERM', shutdown);

// Start HTTP server
server.listen(config.server.port, () => {
    console.log('\n┌─────────────────────────────────────────────────┐');
    console.log('│   BoatData WebSocket Proxy Server              │');
    console.log('└─────────────────────────────────────────────────┘');
    console.log(`\n[SERVER] HTTP server listening on port ${config.server.port}`);
    console.log(`[SERVER] BoatData Dashboard: http://localhost:${config.server.port}/stream.html`);
    console.log(`[SERVER] Source Stats Dashboard: http://localhost:${config.server.port}/sources.html`);
    console.log(`[SERVER] WebSocket /boatdata: ws://localhost:${config.server.port}/boatdata`);
    console.log(`[SERVER] WebSocket /source-stats: ws://localhost:${config.server.port}/source-stats`);
    console.log(`[ESP32]  Target: ${config.esp32.ip}:${config.esp32.port}`);
    console.log(`[ESP32]  - BoatData: ${config.esp32.wsPath}`);
    console.log(`[ESP32]  - Source Stats: ${config.esp32.sourceStatsPath || '/source-stats'}\n`);

    // Connect to ESP32 endpoints
    connectToESP32();
    connectToESP32SourceStats();
});

// Handle server errors
server.on('error', (error) => {
    if (error.code === 'EADDRINUSE') {
        console.error(`\n✗ Port ${config.server.port} is already in use. Try a different port:\n`);
        console.error(`  PORT=3001 node server.js\n`);
    } else {
        console.error('[SERVER] Error:', error.message);
    }
    process.exit(1);
});
