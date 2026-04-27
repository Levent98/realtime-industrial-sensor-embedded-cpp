#ifndef CONFIG_MANAGER_H
#define CONFIG_MANAGER_H

#include "sensortypes.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define CONFIG_FLASH_ADDR  0x08004000
#define CONFIG_MAGIC_NUM   0xABCD1234

extern volatile uint8_t SaveDevSettings;
extern volatile uint8_t SaveSensorInfo;

#pragma pack(push, 1)

typedef struct {
    uint8_t  address;
    uint8_t  multidrop_mode;
    uint16_t cnfg_chng_cntr;
    uint8_t  tagDescDate[21];
    uint8_t  assmbly_number[3];
    uint8_t  long_tag[32];
    uint8_t  message[24];
    uint16_t dmy1;
    uint16_t signature;
} __attribute__((packed)) hart_device_stgn;

typedef struct __attribute__((packed, aligned(4)))
{
    uint8_t  dmy_fill[8];
    uint32_t verify_key;
    uint16_t id;
    uint16_t hart_config;
    uint8_t  password[4];
    uint8_t  base_ser_num[8];
    uint8_t  location[32];
    uint16_t display;
    uint16_t config_reg;
    uint8_t  analog_out_lev[8];
    uint16_t alarm_threshold[4];
    uint8_t  alarm_off_hysteresis[4];
    uint8_t  alarm_settings[4];
    uint8_t  relay_off_delay[4];
    uint16_t man_date_year;
    uint16_t man_date_month;
    uint32_t baudrate;
    hart_device_stgn hart;
    uint16_t dmy1;
    uint16_t setting_crc;
} SettingsStruct;

typedef struct {
    uint16_t model;
    uint8_t model_indx;
    uint8_t dmy1;
    char* model_name;
} __attribute__((packed)) XtmrMedelStruct;

#pragma pack(pop)

typedef SettingsStruct DeviceConfig_t;

extern DeviceConfig_t DeviceConfig;
extern XtmrMedelStruct XtmrModel;

void Config_Init(void);
void Config_LoadFromFlash(void);
uint8_t Config_SaveToFlash(void);
void Config_SetDefaults(void);

uint8_t Process_BaseSettings(uint16_t* data, uint8_t len);
uint8_t Process_SensorSettings(uint16_t* data, uint8_t len);

void Config_QuickSave(void);
void Internal_Process_Base(uint16_t* data);
void Internal_Process_Sensor(uint16_t* data, uint16_t start_addr, uint16_t qty);

uint16_t GetSensorModel(uint16_t indx);
uint16_t CheckSensorType(uint16_t type);

#ifdef __cplusplus
}
#endif

#endif