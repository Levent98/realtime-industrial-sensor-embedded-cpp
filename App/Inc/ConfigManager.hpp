#ifndef CONFIG_MANAGER_HPP
#define CONFIG_MANAGER_HPP

#include "FlashStorage.hpp"

extern "C" {
#include "Config_Manager.h"
}

class ConfigManager {
public:
    explicit ConfigManager(FlashStorage& flash);

    const SettingsStruct& loadOrDefaults();

    bool save();
    bool quickSave();

    SettingsStruct& data();
    const SettingsStruct& data() const;

    void loadDefaults();

private:
    FlashStorage& flash_;
    SettingsStruct config_;
};

#endif
