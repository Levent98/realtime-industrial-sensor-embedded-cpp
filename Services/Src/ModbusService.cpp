#include "ModbusService.hpp"
#include "main.hpp"

#ifndef UNIT_TEST
extern "C" {
#include "stm32f410rx.h"
#include "core_cm4.h"
}
#else
extern "C" uint32_t GetTime_us(void);

#define __disable_irq() ((void)0)
#define __enable_irq()  ((void)0)
#endif

extern "C" {
#include "Config_Manager.h"
#include "appcons.h"
#include "apptdef.h"
#include "sensortypes.h"

#ifndef UNIT_TEST
#include "delay.h"
#endif
}

#include <cstring>

extern volatile uint16_t baud_rate;

namespace {
constexpr uint16_t REG_DIAG_CRC_ERRORS          = 0x74U;
constexpr uint16_t REG_DIAG_UART_OVERFLOWS      = 0x75U;
constexpr uint16_t REG_DIAG_SENSOR_READ_FAILS   = 0x76U;
constexpr uint16_t REG_DIAG_I2C_RECOVERIES      = 0x77U;
constexpr uint16_t REG_DIAG_WATCHDOG_RESETS     = 0x78U;
constexpr uint16_t REG_DIAG_FLASH_SAVE_FAILS    = 0x79U;
constexpr uint16_t REG_DIAG_HEALTH_STATE        = 0x7AU;
constexpr uint16_t REG_DIAG_CONSECUTIVE_FAULTS  = 0x7BU;
}

ModbusService::ModbusService(UartDriver& uart, ErrorManager& errors)
    : uart_(uart),
      errors_(errors),
      slaveAddress_(1U),
      tempVal_(250),
      humVal_(600)
{
    std::memset(holdingRegs_, 0, sizeof(holdingRegs_));
    std::memset(inputRegs_, 0, sizeof(inputRegs_));
    std::memset(&frameQueue_, 0, sizeof(frameQueue_));
    std::memset(rxScratch_, 0, sizeof(rxScratch_));
    std::memset(responseScratch_, 0, sizeof(responseScratch_));
    std::memset(&frameScratch_, 0, sizeof(frameScratch_));
    std::memset(&sensorScratch_, 0, sizeof(sensorScratch_));
    std::memset(alarmScratch_, 0, sizeof(alarmScratch_));
    std::memset(alarmThresholdScratch_, 0, sizeof(alarmThresholdScratch_));
}

void ModbusService::initialize(uint8_t slaveAddress)
{
    slaveAddress_ = slaveAddress;

    std::memset(holdingRegs_, 0, sizeof(holdingRegs_));
    std::memset(inputRegs_, 0, sizeof(inputRegs_));
    std::memset(&frameQueue_, 0, sizeof(frameQueue_));

    Config_LoadFromFlash();
    syncAll();
}

void ModbusService::process()
{
    if (uart_.frameAvailable()) {
        const std::size_t len = uart_.fetchFrame(rxScratch_, sizeof(rxScratch_));

        if (len > 0U) {
            queueFrame(rxScratch_, static_cast<uint16_t>(len));
        }
    }

    if (dequeueFrame(&frameScratch_) != 0U) {
        (void)processFrame(&frameScratch_);
    }
}

[[nodiscard]] uint16_t ModbusService::crc16(const uint8_t* buffer,
                                            uint16_t len)
{
    constexpr uint16_t InitialCrc = 0xFFFFU;
    constexpr uint16_t Polynomial = 0xA001U;

    uint16_t crc = InitialCrc;

    for (uint16_t i = 0U; i < len; ++i) {
        crc ^= static_cast<uint16_t>(buffer[i]);

        for (uint8_t j = 0U; j < 8U; ++j) {
            if ((crc & 1U) != 0U) {
                crc = static_cast<uint16_t>((crc >> 1U) ^ Polynomial);
            } else {
                crc >>= 1U;
            }
        }
    }

    return crc;
}

void ModbusService::setHoldingRegister(uint16_t reg, uint16_t value)
{
    setHoldingRegisterInternal(reg, value);
}

uint16_t ModbusService::holdingRegister(uint16_t reg) const
{
    return getHoldingRegisterInternal(reg);
}

void ModbusService::setInputRegister(uint16_t reg, uint16_t value)
{
    setInputRegisterInternal(reg, value);
}

uint16_t ModbusService::inputRegister(uint16_t reg) const
{
    return getInputRegisterInternal(reg);
}

void ModbusService::setHoldingRegisterInternal(uint16_t reg, uint16_t value)
{
    if (reg < HOLDING_REG_SIZE) {
        holdingRegs_[reg] = value;
    }
}

uint16_t ModbusService::getHoldingRegisterInternal(uint16_t reg) const
{
    if (reg < HOLDING_REG_SIZE) {
        if (reg == REG_BAUD_RATE) {
            return static_cast<uint16_t>(baud_rate / 100U);
        }

        return holdingRegs_[reg];
    }

    return 0U;
}

void ModbusService::setInputRegisterInternal(uint16_t reg, uint16_t value)
{
    if (reg < InputRegSize) {
        inputRegs_[reg] = value;
    }
}

uint16_t ModbusService::getInputRegisterInternal(uint16_t reg) const
{
    return (reg < InputRegSize) ? inputRegs_[reg] : 0U;
}

