#include "MedianFilter.hpp"

MedianFilter::MedianFilter()
    : index_(0U),
      count_(0U),
      samples_{0},
      sortBuffer_{0}
{
}

void MedianFilter::reset()
{
    index_ = 0U;
    count_ = 0U;

    for (uint8_t i = 0U; i < Size; ++i) {
        samples_[i] = 0;
        sortBuffer_[i] = 0;
    }
}

void MedianFilter::seed(int32_t value)
{
    index_ = 0U;
    count_ = Size;

    for (uint8_t i = 0U; i < Size; ++i) {
        samples_[i] = value;
        sortBuffer_[i] = value;
    }
}

bool MedianFilter::initialized() const
{
    return count_ >= Size;
}

int32_t MedianFilter::apply(int32_t value)
{
    uint8_t i;
    uint8_t j;
    int32_t key;

    if (count_ < Size) {
        samples_[count_] = value;
        ++count_;

        if (count_ >= 3U) {
            for (i = 0U; i < count_; ++i) {
                sortBuffer_[i] = samples_[i];
            }

            for (i = 1U; i < count_; ++i) {
                key = sortBuffer_[i];
                j = i;

                while ((j > 0U) && (sortBuffer_[j - 1U] > key)) {
                    sortBuffer_[j] = sortBuffer_[j - 1U];
                    --j;
                }

                sortBuffer_[j] = key;
            }

            return sortBuffer_[count_ / 2U];
        }

        return value;
    }

    samples_[index_] = value;
    ++index_;
    if (index_ >= Size) {
        index_ = 0U;
    }

    for (i = 0U; i < Size; ++i) {
        sortBuffer_[i] = samples_[i];
    }

    for (i = 1U; i < Size; ++i) {
        key = sortBuffer_[i];
        j = i;

        while ((j > 0U) && (sortBuffer_[j - 1U] > key)) {
            sortBuffer_[j] = sortBuffer_[j - 1U];
            --j;
        }

        sortBuffer_[j] = key;
    }

    return sortBuffer_[Size / 2U];
}