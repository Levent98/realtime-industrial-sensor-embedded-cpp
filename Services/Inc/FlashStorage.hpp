#ifndef FLASHSTORAGE_HPP
#define FLASHSTORAGE_HPP

#include <stddef.h>
#include <stdint.h>

class FlashStorage {
public:
    FlashStorage();
    bool eraseSector(uint32_t sector);
    bool write(uint32_t address, const void* data, size_t size);
    bool read(uint32_t address, void* data, size_t size) const;

private:
    static const size_t StorageSize = 256U;
    uint8_t memory_[StorageSize];
};

#endif