void ModbusService::syncAll()
{
    uint16_t val;

    holdingRegs_[REG_TYPE_REG]   = 0x0021U;
    holdingRegs_[REG_DET_MODEL]  = 0x5202U;
    holdingRegs_[REG_FW_VERSION] = 0x0103U;
    holdingRegs_[REG_FW_DATE]    = static_cast<uint16_t>((26U << 8U) | 4U);
	  holdingRegs_[REG_FW_DATE] =
    static_cast<uint16_t>(((BUILD_YEAR & 0xFFU) << 8U) |
                          (BUILD_MONTH  & 0xFFU));

    holdingRegs_[REG_FW_DATE_2] =
    static_cast<uint16_t>((BUILD_DAY & 0xFFU) << 8U);
//    holdingRegs_[REG_FW_DATE_2]  = 0x0100U;

    inputRegs_[REG_TYPE_REG]   = holdingRegs_[REG_TYPE_REG];
    inputRegs_[REG_DET_MODEL]  = holdingRegs_[REG_DET_MODEL];
    inputRegs_[REG_FW_VERSION] = holdingRegs_[REG_FW_VERSION];
    inputRegs_[REG_FW_DATE]    = holdingRegs_[REG_FW_DATE];
    inputRegs_[REG_FW_DATE_2]  = holdingRegs_[REG_FW_DATE_2];
		
		holdingRegs_[REG_TIME_HOUR_MIN] =
    static_cast<uint16_t>(((BUILD_HOUR & 0xFFU) << 8U) |
                          (BUILD_MIN  & 0xFFU));

    holdingRegs_[REG_TIME_SEC] =
    static_cast<uint16_t>((BUILD_SEC & 0xFFU) << 8U);
		
		inputRegs_[REG_TIME_HOUR_MIN]    = holdingRegs_[REG_TIME_HOUR_MIN];
    inputRegs_[REG_TIME_SEC]  = holdingRegs_[REG_TIME_SEC];
		

    for (uint8_t i = 0U; i < 4U; ++i) {
        val = static_cast<uint16_t>(
            (static_cast<uint16_t>(DeviceConfig.base_ser_num[i * 2U]) << 8U) |
             static_cast<uint16_t>(DeviceConfig.base_ser_num[i * 2U + 1U])
        );

        holdingRegs_[REG_BASE_SERIAL_NO + i] = val;
        inputRegs_[REG_BASE_SERIAL_NO + i] = val;
    }

    for (uint8_t i = 0U; i < 16U; ++i) {
        val = static_cast<uint16_t>(
            (static_cast<uint16_t>(DeviceConfig.location[i * 2U]) << 8U) |
             static_cast<uint16_t>(DeviceConfig.location[i * 2U + 1U])
        );

        holdingRegs_[REG_LOC_STRING_START + i] = val;
        inputRegs_[REG_LOC_STRING_START + i] = val;
    }

    holdingRegs_[REG_TEMP_HIGH_ALARM] = DeviceConfig.alarm_threshold[0];
    holdingRegs_[REG_TEMP_LOW_ALARM]  = DeviceConfig.alarm_threshold[1];
    holdingRegs_[REG_HUM_HIGH_ALARM]  = DeviceConfig.alarm_threshold[2];

    inputRegs_[REG_TEMP_HIGH_ALARM] = DeviceConfig.alarm_threshold[0];
    inputRegs_[REG_TEMP_LOW_ALARM]  = DeviceConfig.alarm_threshold[1];
    inputRegs_[REG_HUM_HIGH_ALARM]  = DeviceConfig.alarm_threshold[2];

    for (uint8_t i = 0U; i < 3U; ++i) {
        val = static_cast<uint16_t>(
            (static_cast<uint16_t>(DeviceConfig.alarm_settings[i]) << 8U) |
             static_cast<uint16_t>(DeviceConfig.relay_off_delay[i])
        );

        holdingRegs_[REG_ALARM1_CONFIG + i] = val;
        inputRegs_[REG_ALARM1_CONFIG + i] = val;
    }

    holdingRegs_[REG_ALRM_HYSTERESIS_1] =
        static_cast<uint16_t>(
            (static_cast<uint16_t>(DeviceConfig.alarm_off_hysteresis[0]) << 8U) |
             static_cast<uint16_t>(DeviceConfig.alarm_off_hysteresis[1])
        );

    holdingRegs_[REG_ALRM_HYSTERESIS_2] =
        static_cast<uint16_t>(DeviceConfig.alarm_off_hysteresis[2]);

    inputRegs_[REG_ALRM_HYSTERESIS_1] = holdingRegs_[REG_ALRM_HYSTERESIS_1];
    inputRegs_[REG_ALRM_HYSTERESIS_2] = holdingRegs_[REG_ALRM_HYSTERESIS_2];

    for (uint8_t i = 0U; i < 6U; ++i) {
        holdingRegs_[REG_ANALOG_OUT_LEV1 + i] =
            static_cast<uint16_t>(DeviceConfig.analog_out_lev[i]);
    }

    holdingRegs_[REG_BAUD_RATE]        = static_cast<uint16_t>(DeviceConfig.baudrate / 100U);
    holdingRegs_[REG_DISPLAY_LANGUAGE] = DeviceConfig.display;
    holdingRegs_[REG_BASE_CONFIG]      = DeviceConfig.config_reg;
    holdingRegs_[REG_DATE_YEAR_MON]    =
        static_cast<uint16_t>(
            (static_cast<uint16_t>(DeviceConfig.man_date_year) << 8U) |
             static_cast<uint16_t>(DeviceConfig.man_date_month)
        );
    holdingRegs_[REG_DATE_YEAR_MON]    = static_cast<uint16_t>(((BUILD_YEAR & 0xFFU) << 8U) |
                          (BUILD_MONTH  & 0xFFU));
		holdingRegs_[REG_DATE_DAY]=   static_cast<uint16_t>((BUILD_DAY & 0xFFU));
		inputRegs_[REG_DATE_DAY]=holdingRegs_[REG_DATE_DAY];
    inputRegs_[REG_BASE_CONFIG]    = DeviceConfig.config_reg;
    inputRegs_[REG_DATE_YEAR_MON]  = holdingRegs_[REG_DATE_YEAR_MON];
    inputRegs_[REG_GAS_TYPE]       = 0x31U;
    inputRegs_[REG_SCALING_FACTOR] = 10U;
	    /*
     * SensorStruct -> Modbus register mirror
     * This restores sensor data loaded from FlashConfig_t into holding/input registers.
     */

    holdingRegs_[REG_SENSOR_TYPE] = Sensor.type;
    inputRegs_[REG_SENSOR_TYPE]   = Sensor.type;

    holdingRegs_[REG_GAS_TYPE] = Sensor.gas;
    inputRegs_[REG_GAS_TYPE]   = Sensor.gas;

    holdingRegs_[REG_MEAS_UNIT] = Sensor.display_unit;
    inputRegs_[REG_MEAS_UNIT]   = Sensor.display_unit;

//    holdingRegs_[REG_SCALING_FACTOR] = Sensor.mul_factor;
//    inputRegs_[REG_SCALING_FACTOR]   = Sensor.mul_factor;

    {
        const uint16_t factor =
            (Sensor.mul_factor != 0U) ? Sensor.mul_factor : 1U;

        holdingRegs_[REG_GAS_CONCT_RANGE] =
            static_cast<uint16_t>(Sensor.gas_conc_range * static_cast<double>(factor));

        inputRegs_[REG_GAS_CONCT_RANGE] =
            holdingRegs_[REG_GAS_CONCT_RANGE];

        holdingRegs_[REG_CALB_CONC_VAL] =
            static_cast<uint16_t>(Sensor.Calibration.aplied_gas_conc *
                                  static_cast<double>(factor));

        inputRegs_[REG_CALB_CONC_VAL] =
            holdingRegs_[REG_CALB_CONC_VAL];
    }

    holdingRegs_[REG_CAL_GAS_TYPE] = Sensor.Calibration.applied_gas;
    inputRegs_[REG_CAL_GAS_TYPE]   = Sensor.Calibration.applied_gas;

    holdingRegs_[REG_CALB_CYCLE] = Sensor.Calibration.cycle;
    inputRegs_[REG_CALB_CYCLE]   = Sensor.Calibration.cycle;

    holdingRegs_[REG_GAS_LEL_VALUE] =
        static_cast<uint16_t>(Sensor.gas_lel_value * 100.0);

    inputRegs_[REG_GAS_LEL_VALUE] =
        holdingRegs_[REG_GAS_LEL_VALUE];

    holdingRegs_[REG_PID_GAS_INDEX] =
        static_cast<uint16_t>(Sensor.voc_gas + 1U);

    inputRegs_[REG_PID_GAS_INDEX] =
        holdingRegs_[REG_PID_GAS_INDEX];

    holdingRegs_[REG_SEN_SUPPLY_VOLTAGE] = Sensor.supply_voltage;
    inputRegs_[REG_SEN_SUPPLY_VOLTAGE]   = Sensor.supply_voltage;

    holdingRegs_[WARM_UP_TIME_REG] = Sensor.warm_up_time;
    inputRegs_[WARM_UP_TIME_REG]   = Sensor.warm_up_time;

    holdingRegs_[OPERATING_TEMP_REG] =
        static_cast<uint16_t>(static_cast<int16_t>(Sensor.opr_temp_max));

    inputRegs_[OPERATING_TEMP_REG] =
        holdingRegs_[OPERATING_TEMP_REG];

    holdingRegs_[CALL_SPAN_TIME_REG] = Sensor.call_span_time;
    inputRegs_[CALL_SPAN_TIME_REG]   = Sensor.call_span_time;

    holdingRegs_[CALL_ZERO_TIME_REG] = Sensor.call_zero_time;
    inputRegs_[CALL_ZERO_TIME_REG]   = Sensor.call_zero_time;

    holdingRegs_[IR_THERMISTOR_RES_REG] = Sensor.th_resistor;
    inputRegs_[IR_THERMISTOR_RES_REG]   = Sensor.th_resistor;

    holdingRegs_[IR_THERMISTOR_BETA_REG] = Sensor.th_beta;
    inputRegs_[IR_THERMISTOR_BETA_REG]   = Sensor.th_beta;

    holdingRegs_[SENSOR_MAN_DATE_REG] = Sensor.man_date;
    inputRegs_[SENSOR_MAN_DATE_REG]   = Sensor.man_date;

    for (uint8_t i = 0U; i < 4U; ++i) {
        const uint16_t serial =
            static_cast<uint16_t>(
                (static_cast<uint16_t>(Sensor.serial_number[i * 2U]) << 8U) |
                 static_cast<uint16_t>(Sensor.serial_number[i * 2U + 1U])
            );

        holdingRegs_[REG_SERIAL_NO + i] = serial;
        inputRegs_[REG_SERIAL_NO + i]   = serial;
    }

    for (uint8_t i = 0U; i < 4U; ++i) {
        const uint16_t manId =
            static_cast<uint16_t>(
                (static_cast<uint16_t>(Sensor.man_id_number[i * 2U]) << 8U) |
                 static_cast<uint16_t>(Sensor.man_id_number[i * 2U + 1U])
            );

        holdingRegs_[REG_MAN_ID_NUMB1 + i] = manId;
        inputRegs_[REG_MAN_ID_NUMB1 + i]   = manId;
    }

    holdingRegs_[REG_ALRM_HYSTERESIS_1] =
        static_cast<uint16_t>(
            (static_cast<uint16_t>(DeviceConfig.alarm_off_hysteresis[0]) << 8U) |
             static_cast<uint16_t>(DeviceConfig.alarm_off_hysteresis[1])
        );

    holdingRegs_[REG_ALRM_HYSTERESIS_2] =
        static_cast<uint16_t>(DeviceConfig.alarm_off_hysteresis[2]);

    inputRegs_[REG_ALRM_HYSTERESIS_1] = holdingRegs_[REG_ALRM_HYSTERESIS_1];
    inputRegs_[REG_ALRM_HYSTERESIS_2] = holdingRegs_[REG_ALRM_HYSTERESIS_2];

    updateDiagnosticRegisters();
}

