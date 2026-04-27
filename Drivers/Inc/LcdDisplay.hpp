#ifndef LCD_DISPLAY_HPP
#define LCD_DISPLAY_HPP

#include <stdint.h>

class LcdDisplay
{
public:
    LcdDisplay();

    void initialize();
    void clear();
    void setCursor(uint8_t row, uint8_t column);
    void writeChar(char c);
    void writeString(const char* text);
    void createChar(uint8_t location, const uint8_t* charmap);
    void writeFixed(int32_t value, uint8_t fracDigits);
    void test();

private:
    static constexpr uint8_t kBufferSize = 20U;
    char finalBuffer_[kBufferSize];

    static void rsHigh();
    static void rsLow();
    static void rwLow();
    static void eHigh();
    static void eLow();
    static void backlightHigh();

    static void writeDb4To7(uint8_t nibble);
    static void send4(uint8_t nibble);

    static void command(uint8_t cmd);
    static void data(uint8_t value);

    static uint32_t pow10(uint8_t exp);
};

#endif