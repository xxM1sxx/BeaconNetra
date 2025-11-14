#pragma once

// #include "../ModbusRTUMaster-1.0.5/src/ModbusRTUMaster.h"
#include "../ModbusRTU-master/src/ModbusRTUMaster.h"
#include <Arduino.h>
#include <esp_err.h>

#define ATG_REQUEST_DURATION (1 * 1000)

class ModbusATG;
extern ModbusATG mdbAtg;

enum class FuelTankUnitReg_t : uint16_t
{
    ID = 0x1900,
    STATUS,
    OUTPUT_VOLTAGE,
    LEVEL_TANK,
    LEVEL_VOLTAGE_MIN,
    LEVEL_VOLTAGE_MAX,
    TANK_VOLUME_MAX,
};

enum class Status_t : uint8_t
{
    OK = 0,
    DISCONNECT,
    DATA_INVALID,
    UNINITIALIZED_CONFIG,
};

struct TankData_t
{
    uint8_t  ID;
    Status_t status;
    float    outputVoltage;
    float    percentTankLevel;
    float    minVoltageLevel;
    float    maxVoltageLevel;
    float    maxTankVolume;
};

class ModbusATG
{
  public:
    ModbusATG(HardwareSerial *serial);
    ~ModbusATG();

    bool begin(uint16_t baud, uint8_t pinRX, uint8_t pinTX);
    bool readAllRegisters(const uint8_t slaveAddr, TankData_t &data);
    bool writeAllRegisters(const uint8_t slaveAddr, TankData_t &data);
    bool setIDDevice(const uint8_t slaveAddr, const uint8_t newID);
    bool getIDDevice(const uint8_t slaveAddr, uint8_t &ID);
    bool getStatus(const uint8_t slaveAddr, Status_t &status);
    bool getOutputVoltage(const uint8_t slaveAddr, float &voltageOutput);
    bool getTankLevel(const uint8_t slaveAddr, float &levelTank);
    bool setMinVoltageLevel(const uint8_t slaveAddr, const float minVoltageLevel);
    bool getMinVoltageLevel(const uint8_t slaveAddr, float &minVoltageLevel);
    bool setMaxVoltageLevel(const uint8_t slaveAddr, const float maxVoltageLevel);
    bool getMaxVoltageLevel(const uint8_t slaveAddr, float &maxVoltageLevel);
    bool setMaxTankVolume(const uint8_t slaveAddr, const float maxTankVolume);
    bool getMaxTankVolume(const uint8_t slaveAddr, float &maxTankVolume);
    void debugOutput();

    static const char *TAG;

  private:
    HardwareSerial  *_serial;
    ModbusRTUMaster *_modbus;
    uint16_t         _baudRates;
    uint8_t          _pinRX;
    uint8_t          _pinTX;
};