void ModbusService::sensorUpdate()
{
    uint16_t status = 0U;
    uint16_t alarms = 0U;

    setInputRegisterInternal(REG_TEMP_VALUE, static_cast<uint16_t>(tempVal_));
    setInputRegisterInternal(0x20U, static_cast<uint16_t>(humVal_));

    if ((tempVal_ > static_cast<int16_t>(getHoldingRegisterInternal(REG_TEMP_HIGH_ALARM))) ||
        (tempVal_ < static_cast<int16_t>(getHoldingRegisterInternal(REG_TEMP_LOW_ALARM)))) {
        status |= 0x01U;

        if (tempVal_ > static_cast<int16_t>(getHoldingRegisterInternal(REG_TEMP_HIGH_ALARM))) {
            alarms |= 0x01U;
        }

        if (tempVal_ < static_cast<int16_t>(getHoldingRegisterInternal(REG_TEMP_LOW_ALARM))) {
            alarms |= 0x02U;
        }
    }

    if (humVal_ > static_cast<int16_t>(getHoldingRegisterInternal(REG_HUM_HIGH_ALARM))) {
        status |= 0x02U;
        alarms |= 0x04U;
    }

    setInputRegisterInternal(REG_SENSOR_STATUS, status);
    setInputRegisterInternal(REG_ALARM_FLAGS, alarms);
}

void ModbusService::updateLiveInputs()
{
    inputRegs_[0x20U] = static_cast<uint16_t>(humVal_);

    uint16_t status = 0U;
    if (tempVal_ > static_cast<int16_t>(holdingRegs_[REG_TEMP_HIGH_ALARM])) {
        status |= 0x01U;
    }

    inputRegs_[REG_SENSOR_STATUS] = status;

    updateDiagnosticRegisters();
}

void ModbusService::updateDiagnosticRegisters()
{
    holdingRegs_[REG_DIAG_CRC_ERRORS] = errors_.crcErrorCount();
    holdingRegs_[REG_DIAG_UART_OVERFLOWS] = errors_.uartOverflowCount();
    holdingRegs_[REG_DIAG_SENSOR_READ_FAILS] = errors_.sensorReadFailCount();
    holdingRegs_[REG_DIAG_I2C_RECOVERIES] = errors_.i2cRecoveryCount();
    holdingRegs_[REG_DIAG_WATCHDOG_RESETS] = errors_.watchdogResetCount();
    holdingRegs_[REG_DIAG_FLASH_SAVE_FAILS] = errors_.flashSaveFailCount();
    holdingRegs_[REG_DIAG_HEALTH_STATE] = errors_.systemHealthy() ? 1U : 0U;
    holdingRegs_[REG_DIAG_CONSECUTIVE_FAULTS] =
        static_cast<uint16_t>((static_cast<uint16_t>(errors_.consecutiveSensorReadFailCount()) << 8U) |
                              static_cast<uint16_t>(errors_.consecutiveUartOverflowCount()));

    inputRegs_[REG_DIAG_CRC_ERRORS] = holdingRegs_[REG_DIAG_CRC_ERRORS];
    inputRegs_[REG_DIAG_UART_OVERFLOWS] = holdingRegs_[REG_DIAG_UART_OVERFLOWS];
    inputRegs_[REG_DIAG_SENSOR_READ_FAILS] = holdingRegs_[REG_DIAG_SENSOR_READ_FAILS];
    inputRegs_[REG_DIAG_I2C_RECOVERIES] = holdingRegs_[REG_DIAG_I2C_RECOVERIES];
    inputRegs_[REG_DIAG_WATCHDOG_RESETS] = holdingRegs_[REG_DIAG_WATCHDOG_RESETS];
    inputRegs_[REG_DIAG_FLASH_SAVE_FAILS] = holdingRegs_[REG_DIAG_FLASH_SAVE_FAILS];
    inputRegs_[REG_DIAG_HEALTH_STATE] = holdingRegs_[REG_DIAG_HEALTH_STATE];
    inputRegs_[REG_DIAG_CONSECUTIVE_FAULTS] = holdingRegs_[REG_DIAG_CONSECUTIVE_FAULTS];
}

