#pragma once

#include "common.h"
#include "hour_meter_manager.h"
#include "id_management.h"
#include "modbus_atg.h"
#include <Arduino.h>
#include <ArduinoJson.h>
#include <AsyncJson.h>
#include <ESPAsyncWebServer.h>
#include <LittleFS.h>
#include <NimBLEDevice.h>
#include <Update.h>
#include <WiFi.h>
#include <esp_err.h>

#define SERVICE_OTA_UUID     "6f7e9280-e3d8-47b7-9733-ef0ffffc65fd"
#define CHAR_OTA_UUID        "2233b614-a358-4551-8ad1-b113764c68f3"
#define SERVICE_DIS_UUID     "180A"
#define CHAR_DIS_FW_VER_UUID "2A26"
#define OTA_WIFI_DURATION    (10 * 60 * 1000) // 10 minutes
#define MAX_BEACON_DATA_SIZE 20
#define SERVICE_HM_UPDATE_UUID  "f0a1b2c3-d4e5-4f6a-8b7c-9d0e1f2a3b4c"
#define CHAR_HM_UPDATE_UUID    "a1b2c3d4-e5f6-4a7b-8c9d-0e1f2a3b4c5d"
#define HM_UPDATE_TOKEN        "Qims_HM_BLE"
#define SHARED_SECRET          "FUMS_SECRET_2026"
#define AUTH_TIMEOUT_MS        10000
#define SERVICE_WIFI_CTRL_UUID   "e0b1c2d3-e4f5-4a6b-8c7d-9e0f1a2b3c4d"
#define CHAR_WIFI_CTRL_UUID      "d1c2b3a4-e5f6-4a7b-8c9d-0e1f2a3b4c5e"
#define SERVICE_HM_CTRL_UUID     "b0c1d2e3-f4a5-4b6c-8d7e-9f0a1b2c3d4e"
#define CHAR_HM_CTRL_UUID        "c1d2e3f4-a5b6-4c7d-8e9f-0a1b2c3d4e5f"

extern BeaconData_t data;
extern Setting_t    setting;
extern HourMeter   *hm;
extern TankData_t   tankData;

class BLE
{
  private:
    String      _WIFI_SSID;
    const char *_WIFIpwd;

    String _BT_SSID;

    // For Beacon
    BLEAdvertising *_pAdvertising;

    // For Server
    NimBLEServer         *_pServer;
    NimBLEService        *_pOTAService;
    NimBLEService        *_pDISService;
    NimBLECharacteristic *_pFirmwareChar;
    NimBLECharacteristic *_pCharacteristic;
    NimBLEAdvertising    *_pAdvertisingOTA;
    NimBLEService        *_pHMService;
    NimBLECharacteristic *_pHMUpdateChar;
    NimBLEService        *_pWiFiCtrlService;
    NimBLECharacteristic *_pWiFiCtrlChar;
    NimBLEService        *_pHMCtrlService;
    NimBLECharacteristic *_pHMCtrlChar;
    bool                  _isOTASuccess = false;
    bool                  _isFileBin    = false;

    uint8_t  _currentChallenge[8];
    uint32_t _authStartTime;
    uint16_t _authConnHandle;

    esp_err_t   _updateFirmware();
    time_t      _calculateHMOffsetSeconds(time_t seconds, float offset);
    void        _setNetworkConfig();
    static void _WiFiAPConnectedCB(arduino_event_id_t e);
    static void _WiFiAPDisconnectedCB(arduino_event_id_t e);
    static void _handleSerializingDataJSON(AsyncWebServerRequest *request);

  public:
    BLE();

    enum OTAState
    {
        IDLE = 0,
        OTA_ON
    };

    void        begin();
    void        setCustomBeacon(const BeaconData_t &data, const Setting_t &setting, const TankData_t *tank);
    void        advertiseBeacon();
    void        startServerOTA();
    void        startHTTPServer();
    void        stopHTTPServer();
    void        startWiFi();
    void        stopWiFi();
    bool        connectToWiFi();
    String      getMonitoringData(BeaconData_t &data, Setting_t &setting);
    String      getSettingData(Setting_t &setting);
    void        stopAdvertiseOTA();
    void        setWiFiSSID(String newSSID);
    void        onAuthRequest();
    bool        onAuthResponse(uint8_t *response, uint8_t len);
    void        checkAuthTimeout();
    void        resetAuth(uint16_t conn_handle = 0);
    static void restartTask(void *param);

    static bool     isConnectedToWiFi;
    static bool     isAuthenticated;
    WiFiClass      *_wifi   = nullptr; // For WebServer
    AsyncWebServer *_server = nullptr;

    // Set Static IP
    IPAddress localIP;
    IPAddress gateway;
    IPAddress subnet;
    IPAddress primaryDNS;
    IPAddress secondaryDNS;
};

// Custom BLE Server Callback Class
class OTAServerCallback : public NimBLEServerCallbacks
{
  private:
    BLE *_bleInstance;

  public:
    OTAServerCallback(BLE *bleInstance) : _bleInstance(bleInstance)
    {
    }

    void onConnect(NimBLEServer *pServer, ble_gap_conn_desc *desc) override
    {
        Serial.println("Device is Connected to ESP32 BLE!");
        _bleInstance->resetAuth(desc->conn_handle);

        if (NimBLEDevice::getAdvertising()->isAdvertising())
        {
            NimBLEDevice::getAdvertising()->stop();
            Serial.println("BLE advertising stopped.");
        }
    }

