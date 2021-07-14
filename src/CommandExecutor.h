#pragma once
#include <UserInput.h>

#define MAX_COMMAND_LEN 16 

/* convienence structure for command decoding */
struct ArgumentList {
    String cmdName;
    String arg0;
    String arg1;
    String arg2;
    uint8_t numArgs;
};

typedef void (*cmd_handler_t)(ArgumentList& args);

struct CommandHandler {
    const char name[22];
    cmd_handler_t handler;
    const char help_description[80];
    const char help_cmd_suffix[32];
};

enum CommandHandlerState
{
  INIT,
  WAITING_USER_COMMAND
};

class CommandExecutor {
public:
    static void Init(UserInput* input) { m_userinput = input; }
    static void Process();
    static void executeCommand(String& cmd);
private:
    static void SplitCommand(String& cmd, ArgumentList& argsOut);
    static void ReloadLoRaConfig();

    /* handler function implementations */
    static void HandleErase(ArgumentList& args);
    static void HandlePrintCfg(ArgumentList& args);
    static void HandleHelp(ArgumentList& args);
    /* LoRa Mac settings  */
    static void HandleSetTXSF(ArgumentList& args);
    static void HandleLoRaWANSend(ArgumentList& args);
    static void HandleRepeat(ArgumentList& args);
    static void HandleCheckRadio(ArgumentList& args);
    static void HandleShutdown(ArgumentList& args);
    /* ABP/OTAA settings and methods */
    static void HandleSetABPKeys(ArgumentList& args);
    static void HandleSetOTAAKeys(ArgumentList& args);
    static void HandleSetActivationMethod(ArgumentList& args);
    static void HandleStartOTAA(ArgumentList& args);
    static void HandleDisconnectOTAA(ArgumentList& args);
    static void HandleReboot(ArgumentList& args);
    static void HandleSetRX2SF(ArgumentList& args);
    static void HandleFramecounterUp(ArgumentList& args);
    static void HandleFramecounterDown(ArgumentList& args);
    static void HandleSetOpMode(ArgumentList& args);
    static void HandleSetOTAADevNonce(ArgumentList& args);

    static const CommandHandler m_handler_table[];
    static UserInput* m_userinput;
    static CommandHandlerState m_handler_state;
};