uint8_t ModbusService::processFrame(const frame_item_t* frame)
{
    if ((frame == nullptr) || (frame->len < 4U)) {
        return 0U;
    }

    const uint16_t crcRx =
        static_cast<uint16_t>(
            frame->data[frame->len - 2U] |
            (static_cast<uint16_t>(frame->data[frame->len - 1U]) << 8U)
        );

    if (crc16(frame->data, static_cast<uint16_t>(frame->len - 2U)) != crcRx) {
        errors_.reportCrcError();
        return 0U;
    }

    errors_.reportValidModbusFrame();

    /*
     * Modbus RTU slave address filtering.
     * Use the runtime DeviceConfig.id instead of forcing MODBUS_SLAVE_ID,
     * so changing the slave ID through base settings takes effect.
     *
     * Address 0 is Modbus broadcast: accept valid write frames but never
     * transmit a response. Any other address must match this device.
     */
    if ((DeviceConfig.id > 0U) && (DeviceConfig.id <= 247U)) {
        slaveAddress_ = static_cast<uint8_t>(DeviceConfig.id);
    }

    const uint8_t rxAddress = frame->data[0];
    const bool isBroadcast = (rxAddress == 0U);

    if ((!isBroadcast) && (rxAddress != slaveAddress_)) {
        return 0U;
    }

    switch (frame->data[1]) {
    case FC_READ_HOLDING:
        return processReadHoldingRegisters(frame);

    case FC_READ_INPUT:
        return processReadInputRegisters(frame);

    case FC_WRITE_SINGLE_REG:
        return processWriteSingleRegister(frame);

    case FC_WRITE_MULTIPLE_REGS:
        return processWriteMultipleRegisters(frame);

    default:
        if (frame->data[0] != 0U) {
            sendErrorResponse(frame->data[1], MODBUS_ERR_ILLEGAL_FUNCTION);
        }
        return 0U;
    }
}

uint8_t ModbusService::processReadHoldingRegisters(const frame_item_t* frame)
{
    const uint16_t addr =
        static_cast<uint16_t>((static_cast<uint16_t>(frame->data[2]) << 8U) | frame->data[3]);

    const uint16_t qty =
        static_cast<uint16_t>((static_cast<uint16_t>(frame->data[4]) << 8U) | frame->data[5]);

    if ((qty == 0U) || (qty > 125U) ||
        ((static_cast<uint32_t>(addr) + qty) > HOLDING_REG_SIZE)) {
        if (frame->data[0] != 0U) {
            sendErrorResponse(FC_READ_HOLDING, MODBUS_ERR_ILLEGAL_DATA_ADDR);
        }
        return 0U;
    }

    uint8_t* const resp = responseScratch_;
    uint16_t idx = 0U;

    resp[idx++] = slaveAddress_;
    resp[idx++] = FC_READ_HOLDING;
    resp[idx++] = static_cast<uint8_t>(qty * 2U);

    for (uint16_t i = 0U; i < qty; ++i) {
        const uint16_t val = getHoldingRegisterInternal(addr + i);
        resp[idx++] = static_cast<uint8_t>(val >> 8U);
        resp[idx++] = static_cast<uint8_t>(val & 0xFFU);
    }

    const uint16_t crc = crc16(resp, idx);
    resp[idx++] = static_cast<uint8_t>(crc & 0xFFU);
    resp[idx++] = static_cast<uint8_t>(crc >> 8U);

    if (frame->data[0] != 0U) {
        uart_.send(resp, idx);
    }

    return 1U;
}

uint8_t ModbusService::processReadInputRegisters(const frame_item_t* frame)
{
    const uint16_t addr =
        static_cast<uint16_t>((static_cast<uint16_t>(frame->data[2]) << 8U) | frame->data[3]);

    const uint16_t qty =
        static_cast<uint16_t>((static_cast<uint16_t>(frame->data[4]) << 8U) | frame->data[5]);

    if ((qty == 0U) || (qty > 125U) ||
        ((static_cast<uint32_t>(addr) + qty) > InputRegSize)) {
        if (frame->data[0] != 0U) {
            sendErrorResponse(FC_READ_INPUT, MODBUS_ERR_ILLEGAL_DATA_ADDR);
        }
        return 0U;
    }

    updateLiveInputs();

    uint8_t* const resp = responseScratch_;
    uint16_t idx = 0U;

    resp[idx++] = slaveAddress_;
    resp[idx++] = FC_READ_INPUT;
    resp[idx++] = static_cast<uint8_t>(qty * 2U);

    for (uint16_t i = 0U; i < qty; ++i) {
        const uint16_t val = getInputRegisterInternal(addr + i);
        resp[idx++] = static_cast<uint8_t>(val >> 8U);
        resp[idx++] = static_cast<uint8_t>(val & 0xFFU);
    }

    const uint16_t crc = crc16(resp, idx);
    resp[idx++] = static_cast<uint8_t>(crc & 0xFFU);
    resp[idx++] = static_cast<uint8_t>(crc >> 8U);

    if (frame->data[0] != 0U) {
        uart_.send(resp, idx);
    }

    return 1U;
}

uint8_t ModbusService::processWriteSingleRegister(const frame_item_t* frame)
{
    if (frame->len < 8U) {
        return 0U;
    }

    const uint16_t addr =
        static_cast<uint16_t>((static_cast<uint16_t>(frame->data[2]) << 8U) | frame->data[3]);

    const uint16_t val =
        static_cast<uint16_t>((static_cast<uint16_t>(frame->data[4]) << 8U) | frame->data[5]);

    if (isWritable(addr) == 0U) {
        if (frame->data[0] != 0U) {
            sendErrorResponse(FC_WRITE_SINGLE_REG, MODBUS_ERR_ILLEGAL_DATA_ADDR);
        }
        return 0U;
    }

    setHoldingRegisterInternal(addr, val);

    if (frame->data[0] != 0U) {
        uint8_t resp[8];
        std::memcpy(resp, frame->data, 6U);

        const uint16_t crc = crc16(resp, 6U);
        resp[6] = static_cast<uint8_t>(crc & 0xFFU);
        resp[7] = static_cast<uint8_t>(crc >> 8U);

        uart_.send(resp, 8U);
    }

    return 1U;
}

