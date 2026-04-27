#include "ConfigManager.hpp"
#include <string.h>

ConfigManager::ConfigManager(FlashStorage& flash)
    : flash_(flash)
{
    (void)flash_;
    memset(&config_, 0, sizeof(config_));
}

const SettingsStruct& ConfigManager::loadOrDefaults()
{
    Config_LoadFromFlash();
    config_ = DeviceConfig;
    return config_;
}

bool ConfigManager::save()
{
    DeviceConfig = config_;
    return (Config_SaveToFlash() != 0U);
}

bool ConfigManager::quickSave()
{
    DeviceConfig = config_;
    Config_QuickSave();
    config_ = DeviceConfig;
    return true;
}

SettingsStruct& ConfigManager::data()
{
    return config_;
}

const SettingsStruct& ConfigManager::data() const
{
    return config_;
}

void ConfigManager::loadDefaults()
{
    memset(&DeviceConfig, 0, sizeof(DeviceConfig));
    DeviceConfig.id = 1U;
    DeviceConfig.baudrate = 9600U;
    DeviceConfig.verify_key = CONFIG_MAGIC_NUM;
    Config_QuickSave();
    config_ = DeviceConfig;
}