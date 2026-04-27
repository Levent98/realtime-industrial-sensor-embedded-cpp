#include "FlashStorage.hpp"

FlashStorage::FlashStorage()
{
    eraseSector(0U);
}

bool FlashStorage::eraseSector(uint32_t)
{
    for (size_t i = 0; i < StorageSize; ++i) {
        memory_[i] = 0xFFU;
    }
    return true;
}

bool FlashStorage::write(uint32_t address, const void* data, size_t size)
{
    if ((data == 0) || ((address + size) > StorageSize)) {
        return false;
    }
    const uint8_t* source = static_cast<const uint8_t*>(data);
    for (size_t i = 0; i < size; ++i) {
        memory_[address + i] = source[i];
    }
    return true;
}

bool FlashStorage::read(uint32_t address, void* data, size_t size) const
{
    if ((data == 0) || ((address + size) > StorageSize)) {
        return false;
    }
    uint8_t* destination = static_cast<uint8_t*>(data);
    for (size_t i = 0; i < size; ++i) {
        destination[i] = memory_[address + i];
    }
    return true;
}