uint8_t ModbusService::processWriteMultipleRegisters(const frame_item_t* frame)
{
    if (frame->len < 9U) {
        return 0U;
    }

    const uint16_t addr =
        static_cast<uint16_t>((static_cast<uint16_t>(frame->data[2]) << 8U) | frame->data[3]);

    const uint16_t qty =
        static_cast<uint16_t>((static_cast<uint16_t>(frame->data[4]) << 8U) | frame->data[5]);

    const uint8_t byteCnt = frame->data[6];

    if ((qty == 0U) ||
        (qty > 123U) ||
        (byteCnt != static_cast<uint8_t>(qty * 2U)) ||
        ((static_cast<uint32_t>(addr) + qty) > HOLDING_REG_SIZE) ||
        (frame->len < static_cast<uint16_t>(9U + byteCnt))) {
        if (frame->data[0] != 0U) {
            sendErrorResponse(FC_WRITE_MULTIPLE_REGS, MODBUS_ERR_ILLEGAL_DATA_ADDR);
        }
        return 0U;
    }

    for (uint16_t i = 0U; i < qty; ++i) {
        holdingRegs_[addr + i] =
            static_cast<uint16_t>(
                (static_cast<uint16_t>(frame->data[7U + (i * 2U)]) << 8U) |
                 static_cast<uint16_t>(frame->data[8U + (i * 2U)])
            );
    }

    if (frame->data[0] != 0U) {
        uint8_t resp[8];
        std::memcpy(resp, frame->data, 6U);

        const uint16_t crc = crc16(resp, 6U);
        resp[6] = static_cast<uint8_t>(crc & 0xFFU);
        resp[7] = static_cast<uint8_t>(crc >> 8U);

        uart_.send(resp, 8U);
    }

    runCommand(addr, qty);
    return 1U;
}

void ModbusService::sendErrorResponse(uint8_t function, uint8_t errorCode)
{
    uint8_t resp[5];

    resp[0] = slaveAddress_;
    resp[1] = static_cast<uint8_t>(function | 0x80U);
    resp[2] = errorCode;

    const uint16_t crc = crc16(resp, 3U);
    resp[3] = static_cast<uint8_t>(crc & 0xFFU);
    resp[4] = static_cast<uint8_t>(crc >> 8U);

    uart_.send(resp, 5U);
}

uint8_t ModbusService::queueFrame(const uint8_t* data, uint16_t len)
{
    uint8_t success = 0U;

    if (data == nullptr) {
        return 0U;
    }

    if (len > 256U) {
        len = 256U;
    }

    __disable_irq();

    if (frameQueue_.count < FRAME_QUEUE_SIZE) {
        std::memcpy(frameQueue_.frames[frameQueue_.head].data, data, len);
        frameQueue_.frames[frameQueue_.head].len = len;
        frameQueue_.frames[frameQueue_.head].timestamp = GetTime_us();

        frameQueue_.head++;
        if (frameQueue_.head >= FRAME_QUEUE_SIZE) {
            frameQueue_.head = 0U;
        }

        frameQueue_.count++;
        success = 1U;
    } else {
        frameQueue_.overflow_count++;
    }

    __enable_irq();

    if (success == 0U) {
        errors_.reportUartOverflow();
    }

    return success;
}

uint8_t ModbusService::dequeueFrame(frame_item_t* frame)
{
    uint8_t success = 0U;

    if (frame == nullptr) {
        return 0U;
    }

    __disable_irq();

    if (frameQueue_.count > 0U) {
        std::memcpy(frame->data,
               frameQueue_.frames[frameQueue_.tail].data,
               frameQueue_.frames[frameQueue_.tail].len);

        frame->len = frameQueue_.frames[frameQueue_.tail].len;
        frame->timestamp = frameQueue_.frames[frameQueue_.tail].timestamp;

        frameQueue_.tail++;
        if (frameQueue_.tail >= FRAME_QUEUE_SIZE) {
            frameQueue_.tail = 0U;
        }

        frameQueue_.count--;
        success = 1U;
    }

    __enable_irq();

    return success;
}

const frame_item_t* ModbusService::peekFrame() const
{
    const frame_item_t* frame = nullptr;

    __disable_irq();

    if (frameQueue_.count > 0U) {
        frame = &frameQueue_.frames[frameQueue_.tail];
    }

    __enable_irq();

    return frame;
}

void ModbusService::popFrame()
{
    __disable_irq();

    if (frameQueue_.count > 0U) {
        frameQueue_.tail++;
        if (frameQueue_.tail >= FRAME_QUEUE_SIZE) {
            frameQueue_.tail = 0U;
        }

        frameQueue_.count--;
    }

    __enable_irq();
}

uint8_t ModbusService::queueCount() const
{
    uint8_t count;

    __disable_irq();
    count = frameQueue_.count;
    __enable_irq();

    return count;
}

uint8_t ModbusService::isQueueFull() const
{
    uint8_t full;

    __disable_irq();
    full = (frameQueue_.count >= FRAME_QUEUE_SIZE) ? 1U : 0U;
    __enable_irq();

    return full;
}

uint8_t ModbusService::isQueueEmpty() const
{
    uint8_t empty;

    __disable_irq();
    empty = (frameQueue_.count == 0U) ? 1U : 0U;
    __enable_irq();

    return empty;
}

void ModbusService::clearQueue()
{
    __disable_irq();

    frameQueue_.head = 0U;
    frameQueue_.tail = 0U;
    frameQueue_.count = 0U;
    frameQueue_.overflow_count = 0U;

    __enable_irq();
}

uint32_t ModbusService::overflowCount() const
{
    uint32_t count;

    __disable_irq();
    count = frameQueue_.overflow_count;
    __enable_irq();

    return count;
}

void ModbusService::updateRegisters(uint16_t startReg, const uint16_t* data, uint16_t count)
{
    if (data == nullptr) {
        return;
    }

    for (uint16_t i = 0U; i < count; ++i) {
        if ((startReg + i) < HOLDING_REG_SIZE) {
            holdingRegs_[startReg + i] = data[i];
        }
    }
}

uint8_t ModbusService::isWritable(uint16_t addr)
{
    return (addr < HOLDING_REG_SIZE) ? 1U : 0U;
}

uint8_t ModbusService::runCommand(uint16_t startAddr, uint16_t qty)
{
    uint8_t ack = 1U;

    if ((startAddr <= COMMAND_CODE) &&
        ((static_cast<uint32_t>(startAddr) + qty) > COMMAND_CODE)) {

        const uint16_t cmd = holdingRegs_[COMMAND_CODE];
        uint16_t* args = &holdingRegs_[COMMAND_ARG1];

        switch (cmd) {
        case SET_SENSOR_DATA:
            ack = saveSensorSettings(args, static_cast<uint8_t>(qty - 2U));
            break;

        case SET_DETECTOR_DATA:
            ack = saveBaseSettings(args, static_cast<uint8_t>(qty - 2U));
            break;

        case SET_TIME_DATE:
            ack = 1U;
            break;

        case RESET_DETECTOR:
            /* NVIC_SystemReset(); */
            ack = 1U;
            break;

        default:
            ack = 0U;
            break;
        }
        if (ack != 0U) {
            /*
             * Keep Modbus response path fast.
             * saveBaseSettings() / saveSensorSettings() update DeviceConfig in RAM
             * and set SaveDevSettings / SaveSensorInfo flags.
             * Flash persistence is handled later by Application::run().
             */
            syncAll();
        }

        holdingRegs_[COMMAND_CODE] = 0U;
    }

    return ack;
}

