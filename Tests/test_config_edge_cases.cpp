#include "Config_Manager.h"
#include "appcons.h"
#include <cassert>

int main()
{
    // TC-CONFIG-008 / BT-SW-CONFIG-009: invalid verify key causes default fallback.
    DeviceConfig.verify_key = 0U;
    DeviceConfig.id = 99U;
    Config_LoadFromFlash();
    assert(DeviceConfig.verify_key == CONFIG_MAGIC_NUM);
    assert(DeviceConfig.id == 1U);
    assert(DeviceConfig.baudrate == DEFAULT_MDBS_BAUDRATE);

    // TC-CONFIG-009: valid verify key preserves existing runtime config.
    DeviceConfig.verify_key = CONFIG_MAGIC_NUM;
    DeviceConfig.id = 42U;
    DeviceConfig.baudrate = 38400U;
    Config_LoadFromFlash();
    assert(DeviceConfig.id == 42U);
    assert(DeviceConfig.baudrate == 38400U);

    // TC-CONFIG-010: save operation stamps magic key.
    DeviceConfig.verify_key = 0U;
    assert(Config_SaveToFlash() == 1U);
    assert(DeviceConfig.verify_key == CONFIG_MAGIC_NUM);

    // TC-CONFIG-011: quick-save path uses save operation and keeps magic key valid.
    DeviceConfig.verify_key = 0U;
    Config_QuickSave();
    assert(DeviceConfig.verify_key == CONFIG_MAGIC_NUM);

    // TC-CONFIG-012 / BT-SW-CONFIG-008: base settings writes requested slave ID and save flag.
    uint16_t baseSettings[] = {247U};
    SaveDevSettings = 0U;
    assert(Process_BaseSettings(baseSettings, 1U) == 1U);
    assert(DeviceConfig.id == 247U);
    assert(SaveDevSettings == 1U);

    return 0;
}
