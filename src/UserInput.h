#pragma once

#include <Arduino.h> /* string.h etc. */

enum QuestionFlags
{
    RETURN_FALSE_ON_FAILURE = 1, /* bit mask */
    ASK_AGAIN_ON_FAILURE = 2
};

#define TIMEOUT_INFINITE -1

class UserInput
{
public:
    UserInput(Stream &stream) : m_stream(&stream) {}

    bool askForString(String &question, String &answer, QuestionFlags flags = RETURN_FALSE_ON_FAILURE, int timeout = TIMEOUT_INFINITE);
    bool askForString(String &&question, String &answer, QuestionFlags flags = RETURN_FALSE_ON_FAILURE, int timeout = TIMEOUT_INFINITE);
    bool askForNumber(String &&question, int &answer, QuestionFlags flags = RETURN_FALSE_ON_FAILURE, int timeout = TIMEOUT_INFINITE);
    bool askForNumberBetween(String &&question, int &answer, int start_inclusive, int end_inclusive, QuestionFlags flags = RETURN_FALSE_ON_FAILURE, int timeout = TIMEOUT_INFINITE);
    bool askForLongNumber(String &&question, long &answer, QuestionFlags flags = RETURN_FALSE_ON_FAILURE, int timeout = TIMEOUT_INFINITE);
    bool askForHexBytes(String &&question, uint8_t *answer_buf, size_t buf_len, size_t *num_written, bool skip_spaces = false, QuestionFlags flags = RETURN_FALSE_ON_FAILURE, int timeout = TIMEOUT_INFINITE);
    bool askForNHexBytes(String &&question, uint8_t *answer_buf, size_t buf_len, size_t expected_num_bytes, bool skip_spaces = false, QuestionFlags flags = RETURN_FALSE_ON_FAILURE, int timeout = TIMEOUT_INFINITE);

    template <typename T>
    void println(T &&text) { m_stream->println(text); }
    template <typename T>
    void println(T &text) { m_stream->println(text); }
    template <typename T>
    void print(T &&text) { m_stream->print(text); }
    template <typename T>
    void print(T &text) { m_stream->print(text); }
    Stream *getStream() { return m_stream; }

    /* return value: string complete (newline found) 
     * interrupt / non-blocking way to read user data in the 
     * main loop without blocking.
     */
    bool checkInputSream(String& output);
private:
    bool readInput(String &output, int readTimeout, bool only_1_char = false);
    void removeNewlines(String &input);

    Stream *m_stream;
    String m_isr_input; /* extra internal variable for storing data found in interrupt-way */ 

};