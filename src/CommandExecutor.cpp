#include <CommandExecutor.h>
#include <string.h>
#include <ConfigManager.h>
#include <GlobalVars.h>
#include <StringHelper.h>
#include <LoRaWANManager.h>

bool has_done_one_transmit = false;

const CommandHandler CommandExecutor::m_handler_table[] = {
    {"erase", &CommandExecutor::HandleErase, "Erases the current node configuration"},
    {"printcfg", &CommandExecutor::HandlePrintCfg, "Prints the current node configuration"},
    {"set_tx_sf", &CommandExecutor::HandleSetTXSF, "Sets the TX spreading factor", "(sf)"},
    {"set_rx2_sf", &CommandExecutor::HandleSetRX2SF, "Sets the SF in RX2", "(sf)"},
    {"set_abp_keys", &CommandExecutor::HandleSetABPKeys, "Sets the keys for Activation by Personalization", "(devaddr) (appskey) (nwskey)"},
    {"set_otaa_keys", &CommandExecutor::HandleSetOTAAKeys, "Sets the keys for Over-the-Air Activation", "(appeui) (deveui) (appkey)"},
    {"set_opmode", &CommandExecutor::HandleSetOpMode, "Sets the to be used activation method", "(0=None/1=LoRaWAN/2=LoRa)"},
    {"set_activation_method", &CommandExecutor::HandleSetActivationMethod, "Sets the to be used activation method", "(0=None/1=ABP/2=OTAA)"},
    {"set_framecnt_up", &CommandExecutor::HandleFramecounterUp, "Sets the uplink framecounter", "(num)"},
    {"set_framecnt_down", &CommandExecutor::HandleFramecounterDown, "Sets the downlink framecounter", "(num)"},
    {"set_otaa_devnonce", &CommandExecutor::HandleSetOTAADevNonce, "Sets the OTAA DevNonce used during join", "(-1 for random or constant)"},
    {"join", &CommandExecutor::HandleStartOTAA, "Starts the OTAA join procedure"},
    {"disconnect", &CommandExecutor::HandleDisconnectOTAA, "Disconnects a device that joined via OTAA for re-joining (with different keys)"},
    {"lorawan_send", &CommandExecutor::HandleLoRaWANSend, "Sends a payload via LoRaWAN", "(fport) (data) (conf.)"},
    {"repeat", &CommandExecutor::HandleRepeat, "Repeats the last LoRaWAN transmission a number of times.", "(num_reps or -1) (delay_secs)"},
    {"check_radio", &CommandExecutor::HandleCheckRadio, "Checks if the LoRa radio is present"},
    {"shutdown", &CommandExecutor::HandleShutdown, "Shuts down the LoRa(WAN) stack"},
    {"reboot", &CommandExecutor::HandleReboot, "Reboot microcontroller"},
    {"help", &CommandExecutor::HandleHelp, "Prints all available commands"}
};
UserInput* CommandExecutor::m_userinput = nullptr;
CommandHandlerState CommandExecutor::m_handler_state = INIT;

void CommandExecutor::Process() {
  if (m_handler_state == INIT)
  {
    m_userinput->print("What action would you like to do? ");
    m_handler_state = WAITING_USER_COMMAND;
  }
  else if (m_handler_state == WAITING_USER_COMMAND)
  {
    static String userCmd = "";
    if (m_userinput->checkInputSream(userCmd))
    {
      m_userinput->println("Your answer was: \"" + userCmd + "\" (length " +
                        userCmd.length() + ")");
      executeCommand(userCmd);
      m_handler_state = INIT; //back to waiting for commands
    }
  }
}

