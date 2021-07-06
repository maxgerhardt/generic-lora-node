#pragma once
#include <Arduino.h>

/* Helpful String conversion tools */
class StringHelper
{
public:
    static String toHexString(const uint8_t *data, size_t len, bool insert_space_after_byte = false);

    template <typename T>
    static String toFixedLenHexNumber(T value)
    {
        String output = String(value, HEX);
        while(output.length() != 2u*sizeof(T)) {
            output += "0";
        }
        return output;
    }

    static char nibble_to_ascii(uint8_t val);
    static bool decodeHexChar(char c, uint8_t &out);
    unsigned countSpaces(const char* str);
    /* conversts hex strings x = "AB0102" into uint8_t arr[] = {0xAB, 0x01, 0x02} */
    static bool toHexArray(String &str, uint8_t *output_buf, size_t len, size_t *num_written) { return toHexArray(str.c_str(), output_buf, len, num_written); }
    static bool toHexArray(const char *str, uint8_t *output_buf, size_t len, size_t *num_written);
    static bool TryParseInt(String& input, int& output);
    static bool IsHexString(String& input);
};