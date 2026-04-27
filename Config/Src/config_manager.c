#include "Config_Manager.h"
#include <string.h>

DeviceConfig_t DeviceConfig;
volatile uint8_t SaveDevSettings = 0U;
volatile uint8_t SaveSensorInfo = 0U;
static uint16_t Config_CalcCRC(const uint8_t* data, uint32_t length)
{
    uint16_t crc = 0xFFFFU;

    for (uint32_t i = 0; i < length; ++i) {
        crc ^= data[i];
        for (uint8_t j = 0; j < 8U; ++j) {
            if (crc & 1U) {
                crc = (crc >> 1U) ^ 0xA001U;
            } else {
                crc >>= 1U;
            }
        }
    }
    return crc;
}

void Config_Init(void)
{
    Config_LoadFromFlash();
}

void Config_SetDefaults(void)
{
    memset(&DeviceConfig, 0, sizeof(DeviceConfig));
    DeviceConfig.verify_key = CONFIG_MAGIC_NUM;
    DeviceConfig.id = 1U;
    DeviceConfig.baudrate = 9600U;
    DeviceConfig.setting_crc =
        Config_CalcCRC((const uint8_t*)&DeviceConfig, sizeof(DeviceConfig) - 2U);
}

void Config_LoadFromFlash(void)
{
    if (DeviceConfig.verify_key != CONFIG_MAGIC_NUM) {
        Config_SetDefaults();
        return;
    }

    {
        uint16_t crc =
            Config_CalcCRC((const uint8_t*)&DeviceConfig, sizeof(DeviceConfig) - 2U);

        if (crc != DeviceConfig.setting_crc) {
            Config_SetDefaults();
        }
    }
}

uint8_t Config_SaveToFlash(void)
{
    DeviceConfig.setting_crc =
        Config_CalcCRC((const uint8_t*)&DeviceConfig, sizeof(DeviceConfig) - 2U);
    return 1U;
}

uint8_t Process_BaseSettings(uint16_t *data, uint8_t len)
{
    (void)len;
    if (data == 0) {
        return 0U;
    }
    DeviceConfig.id = data[0];
    return 1U;
}

uint8_t Process_SensorSettings(uint16_t *data, uint8_t len)
{
    (void)len;
    if (data == 0) {
        return 0U;
    }
    DeviceConfig.alarm_threshold[0] = data[0];
    return 1U;
}

void Config_QuickSave(void)
{
    (void)Config_SaveToFlash();
}

void Internal_Process_Base(uint16_t *data)
{
    (void)data;
}

void Internal_Process_Sensor(uint16_t *data, uint16_t start_addr, uint16_t qty)
{
    (void)data;
    (void)start_addr;
    (void)qty;
}

uint16_t GetSensorModel(uint16_t indx)
{
    return indx;
}

uint16_t CheckSensorType(uint16_t type)
{
    return type;
}