//Splits a possible command into the command name 
void CommandExecutor::SplitCommand(String& cmd, ArgumentList& argsOut) {
    int first_index = cmd.indexOf(' ');
    if(first_index == -1) {
        //has no argument, is just command.
        argsOut.cmdName = cmd;
        argsOut.numArgs = 0;
        return;
    } 
    //extract command name
    argsOut.cmdName = cmd.substring(0, first_index);  
    //initialize number of arguments
    argsOut.numArgs = 0;
    String curArg;
    while(first_index != -1) {
        //find next char or end
        int end_index = cmd.indexOf(' ', first_index + 1);
        //there is no next space found -- first_index to string end is the argument
        if(end_index == -1) {
           curArg = cmd.substring(first_index + 1);
        } else {
           curArg = cmd.substring(first_index + 1, end_index);
        }
        argsOut.numArgs++;
        switch(argsOut.numArgs) {
            case 1: argsOut.arg0 = curArg; break;
            case 2: argsOut.arg1 = curArg; break;
            case 3: argsOut.arg2 = curArg; break;
            default: break; /* more arguments are not supported */
        }
        //m_userinput->println("Parsed argument nr " + String(argsOut.numArgs) + ": \"" + curArg + "\" first_index = " + String(first_index) + " end_index = " + String(end_index));
        //search for next one (or exit loop)
        first_index = end_index;
    }
}

void CommandExecutor::executeCommand(String& cmd) {
    ArgumentList argList;
    SplitCommand(cmd, argList);
#if DEBUG_CMD_PARSING    
    m_userinput->print("Command name: ");
    m_userinput->println(argList.cmdName);
    m_userinput->print("Num args: ");
    m_userinput->println(argList.numArgs);
    m_userinput->print("Arg 0: ");
    m_userinput->println(argList.arg0);
    m_userinput->print("Arg 1: ");
    m_userinput->println(argList.arg1);
    m_userinput->print("Arg 2: ");
    m_userinput->println(argList.arg2);
#endif
    const char* c_str = argList.cmdName.c_str();
    bool command_found = false;
    for(unsigned i = 0; i < sizeof(m_handler_table) / sizeof(m_handler_table[0]); i++) {
        /* handler table is stored in flash, on certain architectures this needs a PROGMEM string compare.. */
        if(strcmp_P(c_str, m_handler_table[i].name) == 0) {
            command_found = true;
            m_handler_table[i].handler(argList);
            break;
        }
    }
    if(!command_found) {
        m_userinput->print(F("Command \""));
        m_userinput->print(cmd);
        m_userinput->println(F("\" not found. Type \"help\" for help."));
    }
}

void CommandExecutor::HandleErase(ArgumentList& args) {
    m_userinput->println(F("Erasing old config and loading default one."));
    ConfigManager::EraseConfig();
    ConfigManager::LoadConfig(cfg);
    m_userinput->println(F("Erased."));
}

void CommandExecutor::HandlePrintCfg(ArgumentList& args) {
    ConfigManager::LoadConfig(cfg);
    ConfigManager::PrintConfig(cfg);
}

void CommandExecutor::HandleHelp(ArgumentList& args) {
    m_userinput->println(F("Available commands: (Commands in round brackets are optional)"));
    for(unsigned i = 0; i < sizeof(m_handler_table) / sizeof(m_handler_table[0]); i++) {
        m_userinput->print("\t");
        m_userinput->print(m_handler_table[i].name);
        int left_col_len = strlen_P(m_handler_table[i].name);
        if(strlen_P(m_handler_table[i].help_cmd_suffix) > 0) {
            m_userinput->print(' ');
            m_userinput->print(m_handler_table[i].help_cmd_suffix);
            left_col_len += strlen_P(m_handler_table[i].help_cmd_suffix) + 1;
        }
        //update this is if longer commands exist
        const int target_length = 45;
        int num_spaces = (target_length - left_col_len);
        if(num_spaces < 0) num_spaces = 0;
        while(num_spaces--) m_userinput->print(' ');
        m_userinput->println(m_handler_table[i].help_description);
    }
}
void CommandExecutor::HandleSetTXSF(ArgumentList& args) {
    int sf = 0; 
    bool do_save = false;
    if(args.numArgs != 0 && args.numArgs != 1) {
        m_userinput->println(F("Command must be called with either 0 or 1 arguments."));
        return;        
    }
    if(args.numArgs == 1) {
        do_save = StringHelper::TryParseInt(args.arg0, sf);     
        if(sf < 7 || sf > 12) {
            m_userinput->println(F("Error: SF must be between 7 and 12."));
            do_save = false;
        }  
    } else if(args.numArgs == 0) { 
        do_save = m_userinput->askForNumberBetween("Enter the SF used for transmission", sf, 7, 12);
    } 
    if(do_save) {
        m_userinput->println("Saving new SF = " + String(sf));
        cfg.lora_last_sf = (uint8_t) sf; 
        ConfigManager::SaveConfig(cfg);
        ReloadLoRaConfig();
    }
}

