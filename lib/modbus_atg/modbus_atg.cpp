#include "modbus_atg.h"

const char *ModbusATG::TAG = "MDB_ATG";

/* PUBLIC METHODS */
ModbusATG::ModbusATG(HardwareSerial *serial) : _serial(serial)
{
}

ModbusATG::~ModbusATG()
{
    delete _modbus;
}

bool ModbusATG::begin(uint32_t baud, uint8_t pinRX, uint8_t pinTX)
{
    if (_serial == nullptr)
    {
        ESP_LOGE(TAG, "Serial is uninitialized");
        return false;
    }

    _modbus = new ModbusRTUMaster(*_serial);
    if (_modbus == nullptr)
    {
        ESP_LOGE(TAG, "modbus is uninitialized");
        return false;
    }

    _baudRates = baud;
    _pinRX     = pinRX;
    _pinTX     = pinTX;

    _modbus->begin(baud, SERIAL_8N1, pinRX, pinTX, false);

    return true;
}

bool ModbusATG::readAllRegisters(const uint8_t slaveAddr, TankData_t &data)
{
    const size_t   bufSize      = 7;
    uint16_t       buf[bufSize] = {0};
    const uint16_t registerAddr = static_cast<uint16_t>(FuelTankUnitReg_t::ID);

    bool retVal = _modbus->readHoldingRegisters(slaveAddr, registerAddr, buf, bufSize, true);
    // Request to the holding registers
    if (!retVal)
    {
        ESP_LOGE(TAG, "Failed to read all holding registers. [%d]", _modbus->getExceptionResponse());
        return retVal;
    }

    // get ID
    data.ID = static_cast<uint8_t>(buf[0]);

    // get Status
    data.status = static_cast<Status_t>(buf[1]);

    // get Output Voltage
    data.outputVoltage = static_cast<uint16_t>(buf[2]);

    // get Level Tank
    data.percentTankLevel = static_cast<float>(buf[3] / 100.0f);

    // get Minimum Level Voltage
    data.minVoltageLevel = static_cast<float>(buf[4] / 100.0f);

    // get Maximum Level Voltage
    data.maxVoltageLevel = static_cast<float>(buf[5] / 100.0f);

    // get Maximum Tank Volume
    data.maxTankVolume = static_cast<float>(buf[6] / 10.0f);

    return retVal;
}

bool ModbusATG::writeAllRegisters(const uint8_t slaveAddr, TankData_t &data)
{
    bool         retVal  = true;
    const size_t bufSize = 7;
    uint16_t     buf[bufSize];

    // write ID
    buf[0] = static_cast<uint16_t>(data.ID);

    // WARN: skipping buffer[1:3] since it's read only???

    // write minimum voltage level
    buf[4] = static_cast<uint16_t>(data.minVoltageLevel * 100);

    // write maximum voltage level
    buf[5] = static_cast<uint16_t>(data.maxVoltageLevel * 100);

    // write Tank Volume
    buf[6] = static_cast<uint16_t>(data.maxTankVolume * 10);

    const uint16_t registerAddr = static_cast<uint16_t>(FuelTankUnitReg_t::ID);

    retVal = _modbus->writeMultipleHoldingRegisters(slaveAddr, registerAddr, buf, bufSize, true, TYPEDATA_UINT16_T);
    if (!retVal)
    {
        ESP_LOGE(TAG, "Failed to write all holding registers");
        retVal = false;
        return retVal;
    }

    return retVal;
}

bool ModbusATG::setIDDevice(const uint8_t slaveAddr, const uint8_t newID)
{
    bool           retVal       = true;
    const uint16_t registerAddr = static_cast<uint16_t>(FuelTankUnitReg_t::ID);

    retVal = _modbus->writeSingleHoldingRegister(slaveAddr, registerAddr, newID, true, TYPEDATA_UINT16_T);
    if (!retVal)
    {
        ESP_LOGE(TAG, "Failed to write to ID Register");
        return retVal;
    }

    return retVal;
}

