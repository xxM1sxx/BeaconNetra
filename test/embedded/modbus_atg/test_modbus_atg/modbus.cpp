#include "modbus_atg.h"
#include <Arduino.h>
#include <unity.h>

#define PIN_RX_RS485 18
#define PIN_TX_RS485 19

// Mock HardwareSerial for testing
HardwareSerial testSerial(1);
ModbusATG      modbusTest(&testSerial);

TankData_t testData;

void setUp(void)
{
    // Reset test data before each test
    memset(&testData, 0, sizeof(TankData_t));
}

void tearDown(void)
{
    // Clean up after each test
}

// Test initialization
void test_modbus_initialization(void)
{
    TEST_ASSERT_TRUE(modbusTest.begin(9600, PIN_RX_RS485, PIN_TX_RS485));
}

// Test readAllRegisters with expected values
void test_read_all_registers(void)
{
    bool result = modbusTest.readAllRegisters(1, testData);

    // Test if function returns successfully
    TEST_ASSERT_TRUE(result);

    // Test if data fields are within expected ranges
    TEST_ASSERT_TRUE(testData.ID >= 0 && testData.ID <= 255);
    uint8_t status = static_cast<uint8_t>(testData.status);
    TEST_ASSERT_TRUE(status >= 0 && status <= 3); // Assuming Status_t enum range

    // Test voltage ranges (adjust based on your device specs)
    TEST_ASSERT_TRUE(testData.outputVoltage >= 0.0f && testData.outputVoltage <= 5.0f);
    TEST_ASSERT_TRUE(testData.percentTankLevel >= 0.0f && testData.percentTankLevel <= 100.0f);
    TEST_ASSERT_TRUE(testData.minVoltageLevel >= 0.0f && testData.minVoltageLevel <= 5.0f);
    TEST_ASSERT_TRUE(testData.maxVoltageLevel >= 0.0f && testData.maxVoltageLevel <= 5.0f);
    TEST_ASSERT_TRUE(testData.maxTankVolume >= 0.0f && testData.maxTankVolume <= 1000.0f);

    // Test data consistency
    TEST_ASSERT_TRUE(testData.minVoltageLevel <= testData.maxVoltageLevel);
    TEST_ASSERT_TRUE(testData.percentTankLevel <= 100.0f); // Percentage should be 0-100%
}

// Test individual getter functions
void test_get_id_device(void)
{
    uint8_t deviceId;
    bool    result = modbusTest.getIDDevice(1, deviceId);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_TRUE(deviceId >= 0 && deviceId <= 255);
}

void test_get_status(void)
{
    Status_t statusTemp;
    bool     result = modbusTest.getStatus(1, statusTemp);

    TEST_ASSERT_TRUE(result);
    uint8_t status = static_cast<uint8_t>(statusTemp);
    TEST_ASSERT_TRUE(status >= 0 && status <= 3); // Adjust based on your Status_t enum
}

void test_get_output_voltage(void)
{
    float voltage;
    bool  result = modbusTest.getOutputVoltage(1, voltage);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_TRUE(voltage >= 0.0f && voltage <= 5.0f);
}

void test_get_tank_level(void)
{
    float level;
    bool  result = modbusTest.getTankLevel(1, level);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_TRUE(level >= 0.0f && level <= 100.0f); // Should be percentage
}

void test_get_min_voltage_level(void)
{
    float minVoltage;
    bool  result = modbusTest.getMinVoltageLevel(1, minVoltage);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_TRUE(minVoltage >= 0.0f && minVoltage <= 5.0f);
}

void test_get_max_voltage_level(void)
{
    float maxVoltage;
    bool  result = modbusTest.getMaxVoltageLevel(1, maxVoltage);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_TRUE(maxVoltage >= 0.0f && maxVoltage <= 5.0f);
}

void test_get_max_tank_volume(void)
{
    float maxVolume;
    bool  result = modbusTest.getMaxTankVolume(1, maxVolume);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_TRUE(maxVolume >= 0.0f && maxVolume <= 1000.0f);
}

// Test setter functions (read back to verify)
void test_set_get_min_voltage(void)
{
    float originalVoltage, setVoltage, readVoltage;

    // Get original value
    TEST_ASSERT_TRUE(modbusTest.getMinVoltageLevel(1, originalVoltage));

    // Set new value (use a safe test value within device range)
    setVoltage = 1.5f;
    TEST_ASSERT_TRUE(modbusTest.setMinVoltageLevel(1, setVoltage));

    // Read back to verify
    delay(100); // Small delay for device to process
    TEST_ASSERT_TRUE(modbusTest.getMinVoltageLevel(1, readVoltage));

    // Allow small tolerance for floating point comparison and device precision
    TEST_ASSERT_FLOAT_WITHIN(0.1f, setVoltage, readVoltage);

    // Restore original value
    modbusTest.setMinVoltageLevel(1, originalVoltage);
}

void test_set_get_max_voltage(void)
{
    float originalVoltage, setVoltage, readVoltage;

    TEST_ASSERT_TRUE(modbusTest.getMaxVoltageLevel(1, originalVoltage));

    setVoltage = 3.5f;
    TEST_ASSERT_TRUE(modbusTest.setMaxVoltageLevel(1, setVoltage));

    delay(100);
    TEST_ASSERT_TRUE(modbusTest.getMaxVoltageLevel(1, readVoltage));
    TEST_ASSERT_FLOAT_WITHIN(0.1f, setVoltage, readVoltage);

    // Restore original
    modbusTest.setMaxVoltageLevel(1, originalVoltage);
}