void CommandExecutor::HandleLoRaWANSend(ArgumentList& args) {
    if(args.numArgs != 0 && args.numArgs != 3  && args.numArgs != 2 /* Fport + Payload + (confirmed) */) {
        m_userinput->println(F("Command must be called with either 0, 2 (Fport + Payload) or 3 (FPort + Payload + Confirmed) arguments."));
        return;        
    }
    uint8_t payload[256] = {};
    size_t payload_len = 0;
    bool confirmed = false;
    uint8_t fport = 0;
    if(args.numArgs >= 2) {
        //get Fport and payload input from arg
        int fport_int = 0;
        if(!StringHelper::TryParseInt(args.arg0, fport_int) || (fport_int < 0 && fport_int > 255)) {
            m_userinput->println(F("First argument (FPort) must be a number between 0 and 255"));
            return;               
        }
        fport = (uint8_t) fport_int;
        if(!StringHelper::toHexArray(args.arg1, payload, sizeof(payload), &payload_len)) {
            m_userinput->println(F("Second argument (payload) must be a hex string."));
            return;               
        }
    } else {
        int fport_int = 0;
        if(!m_userinput->askForNumberBetween("Enter FPort", fport_int, 0, 255)) {
            return;
        }
        fport = (uint8_t) fport_int;
        if(!m_userinput->askForHexBytes("Enter Payload in Hex", payload, sizeof(payload), &payload_len)) {
            return;
        }
    }
    if(args.numArgs == 3) {
        int do_confirmed_up = 0;
        if(!StringHelper::TryParseInt(args.arg2, do_confirmed_up) || (do_confirmed_up != 0 && do_confirmed_up != 1)) {
            m_userinput->println(F("Third argument (confirmed) must be a 0 or 1."));
            return;               
        }
        confirmed = do_confirmed_up == 1;
    } else if(args.numArgs == 2) {
        //only fport and payload given? assume confirmed = false
        confirmed = false;
    } else {
        //get input from commandliie
        int confirmed_int = 0;
        if(!m_userinput->askForNumberBetween("Confirmed Uplink?", confirmed_int, 0, 1)) {
            return;
        }
        confirmed = confirmed_int == 1;
    }

    m_userinput->println(F("Transmission params:"));
    m_userinput->print(F("\tFport: "));
    m_userinput->println(fport);
    m_userinput->print(F("\tPayload: "));
    m_userinput->println(StringHelper::toHexString(payload, payload_len, true));
    m_userinput->print(F("\tConfirmed up: "));
    m_userinput->println(confirmed);
    LoRaWANManager::ScheduleTX(fport, payload, payload_len, confirmed);   
    cfg.op_mode = OperationMode::OPMODE_LORAWAN;
    has_done_one_transmit = true;
}

void CommandExecutor::HandleCheckRadio(ArgumentList& args) {
    /* reset */
    m_userinput->println("Resetting radio"); 
    m_userinput->getStream()->flush();

#ifdef CFG_sx1276_radio
    hal_pin_rst(0); // drive RST pin low
#else
    hal_pin_rst(1); // drive RST pin high
#endif
    delay(1);
    hal_pin_rst(2); // configure RST pin floating!
    delay(6);

    u1_t addr = 0x42; /* reg version */
    u1_t buf[1];
    m_userinput->println("Reading radio version from register address 0x" + String(addr, HEX)); 
    m_userinput->getStream()->flush();
    hal_spi_read(addr & 0x7f, buf, 1);
    uint8_t read_version = buf[0];
    m_userinput->println("Read radio version: 0x" + String(read_version, HEX));
    if(read_version == 0x12) {
        m_userinput->println("Detected SX1272 radio.");
    } else if(read_version == 0x22) {
        m_userinput->println("Detected SX1276 radio.");
    } else {
        m_userinput->println("Unknown radio type..");
    }
}

void CommandExecutor::ReloadLoRaConfig() {
    if(cfg.op_mode == OperationMode::OPMODE_LORAWAN) {
        LoRaWANManager::ApplyLoRaWANConfig(cfg);
    }
}

