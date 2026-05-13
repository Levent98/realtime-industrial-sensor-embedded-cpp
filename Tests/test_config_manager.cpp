#include "Config_Manager.h"
#include "appcons.h"
#include <cassert>

int main()
{
    // TC-CONFIG-001: Force invalid/default config state before loading.
    DeviceConfig.verify_key = 0U;

    // TC-CONFIG-001: Invalid persistent configuration should fall back to defaults.
    Config_LoadFromFlash();

    assert(DeviceConfig.verify_key == CONFIG_MAGIC_NUM);
    assert(DeviceConfig.id == 1U);
    assert(DeviceConfig.baudrate == DEFAULT_MDBS_BAUDRATE);
    assert(Sensor.mul_factor == MODBUS_SCALING_FACTOR);

    uint16_t baseData[] = {17U};

    SaveDevSettings = 0U;

    // TC-CONFIG-002: Valid base settings update should change runtime device ID.
    assert(Process_BaseSettings(baseData, 1U) == 1U);
    assert(DeviceConfig.id == 17U);

    // TC-CONFIG-002: Valid base settings update should request persistent save.
    assert(SaveDevSettings == 1U);

    // TC-CONFIG-003: Null base settings pointer should be rejected.
    assert(Process_BaseSettings(nullptr, 1U) == 0U);

    uint16_t sensorData[] = {650U};

    SaveSensorInfo = 0U;
    SaveDevSettings = 0U;

    // TC-CONFIG-004: Valid sensor settings update should change alarm threshold.
    assert(Process_SensorSettings(sensorData, 1U) == 1U);
    assert(DeviceConfig.alarm_threshold[0] == 650U);

    // TC-CONFIG-004: Sensor settings update should set save flags.
    assert(SaveSensorInfo == 1U);
    assert(SaveDevSettings == 1U);

    // TC-CONFIG-005: Null sensor settings pointer should be rejected.
    assert(Process_SensorSettings(nullptr, 1U) == 0U);

    // TC-CONFIG-006: Valid sensor configuration should be accepted.
    assert(CheckConctSettings(&Sensor) == 1U);

    // TC-CONFIG-007: Null sensor configuration pointer should be rejected.
    assert(CheckConctSettings(nullptr) == 0U);

    return 0;
}
