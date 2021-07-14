#include <UserInput.h>
#include <limits.h>
#include <StringHelper.h>

void UserInput::removeNewlines(String& input) {
    input.replace("\r", "");
}

bool UserInput::readInput(String &output, int readTimeout, bool only_1_char)
{
    m_stream->setTimeout(readTimeout < 0 ? ULONG_MAX : (unsigned long)readTimeout);
    if(!only_1_char)
        output = "";
    bool newline_found = false;
    while(true) {
        char as_char = '\0';
        size_t num_read = m_stream->readBytes(&as_char, 1);
        if(num_read == 0) {
            /* timeout */
            break;
        } else {
            //echo back every read character
            m_stream->print(as_char);
            //m_stream->print(String(as_char) + " (hex " + String(as_char, HEX)+ ")");
            if(as_char == '\n') {
                newline_found = true;
                break;
            } else if(as_char == '\b' /* backspace */) {
                if(output.length() > 0) {
                    m_stream->write(" \b", 2);
                    output.remove(output.length() - 1, 1);
                } else {
                    m_stream->write(' ');
                }
            } else if(as_char == '\r') {
                /* do not save carriage return */
            } else {
                output += as_char;
            }
        }
        if(only_1_char) 
            break;
    }
    return newline_found;
}

bool UserInput::askForString(String &&question, String &answer, QuestionFlags flags, int timeout) {
    return askForString(question, answer, flags, timeout);
}

bool UserInput::askForString(String &question, String &answer, QuestionFlags flags, int timeout)
{
    bool read_ok = false;
    do
    {
        m_stream->print(question);
        m_stream->print(' ');
        m_stream->flush();
        read_ok = readInput(answer, timeout);
        if(!read_ok) {
            m_stream->println("Timeout!");
        }
    } while (!read_ok && (flags & ASK_AGAIN_ON_FAILURE) != 0);
    return read_ok;
}

bool UserInput::checkInputSream(String& output) {
    bool newline_found = readInput(m_isr_input, 0, true);
    if(newline_found) {
        output = m_isr_input; 
        m_isr_input = "";
        return true;
    }
    return false;
}

bool UserInput::askForNumber(String &&question, int &answer, QuestionFlags flags, int timeout) {
    String strAnswer;
    bool read_ok = false;
    do
    {
        read_ok = askForString(question, strAnswer, flags, timeout);
        if(!read_ok) {
            /* timeout */
        } else {
            read_ok = StringHelper::TryParseInt(strAnswer, answer);
            if(!read_ok) {
                m_stream->println("Answer \"" + strAnswer + "\" is not a number..");
            }
        }
    } while (!read_ok && (flags & ASK_AGAIN_ON_FAILURE) != 0);
    return read_ok;    
}

bool UserInput::askForLongNumber(String &&question, long &answer, QuestionFlags flags, int timeout) {
    String strAnswer;
    bool read_ok = false;
    do
    {
        read_ok = askForString(question, strAnswer, flags, timeout);
        if(!read_ok) {
            /* timeout */
        } else {
            read_ok = StringHelper::TryParseLong(strAnswer, answer);
            if(!read_ok) {
                m_stream->println("Answer \"" + strAnswer + "\" is not a number..");
            }
        }
    } while (!read_ok && (flags & ASK_AGAIN_ON_FAILURE) != 0);
    return read_ok;    
}

bool UserInput::askForNumberBetween(String &&question, int &answer, int start_inclusive, int end_inclusive, QuestionFlags flags, int timeout) {
    String strAnswer;
    bool read_ok = false;
    question += " [" + String(start_inclusive) + "-" + String(end_inclusive) + "]";
    do
    {
        read_ok = askForString(question, strAnswer, flags, timeout);
        if(!read_ok) {
            /* timeout */
        } else {
            read_ok = StringHelper::TryParseInt(strAnswer, answer);
            if(!read_ok) {
                m_stream->println("Answer \"" + strAnswer + "\" is not a number..");
            }
            if(answer < start_inclusive || answer > end_inclusive) {
                read_ok = false;
                m_stream->println("Answer \"" + strAnswer + "\" not within bounds " + String(start_inclusive) + " to " + String(end_inclusive));
            } 
        }
    } while (!read_ok && (flags & ASK_AGAIN_ON_FAILURE) != 0);
    return read_ok; 
}

bool UserInput::askForHexBytes(String &&question, uint8_t *answer_buf, size_t buf_len, size_t *num_written, bool skip_spaces, QuestionFlags flags, int timeout) {
    String strAnswer;
    bool read_ok = false;
    do
    {
        read_ok = askForString(question, strAnswer, flags, timeout);
        if(!read_ok) {
            /* timeout */
        } else {
            if(skip_spaces) 
                strAnswer.replace(" ", "");
            read_ok = StringHelper::toHexArray(strAnswer, answer_buf, buf_len, num_written);
            if(!read_ok) {
                m_stream->println("Answer \"" + strAnswer + "\" is not a valid hex string or exceeds buffer length..");
            }
        }
    } while (!read_ok && (flags & ASK_AGAIN_ON_FAILURE) != 0);
    return read_ok; 
}

bool UserInput::askForNHexBytes(String &&question, uint8_t *answer_buf, size_t buf_len, size_t expected_num_bytes, bool skip_spaces, QuestionFlags flags, int timeout) {
    String strAnswer;
    bool read_ok = false;
    question += " [" + String(expected_num_bytes) + " Bytes]";
    do
    {
        read_ok = askForString(question, strAnswer, flags, timeout);
        if(!read_ok) {
            /* timeout */
        } else {
            if(skip_spaces) 
                strAnswer.replace(" ", "");
            if(strAnswer.length() / 2 != expected_num_bytes) {
                read_ok = false;
                m_stream->println("Answer \"" + strAnswer + "\" has not the right length to be hex-encoding the expected " + String(expected_num_bytes) + "bytes.");
            } else {
                size_t num_written;
                read_ok = StringHelper::toHexArray(strAnswer, answer_buf, buf_len, &num_written);
                if(!read_ok) {
                    m_stream->println("Answer \"" + strAnswer + "\" is not a valid hex string or exceeds buffer length..");
                } else {
                    //should be impossible to reach but better check anyways
                    if(num_written != expected_num_bytes) {
                        read_ok = false;
                        m_stream->println("Answer \"" + strAnswer + "\" is not a hex string encoding the expected " + String(expected_num_bytes) + "bytes.");
                    } 
                }
            }
        }
    } while (!read_ok && (flags & ASK_AGAIN_ON_FAILURE) != 0);
    return read_ok; 
}
