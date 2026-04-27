#include "ModbusService.hpp"

extern "C" {
#include "stm32f410rx.h"
#include "core_cm4.h"
#include "Config_Manager.h"
#include "appcons.h"
#include "apptdef.h"
#include "sensortypes.h"
#include "delay.h"
#include <string.h>
}

#ifndef TRUE
#define TRUE 1U
#endif

#ifndef FALSE
#define FALSE 0U
#endif

extern volatile uint16_t baud_rate;

ModbusService::ModbusService(UartDriver& uart)
    : uart_(uart),
      slaveAddress_(1U),
      tempVal_(250),
      humVal_(600)
{
    memset(holdingRegs_, 0, sizeof(holdingRegs_));
    memset(inputRegs_, 0, sizeof(inputRegs_));
    memset(&frameQueue_, 0, sizeof(frameQueue_));
}

void ModbusService::initialize(uint8_t slaveAddress)
{
    slaveAddress_ = slaveAddress;

    memset(holdingRegs_, 0, sizeof(holdingRegs_));
    memset(inputRegs_, 0, sizeof(inputRegs_));
    memset(&frameQueue_, 0, sizeof(frameQueue_));

    Config_LoadFromFlash();
    syncAll();
}

void ModbusService::process()
{
    uint8_t buffer[UartDriver::BufferSize];

    if (uart_.frameAvailable()) {
        const size_t len = uart_.fetchFrame(buffer, sizeof(buffer));

        if (len > 0U) {
            queueFrame(buffer, static_cast<uint16_t>(len));
        }
    }

    frame_item_t frame;

    if (dequeueFrame(&frame) != 0U) {
        (void)processFrame(&frame);
    }
}

