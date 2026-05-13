#include "Sht3xSensor.hpp"
#include <cassert>

int main()
{
    // TC-SENSOR-001: Minimum raw temperature value should convert to -45.00 C.
    assert(Sht3xSensor::temperatureX100(0U) == -4500);

    // TC-SENSOR-002: Maximum raw temperature value should convert to 130.00 C.
    assert(Sht3xSensor::temperatureX100(65535U) == 13000);

    // TC-SENSOR-003: Minimum raw humidity value should convert to 0.00 %RH.
    assert(Sht3xSensor::humidityX100(0U) == 0);

    // TC-SENSOR-004: Maximum raw humidity value should convert to 100.00 %RH.
    assert(Sht3xSensor::humidityX100(65535U) == 10000);

    return 0;
}
