#include <StringHelper.h>

String StringHelper::toHexString(const uint8_t *data, size_t len, bool insert_space_after_byte)
{
    String output = "";
    for (size_t i = 0; i < len; i++)
    {
        output += nibble_to_ascii((uint8_t)(data[i] >> 4));
        output += nibble_to_ascii((uint8_t)(data[i] & 0xf));
        if(insert_space_after_byte && (i != len - 1))
            output += " ";
    }
    return output;
}

char StringHelper::nibble_to_ascii(uint8_t val)
{
    if (val >= 0x0 && val <= 0x9)
        return (char)('0' + val);
    else
        return (char)('a' + val - 10);
}

bool StringHelper::decodeHexChar(char c, uint8_t &out)
{
    uint8_t value = 0;
    if (c >= '0' && c <= '9')
        value = (uint8_t)(c - '0');
    else if (c >= 'A' && c <= 'F')
        value = (uint8_t)(10 + (c - 'A'));
    else if (c >= 'a' && c <= 'f')
        value = (uint8_t)(10 + (c - 'a'));
    else
    {
        //invalid hex char
        return false;
    }
    out = value;
    return true;
}

unsigned StringHelper::countSpaces(const char* str) {
    unsigned count = 0;
    for(unsigned i=0; i < strlen(str); i++) {
        if(str[i] == ' ')
            count++;
    }
    return count;
}

bool StringHelper::toHexArray(const char *str, uint8_t *output_buf, size_t len, size_t *num_written)
{
    if (output_buf == nullptr || len == 0)
    {
        return false;
    }
    if (num_written != nullptr)
    {
        *num_written = 0;
    }
    size_t str_len = strlen(str);
    if (str_len % 2 != 0)
    {
        //can't convert uneven number of hex chars into array.
        return false;
    }
    /* a hex string of length n produces a byte array of length n/2. we need at least that many bytes. */
    if (len < str_len / 2)
        return false;

    for (size_t i = 0; i < str_len; i += 2)
    {
        uint8_t value1 = 0, value2 = 0;
        if (!decodeHexChar(str[i], value1))
        {
            return false;
        }
        if (!decodeHexChar(str[i + 1], value2))
        {
            return false;
        }
        //two writes to the same output buffer
        output_buf[i / 2] = (uint8_t)((value1 << 4u) | value2);
        if (num_written != nullptr)
        {
            (*num_written)++;
        }
    }

    return true;
}

bool StringHelper::TryParseInt(String& input, int& output) {
    /* check if input is fully numeric (with the exception of a minus at the start), then convert */
    for(unsigned i=0; i < input.length(); i++) {
        char c = input.charAt(i);
        if((c < '0' || c > '9')) {
            //non-numeric is only okay in the first iteration
            if(c != '-' || i != 0) {
                return false;
            }
        }
    }
    output = (int) input.toInt();
    return true;
}

bool StringHelper::TryParseLong(String& input, long& output) {
    /* check if input is fully numeric (with the exception of a minus at the start), then convert */
    for(unsigned i=0; i < input.length(); i++) {
        char c = input.charAt(i);
        if((c < '0' || c > '9')) {
            //non-numeric is only okay in the first iteration
            if(c != '-' || i != 0) {
                return false;
            }
        }
    }
    output = input.toInt();
    return true;
}

bool StringHelper::IsHexString(String& input) {
    //string length must be even
    if(input.length() % 2 != 0) {
        return false;
    }
    for(unsigned i=0; i < input.length(); i++) {
        char c = input.charAt(i);
        bool is_number = (c >= '0') && (c <= '9');
        bool is_lowercase_hex = (c >= 'a') && (c <= 'f');
        bool is_uppercase_hex = (c >= 'A') && (c <= 'F');
        if(!is_number && !is_lowercase_hex && !is_uppercase_hex) {
            return false;
        }
    }
    return true;
}