uint16_t ModbusService::crc16(const uint8_t* buffer, uint16_t len) const
{
    uint16_t crc = 0xFFFFU;

    for (uint16_t i = 0U; i < len; ++i) {
        crc ^= buffer[i];

        for (uint8_t j = 0U; j < 8U; ++j) {
            if ((crc & 1U) != 0U) {
                crc = static_cast<uint16_t>((crc >> 1U) ^ 0xA001U);
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
    holdingRegs_[REG_FW_DATE_2]  = 0x0100U;

    inputRegs_[REG_TYPE_REG]   = holdingRegs_[REG_TYPE_REG];
    inputRegs_[REG_DET_MODEL]  = holdingRegs_[REG_DET_MODEL];
    inputRegs_[REG_FW_VERSION] = holdingRegs_[REG_FW_VERSION];
    inputRegs_[REG_FW_DATE]    = holdingRegs_[REG_FW_DATE];
    inputRegs_[REG_FW_DATE_2]  = holdingRegs_[REG_FW_DATE_2];

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

    inputRegs_[REG_BASE_CONFIG]    = DeviceConfig.config_reg;
    inputRegs_[REG_DATE_YEAR_MON]  = holdingRegs_[REG_DATE_YEAR_MON];
    inputRegs_[REG_GAS_TYPE]       = 0x31U;
    inputRegs_[REG_SCALING_FACTOR] = 10U;
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
        return 0U;
    }
     slaveAddress_ = MODBUS_SLAVE_ID;
    if ((frame->data[0] != slaveAddress_) && (frame->data[0] != 0U)) {
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
        sendErrorResponse(FC_READ_HOLDING, MODBUS_ERR_ILLEGAL_DATA_ADDR);
        return 0U;
    }

    uint8_t resp[256];
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
        sendErrorResponse(FC_READ_INPUT, MODBUS_ERR_ILLEGAL_DATA_ADDR);
        return 0U;
    }

    updateLiveInputs();

    uint8_t resp[256];
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
        sendErrorResponse(FC_WRITE_SINGLE_REG, MODBUS_ERR_ILLEGAL_DATA_ADDR);
        return 0U;
    }

    setHoldingRegisterInternal(addr, val);

    if (frame->data[0] != 0U) {
        uint8_t resp[8];
        memcpy(resp, frame->data, 6U);

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
        sendErrorResponse(FC_WRITE_MULTIPLE_REGS, MODBUS_ERR_ILLEGAL_DATA_ADDR);
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
        memcpy(resp, frame->data, 6U);

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

uint8_t ModbusService::queueFrame(uint8_t* data, uint16_t len)
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
        memcpy(frameQueue_.frames[frameQueue_.head].data, data, len);
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
        memcpy(frame->data,
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

const frame_item_t* ModbusService::peekFrame()
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

uint8_t ModbusService::queueCount()
{
    uint8_t count;

    __disable_irq();
    count = frameQueue_.count;
    __enable_irq();

    return count;
}

uint8_t ModbusService::isQueueFull()
{
    uint8_t full;

    __disable_irq();
    full = (frameQueue_.count >= FRAME_QUEUE_SIZE) ? 1U : 0U;
    __enable_irq();

    return full;
}

uint8_t ModbusService::isQueueEmpty()
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

uint32_t ModbusService::overflowCount()
{
    uint32_t count;

    __disable_irq();
    count = frameQueue_.overflow_count;
    __enable_irq();

    return count;
}

void ModbusService::updateRegisters(uint16_t startReg, uint16_t* data, uint16_t count)
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

uint8_t ModbusService::isWritable(uint16_t addr) const
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

        
            Config_QuickSave();
            syncAll();
        

        holdingRegs_[COMMAND_CODE] = 0U;
    }

    return ack;
}

uint8_t ModbusService::saveBaseSettings(uint16_t* data, uint8_t len)
{
    uint8_t ok = 1U;

    if (data == nullptr) {
        return 0U;
    }

    (void)len;

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
        static_cast<uint16_t>(DeviceConfig.config_reg &
        static_cast<uint16_t>(~(TEST_DUE_FLT_INHIBIT | ZERO_SUPPRESSION_EN)));

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

    DeviceConfig.baudrate =
        (static_cast<uint32_t>(data[43U]) << 16U) |
         static_cast<uint32_t>(data[44U]);

    if (ok != 0U) {
        SaveDevSettings = TRUE;
    }

    return ok;
}

uint8_t ModbusService::saveSensorSettings(uint16_t* data, uint8_t len)
{
    if (data == nullptr) {
        return 0U;
    }

    uint8_t ok = 0U;
    uint16_t factor = 10U;

    if ((len >= 64U) && (data[63U] > 0U)) {
        factor = data[63U];
    }

    const uint16_t model = GetSensorModel(data[0U]);
    if (model == 0U) {
        ok = 2U;
    }

    const uint16_t type = CheckSensorType(data[1U]);
    if (type == 0U) {
        ok = 2U;
    }

    (void)factor;
    (void)model;
    (void)type;

    DeviceConfig.alarm_threshold[0] = data[7U];
    DeviceConfig.alarm_threshold[1] = data[8U];
    DeviceConfig.alarm_threshold[2] = data[9U];

    for (uint8_t i = 0U; i < 3U; ++i) {
        DeviceConfig.alarm_settings[i] =
            static_cast<uint8_t>(data[21U + (i * 2U)]);

        DeviceConfig.relay_off_delay[i] =
            static_cast<uint8_t>(data[22U + (i * 2U)]);
    }

    if (len >= 56U) {
        DeviceConfig.alarm_off_hysteresis[0] = static_cast<uint8_t>(data[53U]);
        DeviceConfig.alarm_off_hysteresis[1] = static_cast<uint8_t>(data[54U]);
        DeviceConfig.alarm_off_hysteresis[2] = static_cast<uint8_t>(data[55U]);
    }

    if (ok == 0U) {
        SaveSensorInfo = TRUE;
        SaveDevSettings = TRUE;
        return 1U;
    }

    return 0U;
}