bool ModbusATG::setMinVoltageLevel(const uint8_t slaveAddr, const float minVoltageLevel)
{
    bool retVal = true;

    const uint16_t value        = static_cast<uint16_t>(minVoltageLevel * 100);
    const uint16_t registerAddr = static_cast<uint16_t>(FuelTankUnitReg_t::LEVEL_VOLTAGE_MIN);

    retVal = _modbus->writeSingleHoldingRegister(slaveAddr, registerAddr, value, true, TYPEDATA_UINT16_T);
    if (!retVal)
    {
        ESP_LOGE(TAG, "Failed to write to Minimum Voltage Level Register");
        retVal = false;
        return retVal;
    }

    return retVal;
}

bool ModbusATG::setMaxVoltageLevel(const uint8_t slaveAddr, const float maxVoltageLevel)
{
    bool retVal = true;

    const uint16_t value        = static_cast<uint16_t>(maxVoltageLevel * 100);
    const uint16_t registerAddr = static_cast<uint16_t>(FuelTankUnitReg_t::LEVEL_VOLTAGE_MAX);

    retVal = _modbus->writeSingleHoldingRegister(slaveAddr, registerAddr, value, true, TYPEDATA_UINT16_T);
    if (!retVal)
    {
        ESP_LOGE(TAG, "Failed to write to Maximum Voltage Level Register");
        retVal = false;
        return retVal;
    }

    return retVal;
}

bool ModbusATG::setMaxTankVolume(const uint8_t slaveAddr, const float maxTankVolume)
{
    bool retVal = true;

    const uint16_t value        = static_cast<uint16_t>(maxTankVolume * 100);
    const uint16_t registerAddr = static_cast<uint16_t>(FuelTankUnitReg_t::LEVEL_VOLTAGE_MAX);

    retVal = _modbus->writeSingleHoldingRegister(slaveAddr, registerAddr, value, true, TYPEDATA_UINT16_T);
    if (!retVal)
    {
        ESP_LOGE(TAG, "Failed to write to Tank Volume Register");
        return retVal;
    }

    return retVal;
}

bool ModbusATG::getIDDevice(const uint8_t slaveAddr, uint8_t &ID)
{
    bool         retVal       = true;
    const size_t bufSize      = 1;
    uint16_t     buf[bufSize] = {0};
    // const uint16_t registerAddr = static_cast<uint16_t>(FuelTankUnitReg_t::ID);
    const uint16_t registerAddr = 0x1902;

    retVal = _modbus->readHoldingRegisters(slaveAddr, registerAddr, buf, bufSize, true);
    if (!retVal)
    {
        ESP_LOGE(TAG, "Failed to read to ID Register");
        return retVal;
    }

    ID = static_cast<uint8_t>(buf[0]);

    ESP_LOGD(TAG, "id: %d", ID);

    if (buf[0] <= 0 || buf[0] >= 255)
    {
        ESP_LOGE(TAG, "Wrong value of ID: %d", ID);
        retVal = false;
        return retVal;
    }

    return retVal;
}

bool ModbusATG::getStatus(const uint8_t slaveAddr, Status_t &status)
{
    bool           retVal  = true;
    const size_t   bufSize = 1;
    uint16_t       buf[bufSize];
    const uint16_t registerAddr = static_cast<uint16_t>(FuelTankUnitReg_t::STATUS);

    retVal = _modbus->readHoldingRegisters(slaveAddr, registerAddr, buf, bufSize, true);
    if (!retVal)
    {
        ESP_LOGE(TAG, "Failed to read to Status Register");
        retVal = false;
        return retVal;
    }

    status = static_cast<Status_t>(buf[0]);

    ESP_LOGD(TAG, "status: %d", buf[0]);

    return retVal;
}