void CommandExecutor::HandleShutdown(ArgumentList& args) {
    LoRaWANManager::Detach();
    cfg.op_mode = OperationMode::OPMODE_NONE;
}

void CommandExecutor::HandleSetABPKeys(ArgumentList& args) {
    if(args.numArgs != 0 && args.numArgs != 3) {
        m_userinput->println(F("Command must be called with either 0 or 3 arguments."));
        return;        
    }
    uint8_t tmp_devaddr[4] = {};
    uint8_t tmp_appskey[16] = {};
    uint8_t tmp_nwskey[16] = {};
    uint32_t dev_addr;
    if(args.numArgs == 0) {
        if(!m_userinput->askForNHexBytes("Enter Device Address", tmp_devaddr, sizeof(tmp_devaddr), 4, true)) {
            return;
        }
        if(!m_userinput->askForNHexBytes("Enter AppSKey", tmp_appskey, sizeof(tmp_appskey), 16, true)) {
            return;
        }
        if(!m_userinput->askForNHexBytes("Enter NwkSkey", tmp_nwskey, sizeof(tmp_nwskey), 16, true)) {
            return;
        }
    } else {
        size_t decoded_len = 0;
        /* 3 arguments were supplied */
        if(!StringHelper::toHexArray(args.arg0, tmp_devaddr, sizeof(tmp_devaddr), &decoded_len)) {
            m_userinput->println(F("First argument (Device Address) must be a hex string."));
            return;               
        }
        if(decoded_len != 4) {
            m_userinput->println(F("First argument (Device Address) must be 4 bytes."));
            return;               
        }
        if(!StringHelper::toHexArray(args.arg1, tmp_appskey, sizeof(tmp_appskey), &decoded_len)) {
            m_userinput->println(F("Second argument (AppSKey) must be a hex string."));
            return;               
        }
        if(decoded_len != 16) {
            m_userinput->println("Second argument (AppSKey) must be 16 bytes, was " + String(decoded_len));
            return;               
        }
        if(!StringHelper::toHexArray(args.arg2, tmp_nwskey, sizeof(tmp_nwskey), &decoded_len)) {
            m_userinput->println(F("Third argument (NwkSkey) must be a hex string."));
            return;               
        }
        if(decoded_len != 16) {
            m_userinput->println("Third argument (NwkSkey) must be 16 bytes, was " + String(decoded_len));
            return;               
        }
    }
    //decode hex byte to 32-bit integer
    dev_addr = (uint32_t)((tmp_devaddr[0] << 24u) | (tmp_devaddr[1] << 16u) | (tmp_devaddr[2] << 8u) | tmp_devaddr[3]);
    //print back as confirmation
    m_userinput->print(F("Read device address: 0x"));
    m_userinput->println(StringHelper::toFixedLenHexNumber(dev_addr));
    m_userinput->print(F("Read AppSKey: "));
    m_userinput->println(StringHelper::toHexString(tmp_appskey, sizeof(tmp_appskey), true));
    m_userinput->print(F("Read NwkSkey: "));
    m_userinput->println(StringHelper::toHexString(tmp_nwskey, sizeof(tmp_nwskey), true));

    cfg.op_mode = OperationMode::OPMODE_LORAWAN;
    cfg.last_activation_method = LoRaWANActivationmethod::ACTIVATION_METHOD_ABP;
    cfg.lorawan_abp_dev_addr = dev_addr;
    memcpy(cfg.lorawan_abp_appskey, tmp_appskey, sizeof(tmp_appskey));
    memcpy(cfg.lorawan_abp_nwskey, tmp_nwskey, sizeof(tmp_nwskey));
    ConfigManager::SaveConfig(cfg);
    ReloadLoRaConfig();
    m_userinput->println("Saved and reloaded new configuration.");
}