uint8_t ModbusService::saveBaseSettings(const uint16_t* data, uint8_t len)
{
    uint8_t ok = 1U;

    if (data == nullptr) {
        return 0U;
    }

    if (len < 45U) {
        return 0U;
    }

    if ((data[0] > 0U) && (data[0] <= 247U)) {
        DeviceConfig.id = static_cast<uint8_t>(data[0]);
        slaveAddress_ = DeviceConfig.id;
    } else {
        ok = 0U;
    }

    for (uint8_t i = 0U; i < 4U; ++i) {
        DeviceConfig.base_ser_num[i * 2U] =
            static_cast<uint8_t>(data[1U + i] >> 8U);

        DeviceConfig.base_ser_num[i * 2U + 1U] =
            static_cast<uint8_t>(data[1U + i] & 0xFFU);
    }

    for (uint8_t i = 0U; i < 12U; ++i) {
        DeviceConfig.location[i * 2U] =
            static_cast<uint8_t>(data[13U + i] >> 8U);

        DeviceConfig.location[i * 2U + 1U] =
            static_cast<uint8_t>(data[13U + i] & 0xFFU);
    }

    DeviceConfig.location[24U] = 0x00U;

    DeviceConfig.config_reg =
        static_cast<uint16_t>(
            DeviceConfig.config_reg &
            static_cast<uint16_t>(
                ~(TEST_DUE_FLT_INHIBIT |
                  ZERO_SUPPRESSION_EN |
                  RELAY_CARD_INSTALLED |
                  RS485_FLT_INHIBIT)
            )
        );

    if (data[29U] == 0x01U) {
        DeviceConfig.config_reg |= TEST_DUE_FLT_INHIBIT;
    }

    if (data[30U] == 0x01U) {
        DeviceConfig.config_reg |= ZERO_SUPPRESSION_EN;
    }

    DeviceConfig.man_date_year = static_cast<uint8_t>(data[31U]);
    DeviceConfig.man_date_month = static_cast<uint8_t>(data[32U]);

    for (uint8_t i = 0U; i < 6U; ++i) {
        DeviceConfig.analog_out_lev[i] = static_cast<uint8_t>(data[33U + i]);
    }

    DeviceConfig.display = static_cast<uint8_t>(data[39U] & 0x0FU);

    if (data[40U] == 1U) {
        DeviceConfig.display |= DISPLAY_ON;
    }

    if (data[41U] == 1U) {
        DeviceConfig.config_reg |= RS485_FLT_INHIBIT;
    }

    if (data[42U] == 1U) {
        DeviceConfig.config_reg |= RELAY_CARD_INSTALLED;
    }

    const uint32_t baudrate =
        (static_cast<uint32_t>(data[43U]) << 16U) |
         static_cast<uint32_t>(data[44U]);

    DeviceConfig.baudrate = baudrate;

    if (len >= 47U) {
        DeviceConfig.config_reg =
            static_cast<uint16_t>(
                DeviceConfig.config_reg &
                static_cast<uint16_t>(
                    ~(BATTERY_FLT_INHIBIT | CALL_DUE_FLT_INHIBIT)
                )
            );

        if (data[45U] == 1U) {
            DeviceConfig.config_reg |= CALL_DUE_FLT_INHIBIT;
        }

        if (data[46U] == 1U) {
            DeviceConfig.config_reg |= BATTERY_FLT_INHIBIT;
        }
    }

    if ((len >= 51U) && (ok == 1U)) {
        DeviceConfig.config_reg =
            static_cast<uint16_t>(
                DeviceConfig.config_reg &
                static_cast<uint16_t>(~ANLG_OUT_FLT_AUTO_RESET)
            );

        if (data[47U] == 1U) {
            DeviceConfig.config_reg |= ANLG_OUT_FLT_AUTO_RESET;
        }
    }

    if (len >= 53U) {
        if (data[52U] != 0U) {
            DeviceConfig.config_reg |= OVER_RANGE_AUTO_RESET;
        } else {
            DeviceConfig.config_reg =
                static_cast<uint16_t>(
                    DeviceConfig.config_reg &
                    static_cast<uint16_t>(~OVER_RANGE_AUTO_RESET)
                );
        }
    }

    if (ok != 0U) {
        SaveDevSettings = 1U;
    }

    return ok;
}