void test_set_get_max_tank_volume(void)
{
    float originalVolume, setVolume, readVolume;

    TEST_ASSERT_TRUE(modbusTest.getMaxTankVolume(1, originalVolume));

    setVolume = 500.0f;
    TEST_ASSERT_TRUE(modbusTest.setMaxTankVolume(1, setVolume));

    delay(100);
    TEST_ASSERT_TRUE(modbusTest.getMaxTankVolume(1, readVolume));
    TEST_ASSERT_FLOAT_WITHIN(1.0f, setVolume, readVolume);

    // Restore original
    modbusTest.setMaxTankVolume(1, originalVolume);
}

// Test error conditions
void test_invalid_slave_address(void)
{
    TankData_t data;
    bool       result = modbusTest.readAllRegisters(0, data); // Invalid slave address 0

    TEST_ASSERT_FALSE(result); // Should fail with invalid address
}

void test_consistency_between_methods(void)
{
    TankData_t allData;
    float      individualVoltage, individualLevel;

    // Read all registers
    TEST_ASSERT_TRUE(modbusTest.readAllRegisters(1, allData));

    // Read individual values
    TEST_ASSERT_TRUE(modbusTest.getOutputVoltage(1, individualVoltage));
    TEST_ASSERT_TRUE(modbusTest.getTankLevel(1, individualLevel));

    // Compare values - they should match
    TEST_ASSERT_FLOAT_WITHIN(0.01f, allData.outputVoltage, individualVoltage);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, allData.percentTankLevel, individualLevel);
}

// Test data integrity and relationships
void test_tank_data_integrity(void)
{
    bool result = modbusTest.readAllRegisters(1, testData);
    TEST_ASSERT_TRUE(result);

    // Test percentage bounds
    TEST_ASSERT_TRUE(testData.percentTankLevel >= 0.0f && testData.percentTankLevel <= 100.0f);

    // Test voltage relationships
    TEST_ASSERT_TRUE(testData.minVoltageLevel <= testData.maxVoltageLevel);

    // Test that tank level doesn't exceed max volume in terms of percentage concept
    // (percentTankLevel should represent percentage of maxTankVolume)
    if (testData.maxTankVolume > 0)
    {
        float calculatedVolume = (testData.percentTankLevel / 100.0f) * testData.maxTankVolume;
        TEST_ASSERT_TRUE(calculatedVolume <= testData.maxTankVolume);
    }

    // Test that all voltages are positive
    TEST_ASSERT_TRUE(testData.outputVoltage >= 0);
    TEST_ASSERT_TRUE(testData.minVoltageLevel >= 0);
    TEST_ASSERT_TRUE(testData.maxVoltageLevel >= 0);
}

// Test repeated reads for stability
void test_repeated_reads_consistency(void)
{
    TankData_t data1, data2, data3;

    TEST_ASSERT_TRUE(modbusTest.readAllRegisters(1, data1));
    delay(50);
    TEST_ASSERT_TRUE(modbusTest.readAllRegisters(1, data2));
    delay(50);
    TEST_ASSERT_TRUE(modbusTest.readAllRegisters(1, data3));

    // Device ID and status should remain consistent
    TEST_ASSERT_EQUAL(data1.ID, data2.ID);
    TEST_ASSERT_EQUAL(data2.ID, data3.ID);
    TEST_ASSERT_EQUAL(data1.status, data2.status);
    TEST_ASSERT_EQUAL(data2.status, data3.status);

    // Voltages and levels should be reasonably consistent
    TEST_ASSERT_FLOAT_WITHIN(0.5f, data1.outputVoltage, data2.outputVoltage);
    TEST_ASSERT_FLOAT_WITHIN(0.5f, data2.outputVoltage, data3.outputVoltage);
    TEST_ASSERT_FLOAT_WITHIN(2.0f, data1.percentTankLevel, data2.percentTankLevel); // Allow 2% variation
}

void setup()
{
    delay(2000); // Service delay
    UNITY_BEGIN();

    Serial.println("Starting ModbusATG Unit Tests...");

    // Run initialization test first
    RUN_TEST(test_modbus_initialization);
    delay(1000); // Wait for modbus to initialize

    // Run core functionality tests
    RUN_TEST(test_read_all_registers);
    RUN_TEST(test_get_id_device);
    RUN_TEST(test_get_status);
    RUN_TEST(test_get_output_voltage);
    RUN_TEST(test_get_tank_level);
    RUN_TEST(test_get_min_voltage_level);
    RUN_TEST(test_get_max_voltage_level);
    RUN_TEST(test_get_max_tank_volume);

    // Run data integrity tests
    RUN_TEST(test_tank_data_integrity);
    RUN_TEST(test_repeated_reads_consistency);

    // Run setter tests (comment out if you don't want to modify device)
    RUN_TEST(test_set_get_min_voltage);
    RUN_TEST(test_set_get_max_voltage);
    RUN_TEST(test_set_get_max_tank_volume);

    // Run validation tests
    RUN_TEST(test_invalid_slave_address);
    RUN_TEST(test_consistency_between_methods);

    Serial.println("ModbusATG Unit Tests Completed");
    UNITY_END();
}

void loop()
{
    // Empty - tests run once in setup()
    delay(1000);
}
