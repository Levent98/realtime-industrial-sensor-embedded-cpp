#include "I2cBus.hpp"

I2cBus::I2cBus() noexcept = default;

void I2cBus::initialize() {}

I2cStatus I2cBus::write(std::uint8_t, const std::uint8_t*, std::size_t)
{
    return I2cStatus::InvalidArgument;
}

I2cStatus I2cBus::read(std::uint8_t, std::uint8_t*, std::size_t)
{
    return I2cStatus::InvalidArgument;
}

I2cStatus I2cBus::writeRead(std::uint8_t,
                            const std::uint8_t*,
                            std::size_t,
                            std::uint8_t*,
                            std::size_t)
{
    return I2cStatus::InvalidArgument;
}

bool I2cBus::recoverBus()
{
    return false;
}
