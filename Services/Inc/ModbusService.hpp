#ifndef MODBUSSERVICE_HPP
#define MODBUSSERVICE_HPP

#include <stdint.h>
#include <stddef.h>
#include "UartDriver.hpp"

extern "C" {
#include "modbus_rtu.h"
}

class ModbusService {
public:
    explicit ModbusService(UartDriver& uart);

    void initialize(uint8_t slaveAddress);
    void process();

    void setHoldingRegister(uint16_t reg, uint16_t value);
    uint16_t holdingRegister(uint16_t reg) const;

    void setInputRegister(uint16_t reg, uint16_t value);
    uint16_t inputRegister(uint16_t reg) const;

    void updateRegisters(uint16_t startReg, uint16_t* data, uint16_t count);

    void syncAll();
    void sensorUpdate();
    void updateLiveInputs();

    uint8_t queueFrame(uint8_t* data, uint16_t len);
    uint8_t dequeueFrame(frame_item_t* frame);
    const frame_item_t* peekFrame();
    void popFrame();

    uint8_t queueCount();
    uint8_t isQueueFull();
    uint8_t isQueueEmpty();
    void clearQueue();
    uint32_t overflowCount();

    uint8_t processFrame(const frame_item_t* frame);
    void sendErrorResponse(uint8_t function, uint8_t errorCode);

private:
    static const uint16_t InputRegSize = HOLDING_REG_SIZE;

    uint16_t holdingRegs_[HOLDING_REG_SIZE];
    uint16_t inputRegs_[InputRegSize];
    frame_queue_t frameQueue_;

    UartDriver& uart_;
    uint8_t slaveAddress_;

    int16_t tempVal_;
    int16_t humVal_;

    uint16_t crc16(const uint8_t* buffer, uint16_t len) const;

    uint8_t processReadHoldingRegisters(const frame_item_t* frame);
    uint8_t processReadInputRegisters(const frame_item_t* frame);
    uint8_t processWriteSingleRegister(const frame_item_t* frame);
    uint8_t processWriteMultipleRegisters(const frame_item_t* frame);

    void setHoldingRegisterInternal(uint16_t reg, uint16_t value);
    uint16_t getHoldingRegisterInternal(uint16_t reg) const;
    void setInputRegisterInternal(uint16_t reg, uint16_t value);
    uint16_t getInputRegisterInternal(uint16_t reg) const;

    uint8_t isWritable(uint16_t addr) const;
    uint8_t runCommand(uint16_t startAddr, uint16_t qty);
    uint8_t saveBaseSettings(uint16_t* data, uint8_t len);
    uint8_t saveSensorSettings(uint16_t* data, uint8_t len);
};

#endif