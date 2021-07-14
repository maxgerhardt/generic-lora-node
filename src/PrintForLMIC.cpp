#include <Arduino.h>
#include <cstdarg>
#include <GlobalVars.h>
#include <StringHelper.h>

extern "C" void Arduino_Println(const char *fmt, ...)
{
    va_list vl;
    va_start(vl, fmt);
    static char output_buf[128];
    int num_bytes = vsnprintf(output_buf, sizeof(output_buf), fmt, vl);
    if (num_bytes > 0)
    {
        userInput.print(output_buf);
    }
    va_end(vl);
}

extern "C" void Arduino_PrintHex(const char* name, const void* data, size_t len)
{
    userInput.print(name);
    userInput.print(": ");
    userInput.println(StringHelper::toHexString((const uint8_t*)data, len, true));
}