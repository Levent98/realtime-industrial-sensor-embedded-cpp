#include <cstdint>

#include "LcdDisplay.hpp"
#include "PwmOutput.hpp"

LcdDisplay::LcdDisplay()
    : finalBuffer_{0}
{
}

void LcdDisplay::initialize()
{
}

void LcdDisplay::clear()
{
}

void LcdDisplay::setCursor(uint8_t, uint8_t)
{
}

void LcdDisplay::writeString(const char*)
{
}

void LcdDisplay::writeFixed(int, uint8_t)
{
}

PwmOutput::PwmOutput()
{
}

void PwmOutput::initialize()
{
}

void PwmOutput::setTemperature(int32_t)
{
}

void PwmOutput::setHumidity(uint32_t)
{
}