void CommandExecutor::HandleSetOTAAKeys(ArgumentList& args) {
    if(args.numArgs != 0 && args.numArgs != 3) {
        m_userinput->println(F("Command must be called with either 0 or 3 arguments."));
        return;        
    }
    uint8_t tmp_appeui[8] = {};
    uint8_t tmp_deveui[8] = {};
    uint8_t tmp_appkey[16] = {};
    if(args.numArgs == 0) {
        if(!m_userinput->askForNHexBytes("Enter AppEUI", tmp_appeui, sizeof(tmp_appeui), 8, true)) {
            return;
        }
        if(!m_userinput->askForNHexBytes("Enter DevEUI", tmp_deveui, sizeof(tmp_deveui), 8, true)) {
            return;
        }
        if(!m_userinput->askForNHexBytes("Enter AppKey", tmp_appkey, sizeof(tmp_appkey), 16, true)) {
            return;
        }
    } else {
        size_t decoded_len = 0;
        /* 3 arguments were supplied */
        if(!StringHelper::toHexArray(args.arg0, tmp_appeui, sizeof(tmp_appeui), &decoded_len)) {
            m_userinput->println(F("First argument (AppEUI) must be a hex string."));
            return;               
        }
        if(decoded_len != 8) {
            m_userinput->println(F("First argument (AppEUI) must be 8 bytes."));
            return;               
        }
        if(!StringHelper::toHexArray(args.arg1, tmp_deveui, sizeof(tmp_deveui), &decoded_len)) {
            m_userinput->println(F("Second argument (DevEUI) must be a hex string."));
            return;               
        }
        if(decoded_len != 8) {
            m_userinput->println("Second argument (DevEUI) must be 8 bytes, was " + String(decoded_len));
            return;               
        }
        if(!StringHelper::toHexArray(args.arg2, tmp_appkey, sizeof(tmp_appkey), &decoded_len)) {
            m_userinput->println(F("Third argument (AppKey) must be a hex string."));
            return;               
        }
        if(decoded_len != 16) {
            m_userinput->println("Third argument (AppKey) must be 16 bytes, was " + String(decoded_len));
            return;               
        }
    }
    //print back as confirmation
    m_userinput->print(F("Read AppEUI: "));
    m_userinput->println(StringHelper::toHexString(tmp_appeui, sizeof(tmp_appeui), true));
    m_userinput->print(F("Read DevEUI: "));
    m_userinput->println(StringHelper::toHexString(tmp_deveui, sizeof(tmp_deveui), true));
    m_userinput->print(F("Read AppKey: "));
    m_userinput->println(StringHelper::toHexString(tmp_appkey, sizeof(tmp_appkey), true));

    cfg.op_mode = OperationMode::OPMODE_LORAWAN;
    cfg.last_activation_method = LoRaWANActivationmethod::ACTIVATION_METHOD_OTAA;
    memcpy(cfg.lorawan_otaa_appeui, tmp_appeui, sizeof(tmp_appeui));
    memcpy(cfg.lorawan_otaa_deveui, tmp_deveui, sizeof(tmp_deveui));
    memcpy(cfg.lorawan_otaa_appkey, tmp_appkey, sizeof(tmp_appkey));
    ConfigManager::SaveConfig(cfg);
    ReloadLoRaConfig();
    m_userinput->println("Saved and reloaded new configuration.");
}

void CommandExecutor::HandleSetActivationMethod(ArgumentList& args) {
    int activ_method = 0; 
    bool do_save = false;
    if(args.numArgs != 0 && args.numArgs != 1) {
        m_userinput->println(F("Command must be called with either 0 or 1 arguments."));
        return;        
    }
    if(args.numArgs == 1) {
        do_save = StringHelper::TryParseInt(args.arg0, activ_method);     
        if(activ_method < 0 || activ_method > 2) {
            m_userinput->println(F("Error: Activation method must be between 0 and 2."));
            do_save = false;
        }
    } else if(args.numArgs == 0) { 
        do_save = m_userinput->askForNumberBetween("Enter the Activation method (0=None,1=ABP,OTAA=2)", activ_method, 0, 2);
    } 
    if(do_save) {
        m_userinput->println("Saving new activation method = " + String(activ_method));
        cfg.last_activation_method = (LoRaWANActivationmethod) activ_method; 
        ConfigManager::SaveConfig(cfg);
        ReloadLoRaConfig();
    }
}