    void onDisconnect(NimBLEServer *pServer) override
    {
        Serial.println("Device is disconnected from ESP32 BLE!");
        _bleInstance->resetAuth();

        NimBLEDevice::getAdvertising()->start();
        Serial.println("BLE advertising restarted.");

        if (_bleInstance->_wifi)
        {
            _bleInstance->_wifi->disconnect(true); // disconnect WiFi
            vTaskDelay(pdMS_TO_TICKS(500));

            // save the new name before disconnect
            _bleInstance->setWiFiSSID(setting.ID);

            _bleInstance->_wifi->mode(WIFI_OFF); // Turn off WiFi
            vTaskDelay(pdMS_TO_TICKS(500));

            delete _bleInstance->_wifi;
            vTaskDelay(pdMS_TO_TICKS(100));
            _bleInstance->_wifi = nullptr;
        }
    }
};

// Custom BLE Callback Class
class OTACharacteristicCallback : public NimBLECharacteristicCallbacks
{
  private:
    BLE *_ble;

  public:
    OTACharacteristicCallback(BLE *ble) : _ble(ble) {}

    void onWrite(NimBLECharacteristic *pCharacteristic) override
    {
        std::string value = pCharacteristic->getValue();
        if (value.empty()) return;

        uint8_t cmd = (uint8_t)value[0];

        if (!BLE::isAuthenticated)
        {
            if (cmd == 0x19) // AUTH_REQUEST
            {
                _ble->onAuthRequest();
                return;
            }
            if (cmd == 0x18) // AUTH_RESPONSE
            {
                uint8_t *data = (uint8_t *)value.c_str();
                bool     ok   = _ble->onAuthResponse(data + 1, value.size() - 1);
                uint8_t  ack[2] = {0x19, ok ? (uint8_t)0x00 : (uint8_t)0x01};
                pCharacteristic->setValue(ack, 2);
                return;
            }
            Serial.println("[AUTH] Command rejected - not authenticated");
            return;
        }

        Serial.print("[OTA] Received: ");
        Serial.println(value.c_str());
    }
};

class HMUpdateCharacteristicCallback : public NimBLECharacteristicCallbacks
{
    void onWrite(NimBLECharacteristic *pCharacteristic) override
    {
        if (!BLE::isAuthenticated)
        {
            Serial.println("[HM] Rejected - not authenticated");
            return;
        }

        std::string value = pCharacteristic->getValue();
        if (value.empty())
        {
            Serial.println("[HM] Empty write received");
            return;
        }

        String str(value.c_str());
        int sep = str.indexOf(':');
        if (sep < 0)
        {
            Serial.println("[HM] Invalid format - use TOKEN:hours");
            return;
        }

        String token = str.substring(0, sep);
        if (token != HM_UPDATE_TOKEN)
        {
            Serial.println("[HM] Unauthorized update attempt - token mismatch");
            return;
        }

        String valStr = str.substring(sep + 1);
        float hours = valStr.toFloat();
        if (hours < 0.0f || hours > 100000.0f)
        {
            Serial.printf("[HM] Value out of range: %.2f\n", hours);
            return;
        }

        time_t seconds = (time_t)(hours * 3600.0f);
        data.hourMeter = seconds;
        hm->saveToStorage(data.hourMeter);

        Serial.printf("[HM] Updated via BLE: %.2f hours (%ld seconds)\n", hours, seconds);
    }
};

class WiFiControlCallback : public NimBLECharacteristicCallbacks
{
  private:
    BLE *_bleInstance;

  public:
    WiFiControlCallback(BLE *bleInstance) : _bleInstance(bleInstance)
    {
    }

    void onWrite(NimBLECharacteristic *pCharacteristic) override
    {
        if (!BLE::isAuthenticated)
        {
            Serial.println("[WiFi] Rejected - not authenticated");
            return;
        }

        std::string value = pCharacteristic->getValue();
        if (value.empty())
        {
            Serial.println("[WiFi] Empty write received");
            return;
        }

        String cmd(value.c_str());
        cmd.trim();

        if (cmd == "WIFI_ON")
        {
            Serial.println("[WiFi] Command: WIFI_ON - Starting WiFi AP + HTTP Server");
            _bleInstance->startWiFi();
            _bleInstance->startHTTPServer();
        }
        else if (cmd == "WIFI_OFF")
        {
            Serial.println("[WiFi] Command: WIFI_OFF - Stopping HTTP Server + WiFi AP");
            _bleInstance->stopHTTPServer();
            _bleInstance->stopWiFi();
        }
        else
        {
            Serial.printf("[WiFi] Unknown command: %s\n", cmd.c_str());
        }
    }
};

class HMControlCallback : public NimBLECharacteristicCallbacks
{
    void onWrite(NimBLECharacteristic *pCharacteristic) override
    {
        if (!BLE::isAuthenticated)
        {
            Serial.println("[HM CTRL] Rejected - not authenticated");
            return;
        }

        std::string value = pCharacteristic->getValue();
        if (value.empty())
        {
            Serial.println("[HM CTRL] Empty write received");
            return;
        }

        String cmd(value.c_str());
        cmd.trim();

        if (cmd == "HM_START")
        {
            hmBLETriggered = true;
            Serial.println("[HM CTRL] BLE trigger: HM counting STARTED");
        }
        else if (cmd == "HM_STOP")
        {
            hmBLETriggered = false;
            Serial.println("[HM CTRL] BLE trigger: HM counting STOPPED");
        }
        else
        {
            Serial.printf("[HM CTRL] Unknown command: %s\n", cmd.c_str());
        }
    }
};
