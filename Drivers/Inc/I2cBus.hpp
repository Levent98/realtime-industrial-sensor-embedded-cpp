#ifndef I2CBUS_HPP
#define I2CBUS_HPP

#include <stdint.h>
#include <stddef.h>

enum class I2cStatus : uint8_t {
    Ok = 0,
    Timeout,
    Nack,
    BusError,
    InvalidArgument
};

class I2cBus {
public:
    I2cBus();

    void initialize();

    I2cStatus write(uint8_t address, const uint8_t* data, size_t size);
    I2cStatus read(uint8_t address, uint8_t* data, size_t size);
    I2cStatus writeRead(uint8_t address,
                        const uint8_t* txData,
                        size_t txSize,
                        uint8_t* rxData,
                        size_t rxSize);

    bool recoverBus();

private:
    bool initialized_;
};

#endif