void CommandExecutor::HandleStartOTAA(ArgumentList& args) {
    if(cfg.last_activation_method != LoRaWANActivationmethod::ACTIVATION_METHOD_OTAA) {
        m_userinput->println("Can only join when using OTAA.");
    } else {
        //test: use highest Dr directly..
        LMICcore_setDrJoin(DRCHG_SET, DR_SF12);
        LMIC_startJoining();
        LMICcore_setDrJoin(DRCHG_SET, DR_SF12);
    }
}

void CommandExecutor::HandleDisconnectOTAA(ArgumentList& args) {
    if(cfg.last_activation_method != LoRaWANActivationmethod::ACTIVATION_METHOD_OTAA) {
        m_userinput->println("Can only disconnect when using OTAA.");
    } else {
        LMIC_unjoin();
    }
}

void CommandExecutor::HandleReboot(ArgumentList& args) {
    m_userinput->println(F("Restarting microcontroller.."));
    m_userinput->getStream()->flush();
#if defined(__arm__)
    NVIC_SystemReset();
#endif
}

void CommandExecutor::HandleSetRX2SF(ArgumentList& args) {
    int sf = 0; 
    bool do_save = false;
    if(args.numArgs != 0 && args.numArgs != 1) {
        m_userinput->println(F("Command must be called with either 0 or 1 arguments."));
        return;        
    }
    if(args.numArgs == 1) {
        do_save = StringHelper::TryParseInt(args.arg0, sf);     
        if(sf < 7 || sf > 12) {
            m_userinput->println(F("Error: SF must be between 7 and 12."));
            do_save = false;
        }  
    } else if(args.numArgs == 0) { 
        do_save = m_userinput->askForNumberBetween("Enter the SF used for RX2 window", sf, 7, 12);
    } 
    if(do_save) {
        m_userinput->println("Saving new RX2 SF = " + String(sf));
        cfg.lorawan_rx2_sf = (uint8_t) sf; 
        ConfigManager::SaveConfig(cfg);
        ReloadLoRaConfig();
    }
}

void CommandExecutor::HandleFramecounterUp(ArgumentList& args) {
    int framecnt_up = 0; 
    bool do_save = false;
    if(args.numArgs != 0 && args.numArgs != 1) {
        m_userinput->println(F("Command must be called with either 0 or 1 arguments."));
        return;        
    }
    if(args.numArgs == 1) {
        do_save = StringHelper::TryParseInt(args.arg0, framecnt_up);     

    } else if(args.numArgs == 0) { 
        do_save = m_userinput->askForNumber("Enter the uplink framecounter", framecnt_up);
    } 
    if(do_save) {
        m_userinput->println("Saving new uplink framecounter = " + String(framecnt_up));
        cfg.lorawan_framecnt_up = (uint32_t) framecnt_up; 
        ConfigManager::SaveConfig(cfg);
        ReloadLoRaConfig();
    }
}

void CommandExecutor::HandleFramecounterDown(ArgumentList& args) {
    int framecnt_down = 0; 
    bool do_save = false;
    if(args.numArgs != 0 && args.numArgs != 1) {
        m_userinput->println(F("Command must be called with either 0 or 1 arguments."));
        return;        
    }
    if(args.numArgs == 1) {
        do_save = StringHelper::TryParseInt(args.arg0, framecnt_down);     

    } else if(args.numArgs == 0) { 
        do_save = m_userinput->askForNumber("Enter the downlink framecounter", framecnt_down);
    } 
    if(do_save) {
        m_userinput->println("Saving new downlink framecounter = " + String(framecnt_down));
        cfg.lorawan_framecnt_down = (uint32_t) framecnt_down; 
        ConfigManager::SaveConfig(cfg);
        ReloadLoRaConfig();
    }
}

void CommandExecutor::HandleSetOpMode(ArgumentList& args) {
    int activ_method = 0; 
    bool do_save = false;
    if(args.numArgs != 0 && args.numArgs != 1) {
        m_userinput->println(F("Command must be called with either 0 or 1 arguments."));
        return;        
    }
    if(args.numArgs == 1) {
        do_save = StringHelper::TryParseInt(args.arg0, activ_method);     
        if(activ_method < 0 || activ_method > 2) {
            m_userinput->println(F("Error: Operation mode must be between 0 and 2."));
            do_save = false;
        }
    } else if(args.numArgs == 0) { 
        do_save = m_userinput->askForNumberBetween("Enter the Operation mode (0=None,1=LoRaWAN/2=LoRa)", activ_method, 0, 2);
    } 
    if(do_save) {
        m_userinput->println("Saving new operation mode = " + String(activ_method));
        cfg.op_mode = (OperationMode) activ_method; 
        ConfigManager::SaveConfig(cfg);
        ReloadLoRaConfig();
    }
}

