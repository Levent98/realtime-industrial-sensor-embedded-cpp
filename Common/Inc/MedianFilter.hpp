#ifndef MEDIAN_FILTER_HPP
#define MEDIAN_FILTER_HPP

#include <stdint.h>

class MedianFilter {
public:
    static constexpr uint8_t Size = 18U;

    MedianFilter();

    void reset();
    void seed(int32_t value);
    int32_t apply(int32_t value);
    bool initialized() const;

private:
    uint8_t index_;
    uint8_t count_;
    int32_t samples_[Size];
    int32_t sortBuffer_[Size];
};

#endif