bool ModbusATG::getOutputVoltage(const uint8_t slaveAddr, float &voltageOutput)
{
    bool           retVal  = true;
    const size_t   bufSize = 1;
    uint16_t       buf[bufSize];
    const uint16_t registerAddr = static_cast<uint16_t>(FuelTankUnitReg_t::OUTPUT_VOLTAGE);

    retVal = _modbus->readHoldingRegisters(slaveAddr, registerAddr, buf, bufSize, true);
    if (!retVal)
    {
        ESP_LOGE(TAG, "Failed to read to Output Voltage Register");
        retVal = false;
        return retVal;
    }

    voltageOutput = static_cast<float>(buf[0] / 100.0f);

    ESP_LOGD(TAG, "voltage output: %.2f", voltageOutput);

    return retVal;
}

bool ModbusATG::getTankLevel(const uint8_t slaveAddr, float &levelTank)
{
    bool           retVal  = true;
    const size_t   bufSize = 1;
    uint16_t       buf[bufSize];
    const uint16_t registerAddr = static_cast<uint16_t>(FuelTankUnitReg_t::LEVEL_TANK);

    retVal = _modbus->readHoldingRegisters(slaveAddr, registerAddr, buf, bufSize, true);
    if (!retVal)
    {
        ESP_LOGE(TAG, "Failed to read to Level Tank Register");
        retVal = false;
        return retVal;
    }

    levelTank = static_cast<float>(buf[0] / 100.0f);

    ESP_LOGD(TAG, "level tank: %.2f", level tank);

    return retVal;
}

bool ModbusATG::getMinVoltageLevel(const uint8_t slaveAddr, float &minVoltageLevel)
{
    bool          retVal  = true;
    const uint8_t bufSize = 1;
    // uint16_t       buf[bufSize];
    uint16_t       buf;
    const uint16_t registerAddr = static_cast<uint16_t>(FuelTankUnitReg_t::LEVEL_VOLTAGE_MIN);

    retVal = _modbus->readHoldingRegisters(slaveAddr, registerAddr, &buf, bufSize, true);
    if (!retVal)
    {
        ESP_LOGE(TAG, "Failed to read to minimum voltage level Register");
        ESP_LOGE(TAG, "Exception: %d", _modbus->getExceptionResponse());
        return retVal;
    }

    minVoltageLevel = static_cast<float>(buf / 100.0f);

    ESP_LOGE(TAG, "minVoltageLevel : %.2f", minVoltageLevel);

    return retVal;
}

bool ModbusATG::getMaxVoltageLevel(const uint8_t slaveAddr, float &maxVoltageLevel)
{
    bool           retVal  = true;
    const size_t   bufSize = 1;
    uint16_t       buf[bufSize];
    const uint16_t registerAddr = static_cast<uint16_t>(FuelTankUnitReg_t::LEVEL_VOLTAGE_MAX);

    retVal = _modbus->readHoldingRegisters(slaveAddr, registerAddr, buf, bufSize, true);
    if (!retVal)
    {
        ESP_LOGE(TAG, "Failed to read to maximum voltage level Register");
        retVal = false;
        return retVal;
    }

    maxVoltageLevel = static_cast<float>(buf[0] / 100.0f);

    ESP_LOGD(TAG, "minVoltageLevel : %.2f", maxVoltageLevel);

    return retVal;
}

bool ModbusATG::getMaxTankVolume(const uint8_t slaveAddr, float &maxTankVolume)
{
    bool           retVal  = true;
    const size_t   bufSize = 1;
    uint16_t       buf[bufSize];
    const uint16_t registerAddr = static_cast<uint16_t>(FuelTankUnitReg_t::TANK_VOLUME_MAX);

    retVal = _modbus->readHoldingRegisters(slaveAddr, registerAddr, buf, bufSize, true);
    if (!retVal)
    {
        ESP_LOGE(TAG, "Failed to read to Maximum Tank Volume Register");
        retVal = false;
        return retVal;
    }

    maxTankVolume = static_cast<float>(buf[0] / 100.0f);

    ESP_LOGD(TAG, "minVoltageLevel : %.2f", maxTankVolume);

    return retVal;
}

void ModbusATG::debugOutput()
{
    Serial.println("[DEBUG outside]");
    while (_serial->available())
    {

        char c = _serial->read();
        Serial.printf("%02x ", c);
    }
    Serial.println();
}