void CommandExecutor::HandleRepeat(ArgumentList& args) {
    int num_transmissions = 0;
    int delay_seconds = 0;
    bool is_ok = false;
    if(!has_done_one_transmit) {
        m_userinput->println(F("Cannot repeat a transmission until at least one transmission is done."));
        return;        
    }
    if(args.numArgs != 0 && args.numArgs != 2) {
        m_userinput->println(F("Command must be called with either 0 or 2 arguments."));
        return;        
    }
    if(args.numArgs == 2) {
        is_ok = StringHelper::TryParseInt(args.arg0, num_transmissions); 
        if(!is_ok) {
            m_userinput->println(F("Failed to parse argument 0 as int."));
            return;
        }  
        if(!(num_transmissions == -1 || num_transmissions > 0 )) {
            m_userinput->println(F("Error: Number of transmissions must be either be -1 (infinite) or a positive number."));
            m_userinput->println("Was: " + String(num_transmissions));
            return;
        }
        is_ok = StringHelper::TryParseInt(args.arg1, delay_seconds);
        if(!is_ok) {
            m_userinput->println(F("Can't parse argument 1 as int."));
            return;
        }   
        if(delay_seconds < 0 ) {
            m_userinput->println(F("Error: Delay between transmissions must be positive."));
            is_ok = false;
            return;
        }
    } else if(args.numArgs == 0) { 
        is_ok = m_userinput->askForNumber("Enter the number of repititions (-1 for infinite)", num_transmissions);
        if(!(num_transmissions == -1 || num_transmissions > 0 )) {
            m_userinput->println(F("Error: Number of transmissions must be either be -1 (infinite) or a positive number."));
            is_ok = false;
            return;
        }
        is_ok = m_userinput->askForNumber("Enter the delay in milliseconds before starting the next transmission", delay_seconds);
        if(delay_seconds < 0) {
            is_ok = false;
            m_userinput->println(F("Error: Delay between transmissions must be positive."));
            return;
        }
    } 
    if(is_ok) {
        m_userinput->println("Repeating last transmission " + (num_transmissions == -1 ? String("infinite") : String(num_transmissions)) + " times, delay " + String(delay_seconds) + " seconds.");
        auto* tx = LoRaWANManager::GetLastTX();
        m_userinput->println("Transmission data: " +  StringHelper::toHexString(tx->payload, tx->payload_len) +  " Fport " + String(tx->fport));
        LoRaWANManager::RepeatLast(num_transmissions, delay_seconds);
    }
}

void CommandExecutor::HandleSetOTAADevNonce(ArgumentList& args) {
    int otaa_dev_nonnce = 0; 
    bool do_save = false;
    if(args.numArgs != 0 && args.numArgs != 1) {
        m_userinput->println(F("Command must be called with either 0 or 1 arguments."));
        return;        
    }
    if(args.numArgs == 1) {
        do_save = StringHelper::TryParseInt(args.arg0, otaa_dev_nonnce);     

    } else if(args.numArgs == 0) { 
        do_save = m_userinput->askForNumber("Enter the OTAA device nonce to use (-1 for random or constant value)", otaa_dev_nonnce);
    } 
    if(do_save) {
        if(otaa_dev_nonnce < -1 || otaa_dev_nonnce > 65535) {
            m_userinput->println("Error, device nonce must either be -1 or a 16-bit integer (0-65535)), was \"" + String(otaa_dev_nonnce) + "\".");
            return;
        }
        m_userinput->println("Setting new device nonce choice: " + (otaa_dev_nonnce == -1 ? String("random")  : StringHelper::toFixedLenHexNumber(otaa_dev_nonnce)));
        cfg.devnonce_choice = otaa_dev_nonnce;
        ConfigManager::SaveConfig(cfg);
        ReloadLoRaConfig();
    }
}