uint8_t ModbusService::saveSensorSettings(const uint16_t* data, uint8_t len)
{
    if (data == nullptr) {
        return 0U;
    }

    if (len < 56U) {
        return 0U;
    }

    /*
     * Work on a persistent scratch copy first, like the original C implementation.
     * This prevents half-updated Sensor values while avoiding a large stack frame.
     */
    sensorScratch_ = Sensor;
    SensorStruct& settings = sensorScratch_;

    uint8_t ok = 0U;
    uint16_t factor = 10U;

    std::memset(alarmScratch_, 0, sizeof(alarmScratch_));
    std::memset(alarmThresholdScratch_, 0, sizeof(alarmThresholdScratch_));
    AlarmStruct* const alarm_stgn = alarmScratch_;
    uint16_t* const alrm_thrshold = alarmThresholdScratch_;

    if ((len >= 64U) && (data[63U] > 0U)) {
        factor = data[63U];
    }

    const uint16_t model = GetSensorModel(data[0U]);
    if (model == 0U) {
        ok = 2U;
    }

    const uint16_t type = CheckSensorType(data[1U]);
    if (type == 0U) {
        ok = 3U;
    }

    settings.model = model;
    settings.type = type;
    settings.gas = static_cast<uint8_t>(data[2U]);

    if (settings.gas < GAS_INDX_OFFSET) {
        ok = 4U;
    }

    settings.display_unit = static_cast<uint8_t>(data[3U]);

    if ((settings.display_unit < PPM) || (settings.display_unit > PPB)) {
        ok = 5U;
    }

    settings.mul_factor = factor;

    settings.gas_conc_range = static_cast<double>(data[4U]);
    settings.gas_conc_range /= static_cast<double>(factor);

    settings.Calibration.aplied_gas_conc = static_cast<double>(data[33U]);
    settings.Calibration.aplied_gas_conc /= static_cast<double>(factor);

//    if (ok == 0U) {
//        ok = CheckConctSettings(&Sensor);
//    }

    settings.Calibration.applied_gas = static_cast<uint8_t>(data[6U]);

    if ((settings.type == CATALYTIC_SENSOR) && (settings.gas != C2H2)) {
        const bool appliedGasValid =
            (settings.Calibration.applied_gas == settings.gas) ||
            (settings.Calibration.applied_gas == CH4) ||
            (settings.Calibration.applied_gas == C5H12) ||
            (settings.Calibration.applied_gas == C3H8);

        if (!appliedGasValid) {
            ok = 19U;
        }
    } else if ((settings.type == PID_SENSOR) &&
               ((settings.gas == VOC) || (settings.gas == TVOC))) {
        const bool appliedGasValid =
            (settings.Calibration.applied_gas == settings.gas) ||
            (settings.Calibration.applied_gas == ISOBUTYLENE);

        if (!appliedGasValid) {
            ok = 19U;
        }
    } else {
        settings.Calibration.applied_gas = settings.gas;
    }

    alrm_thrshold[ALARM1] = data[7U];
    alrm_thrshold[ALARM2] = data[8U];
    alrm_thrshold[ALARM3] = data[9U];

//    SetGasAlarmLimits(&Sensor);

    if (settings.type == INFRARED_SENSOR) {
        settings.rd_time_delay_peak = static_cast<uint8_t>(data[10U]);
        settings.rd_time_delay_deep = static_cast<uint8_t>(data[11U]);
        settings.lamp_drv_period = static_cast<uint8_t>(data[12U]);

        if ((settings.rd_time_delay_peak < MIN_RD_DELAY) ||
            (settings.rd_time_delay_peak > MAX_RD_DELAY)) {
            ok = 24U;
        }

        if ((settings.rd_time_delay_deep < MIN_RD_DELAY) ||
            (settings.rd_time_delay_deep > MAX_RD_DELAY)) {
            ok = 25U;
        }

        if ((settings.lamp_drv_period != LAMP_PERIOD_LOW) &&
            (settings.lamp_drv_period != LAMP_PERIOD_HIGH)) {
            ok = 26U;
        }
    }

    for (uint8_t i = 0U; i < 4U; ++i) {
        settings.serial_number[i * 2U] =
            static_cast<uint8_t>(data[13U + i] >> 8U);

        settings.serial_number[i * 2U + 1U] =
            static_cast<uint8_t>(data[13U + i] & 0xFFU);
    }
    settings.serial_number[8U] = 0U;

    for (uint8_t i = 0U; i < 4U; ++i) {
        settings.man_id_number[i * 2U] =
            static_cast<uint8_t>(data[17U + i] >> 8U);

        settings.man_id_number[i * 2U + 1U] =
            static_cast<uint8_t>(data[17U + i] & 0xFFU);
    }
    settings.man_id_number[8U] = 0U;

    uint8_t indx = 21U;

    for (uint8_t i = 0U; i < 3U; ++i) {
        alarm_stgn[ALARM1 + i].config_status = 0U;

        if ((data[indx] & 0xFF00U) != 0U) {
            alarm_stgn[ALARM1 + i].config_status |= AUTO_RESET;
        }

        if ((data[indx] & 0x00FFU) != 0U) {
            alarm_stgn[ALARM1 + i].config_status |= DFLT_ENERGISED;
        }

        ++indx;

        if ((data[indx] & 0xFF00U) != 0U) {
            alarm_stgn[ALARM1 + i].config_status |= TRIP_DIRECTION;
        }

        alarm_stgn[ALARM1 + i].off_delay =
            static_cast<uint8_t>(data[indx] & 0x00FFU);

        ++indx;
    }

    settings.bump_test_interval = static_cast<uint8_t>(data[27U]);

    if ((settings.bump_test_interval == 0U) ||
        (settings.bump_test_interval > 3U)) {
        ok = 27U;
    }

    settings.Calibration.cycle = static_cast<uint8_t>(data[34U]);

    if ((settings.Calibration.cycle < MIN_CALB_CYLE) ||
        (settings.Calibration.cycle > MAX_CALB_CYLE)) {
        ok = 28U;
    }

    const uint8_t wr_prtc = static_cast<uint8_t>(data[35U]);

    int32_t coef = 0;

    coef = (static_cast<int32_t>(data[37U]) << 16) |
           static_cast<int32_t>(data[36U]);
    settings.Coefficient.a = static_cast<double>(coef) / 1000000.0;

    coef = (static_cast<int32_t>(data[39U]) << 16) |
           static_cast<int32_t>(data[38U]);
    settings.Coefficient.n = static_cast<double>(coef) / 1000000.0;

    coef = (static_cast<int32_t>(data[41U]) << 16) |
           static_cast<int32_t>(data[40U]);
    settings.Coefficient.alpha_pos = static_cast<double>(coef) / 1000000.0;

    coef = (static_cast<int32_t>(data[43U]) << 16) |
           static_cast<int32_t>(data[42U]);
    settings.Coefficient.alpha_neg = static_cast<double>(coef) / 1000000.0;

    coef = (static_cast<int32_t>(data[45U]) << 16) |
           static_cast<int32_t>(data[44U]);
    settings.Coefficient.beta_pos = static_cast<double>(coef) / 1000000.0;

    coef = (static_cast<int32_t>(data[47U]) << 16) |
           static_cast<int32_t>(data[46U]);
    settings.Coefficient.beta_neg = static_cast<double>(coef) / 1000000.0;

    settings.th_resistor = data[48U];
    settings.th_beta = data[49U];

    settings.man_date =
        static_cast<uint16_t>((data[50U] << 8U) | data[51U]);

    settings.voc_gas = 0U;

    if ((settings.type == PID_SENSOR) &&
        ((settings.gas == VOC) || (settings.gas == TVOC))) {
        if ((data[52U] <= 916U) && (data[52U] > 0U)) {
            settings.voc_gas = static_cast<uint16_t>(data[52U] - 1U);
        } else {
            ok = 34U;
        }
    }

    if (len >= 57U) {
        alarm_stgn[ALARM1].hysteresis = static_cast<uint8_t>(data[53U]);
        alarm_stgn[ALARM2].hysteresis = static_cast<uint8_t>(data[54U]);
        alarm_stgn[ALARM3].hysteresis = static_cast<uint8_t>(data[55U]);

        if ((data[53U] < MIN_HYSTERESIS) ||
            (data[53U] > MAX_HYSTERESIS)) {
            ok = 35U;
        } else if ((data[54U] < MIN_HYSTERESIS) ||
                   (data[54U] > MAX_HYSTERESIS)) {
            ok = 36U;
        } else if ((data[55U] < MIN_HYSTERESIS) ||
                   (data[55U] > MAX_HYSTERESIS)) {
            ok = 37U;
        }

        if ((data[56U] & 0xFF00U) != 0U) {
            alarm_stgn[ALARM1].config_status |= ALRM_TRG_TYPE;
        }

        if ((data[56U] & 0x00FFU) != 0U) {
            alarm_stgn[ALARM2].config_status |= ALRM_TRG_TYPE;
        }

        if ((data[57U] & 0xFF00U) != 0U) {
            alarm_stgn[ALARM3].config_status |= ALRM_TRG_TYPE;
        }
    } else {
        alarm_stgn[ALARM1].hysteresis = DeviceConfig.alarm_off_hysteresis[0U];
        alarm_stgn[ALARM2].hysteresis = DeviceConfig.alarm_off_hysteresis[1U];
        alarm_stgn[ALARM3].hysteresis = DeviceConfig.alarm_off_hysteresis[2U];
    }

    if (len >= 59U) {
        settings.gas_lel_value = static_cast<double>(data[58U]) / 100.0;
        settings.Calibration.aplied_gas_lel_value =
            static_cast<double>(data[60U]) / 100.0;

        if (settings.type == CATALYTIC_SENSOR) {
            settings.relative_sensitivity =
                static_cast<double>(data[59U]) / 100.0;

            settings.Calibration.aplied_gas_rel_sens =
                static_cast<double>(data[61U]) / 100.0;
        } else if (settings.type == PID_SENSOR) {
            settings.relative_sensitivity =
                static_cast<double>(data[62U]) / 100.0;
        }
    }

    if (len >= 65U) {
        settings.supply_voltage = static_cast<uint8_t>(data[64U]);
    } else {
        settings.supply_voltage = 25U;
    }

    if (settings.supply_voltage > 50U) {
        ok = 40U;
    }

    if (len >= 66U) {
        if (data[65U] <= 10U) {
            settings.filter_update_tick = static_cast<uint8_t>(data[65U]);
        } else {
            ok = 41U;
        }
    } else {
        settings.filter_update_tick = 0U;
    }

    if (len >= 68U) {
        settings.warm_up_time = static_cast<uint8_t>(data[66U]);
        settings.opr_temp_max = static_cast<double>(data[67U]);
    } else {
        settings.warm_up_time = static_cast<uint8_t>(MIN_WARM_UP_TIME);
        settings.opr_temp_max = static_cast<double>(MAX_CTLY_TEMPRATURE);
    }

    if (len >= 69U) {
        settings.opr_temp_min = static_cast<double>(
            static_cast<int16_t>(data[68U])
        );
    } else {
        settings.opr_temp_min = static_cast<double>(MIN_TEMPRATURE);
    }

    if (len >= 71U) {
        settings.call_span_time = data[69U];
        settings.call_zero_time = data[70U];
    } else {
        settings.call_span_time = static_cast<uint16_t>(MIN_CALIBRATION_TIME);
        settings.call_zero_time = static_cast<uint16_t>(MIN_CALIBRATION_TIME);
    }

    if ((settings.warm_up_time < static_cast<uint8_t>(MIN_WARM_UP_TIME)) ||
        (settings.warm_up_time > static_cast<uint8_t>(MAX_WARM_UP_TIME))) {
        ok = 42U;
    }

    if ((settings.opr_temp_max > static_cast<double>(MAX_TEMPRATURE_RANGE_H)) ||
        (settings.opr_temp_max < static_cast<double>(MAX_TEMPRATURE_RANGE_L))) {
        ok = 43U;
    }

    if ((settings.opr_temp_min > static_cast<double>(MIN_TEMPRATURE_RANGE_H)) ||
        (settings.opr_temp_min < static_cast<double>(MIN_TEMPRATURE_RANGE_L))) {
        ok = 44U;
    }

    if ((settings.call_span_time < static_cast<uint16_t>(MIN_CALIBRATION_TIME)) ||
        (settings.call_span_time > static_cast<uint16_t>(MAX_CALIBRATION_TIME))) {
        ok = 45U;
    }

    if ((settings.call_zero_time < static_cast<uint16_t>(MIN_CALIBRATION_TIME)) ||
        (settings.call_zero_time > static_cast<uint16_t>(MAX_CALIBRATION_TIME))) {
        ok = 46U;
    }

    for (uint8_t i = 0U; i < 3U; ++i) {
        double f_tmp = static_cast<double>(alrm_thrshold[ALARM1 + i]);
        // Sensor.mul_factor is not applied to alarm thresholds; raw Modbus value is preserved.

//        if (f_tmp > HighAlarmLimit) {
//            ok = 20U;
//        } else if (f_tmp < LowAlarmLimit) {
//            ok = 21U;
//        }

        alarm_stgn[ALARM1 + i].on_level = f_tmp;

        f_tmp = static_cast<double>(alarm_stgn[ALARM1 + i].hysteresis);
        f_tmp /= 100.0;
        f_tmp *= settings.gas_conc_range;

        if (f_tmp > (alarm_stgn[ALARM1 + i].on_level / 2.0)) {
            ok = 38U;
        }
    }

    uint8_t direction =
        static_cast<uint8_t>(alarm_stgn[ALARM1].config_status & TRIP_DIRECTION);

    if ((direction == TRIP_FALLING) && (settings.gas != O2)) {
        ok = 37U;
    } else if (direction !=
               (alarm_stgn[ALARM2].config_status & TRIP_DIRECTION)) {
        ok &= 0xFFU;
    } else if ((direction == TRIP_RISING) &&
               (alarm_stgn[ALARM1].on_level < alarm_stgn[ALARM2].on_level)) {
        ok &= 0xFFU;
    } else if ((direction == TRIP_FALLING) &&
               (alarm_stgn[ALARM1].on_level > alarm_stgn[ALARM2].on_level)) {
        ok &= 0xFFU;
    } else {
        ok = 31U;
    }

    direction =
        static_cast<uint8_t>(alarm_stgn[ALARM2].config_status & TRIP_DIRECTION);

    if ((direction == TRIP_FALLING) && (settings.gas != O2)) {
        ok = 36U;
    } else if (direction !=
               (alarm_stgn[ALARM3].config_status & TRIP_DIRECTION)) {
        ok = 32U;
    } else if ((direction == TRIP_RISING) &&
               (alarm_stgn[ALARM2].on_level < alarm_stgn[ALARM3].on_level)) {
        ok &= 0xFFU;
    } else if ((direction == TRIP_FALLING) &&
               (alarm_stgn[ALARM2].on_level > alarm_stgn[ALARM3].on_level)) {
        ok &= 0xFFU;
    } else {
        ok = 33U;
    }

    direction =
        static_cast<uint8_t>(alarm_stgn[ALARM3].config_status & TRIP_DIRECTION);

    if ((direction == TRIP_FALLING) && (settings.gas != O2)) {
        ok = 35U;
    }

    settings.type = CheckSensorType(settings.type);

    if (settings.type == 0U) {
        ok = 34U;
    }

    if ((ok == 31U) || (ok == 32U) || (ok == 33U)) {
        ok = 0U;
    }

    /*
     * The PC/master writes a complete sensor package. Some optional legacy
     * fields can be inconsistent with the selected sensor type/gas and should
     * not cause the whole save to be rejected. Keep only the fundamental
     * identity/range checks as hard failures.
     */
    if ((ok != 0U) &&
        (ok != 2U) &&
        (ok != 3U) &&
        (ok != 4U) &&
        (ok != 5U)) {
        ok = 0U;
    }

    if (ok == 0U) {
        if (settings.mul_factor == 0U) {
            settings.mul_factor = 1U;
        }

        if (settings.type != CATALYTIC_SENSOR) {
            settings.Calibration.span_gas_type = settings.gas;
        }

        for (uint8_t i = 0U; i < 3U; ++i) {
            DeviceConfig.alarm_threshold[i] = alrm_thrshold[ALARM1 + i];

            DeviceConfig.alarm_settings[i] =
                alarm_stgn[ALARM1 + i].config_status;

            DeviceConfig.relay_off_delay[i] =
                alarm_stgn[ALARM1 + i].off_delay;

            DeviceConfig.alarm_off_hysteresis[i] =
                alarm_stgn[ALARM1 + i].hysteresis;
        }

        settings.wr_protect = wr_prtc;

        /* Commit only after validation/conversion is complete. */
        Sensor = settings;

        SaveSensorInfo = 1U;
        SaveDevSettings = 1U;

        return 1U;
    }

    return 0U;
}
