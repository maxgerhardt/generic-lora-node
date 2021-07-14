#include <LoRaWANManager.h>
#include <arduino_lmic.h>
#include <GlobalVars.h>
#include <StringHelper.h>

TXSettings LoRaWANManager::last_tx = {};
osjob_t LoRaWANManager::repeat_job;

/* reference 'secret' LMIC stack variable */
extern "C" int lmic_devnonce_choice;

bool LoRaWANManager::Init(GenericNodeConfig& cfg) {
    os_init();
    LMIC_reset();
    ApplyLoRaWANConfig(cfg);
    return true;
}

void LoRaWANManager::ApplyLoRaWANConfig(GenericNodeConfig& cfg) {
    // copy keys into internal LMIC buffers
    //netid = 0x13 for TTN, see https://www.thethingsnetwork.org/docs/lorawan/prefix-assignments/
    if(cfg.last_activation_method == LoRaWANActivationmethod::ACTIVATION_METHOD_ABP) {
        userInput.println("Setting ABP session keys.");
        LMIC_setSession (0x13, cfg.lorawan_abp_dev_addr, cfg.lorawan_abp_nwskey, cfg.lorawan_abp_appskey);
    } else if(cfg.last_activation_method == LoRaWANActivationmethod::ACTIVATION_METHOD_OTAA) {
        userInput.println("Resetting stack os that OTAA gets used.");
        /* do nothing.. how does LMIC know we want to use OTAA? */
        //reset.. I guess
        LMIC_reset();
    } else {
        userInput.println("Not loading any keys., method = " + String(cfg.last_activation_method));
    }

    //setup all channels here. 
    #if defined(CFG_eu868)
    LMIC_setupChannel(0, 868100000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      // g-band
    LMIC_setupChannel(1, 868300000, DR_RANGE_MAP(DR_SF12, DR_SF7B), BAND_CENTI);      // g-band
    LMIC_setupChannel(2, 868500000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      // g-band
    LMIC_setupChannel(3, 867100000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      // g-band
    LMIC_setupChannel(4, 867300000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      // g-band
    LMIC_setupChannel(5, 867500000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      // g-band
    LMIC_setupChannel(6, 867700000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      // g-band
    LMIC_setupChannel(7, 867900000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      // g-band
    LMIC_setupChannel(8, 868800000, DR_RANGE_MAP(DR_FSK,  DR_FSK),  BAND_MILLI);      // g2-band
    // TTN defines an additional channel at 869.525Mhz using SF9 for class B
    // devices' ping slots. LMIC does not have an easy way to define set this
    // frequency and support for class B is spotty and untested, so this
    // frequency is not configured here.
    #elif defined(CFG_us915) || defined(CFG_au915)
    // NA-US and AU channels 0-71 are configured automatically
    // but only one group of 8 should (a subband) should be active
    // TTN recommends the second sub band, 1 in a zero based count.
    // https://github.com/TheThingsNetwork/gateway-conf/blob/master/US-global_conf.json
    LMIC_selectSubBand(1);
    #elif defined(CFG_as923)
    // Set up the channels used in your country. Only two are defined by default,
    // and they cannot be changed.  Use BAND_CENTI to indicate 1% duty cycle.
    // LMIC_setupChannel(0, 923200000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);
    // LMIC_setupChannel(1, 923400000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);

    // ... extra definitions for channels 2..n here
    #elif defined(CFG_kr920)
    // Set up the channels used in your country. Three are defined by default,
    // and they cannot be changed. Duty cycle doesn't matter, but is conventionally
    // BAND_MILLI.
    // LMIC_setupChannel(0, 922100000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_MILLI);
    // LMIC_setupChannel(1, 922300000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_MILLI);
    // LMIC_setupChannel(2, 922500000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_MILLI);

    // ... extra definitions for channels 3..n here.
    #elif defined(CFG_in866)
    // Set up the channels used in your country. Three are defined by default,
    // and they cannot be changed. Duty cycle doesn't matter, but is conventionally
    // BAND_MILLI.
    // LMIC_setupChannel(0, 865062500, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_MILLI);
    // LMIC_setupChannel(1, 865402500, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_MILLI);
    // LMIC_setupChannel(2, 865985000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_MILLI);
    #endif

    // Disable link check validation
    //TODO: Rethink this for when we actually want to do a link check.
    LMIC_setLinkCheckMode(0);

    //set up RX2 datarate / SF.
    // info: TTN uses SF9 for its RX2 window.
    LMIC.dn2Dr = ConvertSFIntoLMICDr((int) cfg.lorawan_rx2_sf);
    LMIC_setAdrMode(cfg.lorawan_adr_enabled);
    // setup TX SF/DR and power (e.g. +14)
    LMIC_setDrTxpow(ConvertSFIntoLMICDr((int) cfg.lora_last_sf), cfg.lora_tx_power);
    // setup 50% default clock error (otherwise many nodes cannot receive).
    // affects timing when RX window is opened (slightly before). 
    LMIC_setClockError(MAX_CLOCK_ERROR * 75 / 100);
    // set sequnce numbers
    LMIC.seqnoUp = cfg.lorawan_framecnt_up;
    LMIC.seqnoDn = cfg.lorawan_framecnt_down;
    //set devnonce choice
    lmic_devnonce_choice = cfg.devnonce_choice;
}

u1_t LoRaWANManager::ConvertSFIntoLMICDr(int sf) {
    switch(sf) {
        case 7: return DR_SF7;
        case 8: return DR_SF8;
        case 9: return DR_SF9;
        case 10: return DR_SF10;
        case 11: return DR_SF11;
        case 12: return DR_SF12;
        default: return DR_SF12; /* unknown */
    }
}

void LoRaWANManager::Process() {
    os_runloop_once();
}

/* must be globally visible function, not in class */
void onEvent (ev_t ev) {
    userInput.print(os_getTime());
    userInput.print(": ");
    switch(ev) {
        case EV_SCAN_TIMEOUT:
            userInput.println(F("EV_SCAN_TIMEOUT"));
            break;
        case EV_BEACON_FOUND:
            userInput.println(F("EV_BEACON_FOUND"));
            break;
        case EV_BEACON_MISSED:
            userInput.println(F("EV_BEACON_MISSED"));
            break;
        case EV_BEACON_TRACKED:
            userInput.println(F("EV_BEACON_TRACKED"));
            break;
        case EV_JOINING:
            userInput.println(F("EV_JOINING"));
            break;
        case EV_JOINED:
            userInput.println(F("EV_JOINED"));
            {
                userInput.println(F("OTTA Join successful. Printing key parameters."));
                u4_t netid = 0;
                devaddr_t devaddr = 0;
                u1_t nwkKey[16];
                u1_t artKey[16];
                LMIC_getSessionKeys(&netid, &devaddr, nwkKey, artKey);
                userInput.print("NetID: 0x");
                userInput.println(StringHelper::toFixedLenHexNumber(netid));
                userInput.print("DevAddr: 0x");
                userInput.println(StringHelper::toFixedLenHexNumber(devaddr));
                userInput.print("NwkKey: 0x");
                userInput.println(StringHelper::toHexString(nwkKey, sizeof(nwkKey)));
                userInput.print("ArtKey (Application Router Session Key): 0x");
                userInput.println(StringHelper::toHexString(artKey, sizeof(artKey)));
                // Disable link check validation (automatically enabled
                // during join, but because slow data rates change max TX
                // size, we don't use it in this example.
                LMIC_setLinkCheckMode(0);
            }
            break;
        /*
        || This event is defined but not used in the code. No
        || point in wasting codespace on it.
        ||
        || case EV_RFU1:
        ||     userInput.println(F("EV_RFU1"));
        ||     break;
        */
        case EV_JOIN_FAILED:
            userInput.println(F("EV_JOIN_FAILED"));
            break;
        case EV_REJOIN_FAILED:
            userInput.println(F("EV_REJOIN_FAILED"));
            break;
        case EV_TXCOMPLETE:
            userInput.println(F("EV_TXCOMPLETE (includes waiting for RX windows)"));
            if (LMIC.txrxFlags & TXRX_ACK)
              userInput.println(F("Received ack"));
            if (LMIC.dataLen) {
              userInput.println(F("Received "));
              userInput.println(LMIC.dataLen);
              userInput.println(F(" bytes of payload"));
            }
            //write framecounters back in config
            cfg.lorawan_framecnt_up = LMIC.seqnoUp;
            cfg.lorawan_framecnt_down = LMIC.seqnoDn;
            ConfigManager::SaveConfig(cfg);
            //Scheck if we need to schedule the next transmission
            LoRaWANManager::CheckRepeatedTransmit();
            break;
        case EV_LOST_TSYNC:
            userInput.println(F("EV_LOST_TSYNC"));
            break;
        case EV_RESET:
            userInput.println(F("EV_RESET"));
            break;
        case EV_RXCOMPLETE:
            // data received in ping slot
            userInput.println(F("EV_RXCOMPLETE"));
            break;
        case EV_LINK_DEAD:
            userInput.println(F("EV_LINK_DEAD"));
            break;
        case EV_LINK_ALIVE:
            userInput.println(F("EV_LINK_ALIVE"));
            break;
        /*
        || This event is defined but not used in the code. No
        || point in wasting codespace on it.
        ||
        || case EV_SCAN_FOUND:
        ||    userInput.println(F("EV_SCAN_FOUND"));
        ||    break;
        */
        case EV_TXSTART:
            userInput.println(F("EV_TXSTART"));
            break;
        case EV_TXCANCELED:
            userInput.println(F("EV_TXCANCELED"));
            break;
        case EV_RXSTART:
            /* do not print anything -- it wrecks timing */
            break;
        case EV_JOIN_TXCOMPLETE:
            userInput.println(F("EV_JOIN_TXCOMPLETE: no JoinAccept"));
            break;
        default:
            userInput.print(F("Unknown event: "));
            userInput.println((unsigned) ev);
            break;
    }
}

/// This EUI must be in little-endian format, so least-significant-byte
// first. When copying an EUI from ttnctl output, this means to reverse
// the bytes. For TTN issued EUIs the last bytes should be 0xD5, 0xB3,
// 0x70.
void os_getArtEui (u1_t* buf) { userInput.println("Loaded AppEUI"); memcpy_P(buf, cfg.lorawan_otaa_appeui, 8);}
// This should also be in little endian format, see above.
void os_getDevEui (u1_t* buf) { userInput.println("Loaded DevEUI"); memcpy_P(buf, cfg.lorawan_otaa_deveui, 8);}
// This key should be in big endian format (or, since it is not really a
// number but a block of memory, endianness does not really apply). In
// practice, a key taken from ttnctl can be copied as-is.
void os_getDevKey (u1_t* buf) { userInput.println("Loaded AppKey");  memcpy_P(buf, cfg.lorawan_otaa_appkey, 16);}

void LoRaWANManager::Detach() { 
    LMIC_shutdown();
}

void LoRaWANManager::ScheduleTX(uint8_t fport, const uint8_t* payload, size_t payload_len, bool confirmed_uplink, bool reset_repeat) {
    if (LMIC.opmode & OP_TXRXPEND) {
        userInput.println(F("Transmission is still pending, not sending payload."));
    } else {
        //save as last transmission
        last_tx.confirmed = confirmed_uplink;
        last_tx.fport = fport;
        if(payload_len <= sizeof(last_tx.payload)){
            last_tx.payload_len = payload_len;
            memcpy(last_tx.payload, payload, payload_len);
        }
        if(reset_repeat) {
            last_tx.repititions = 0;
            last_tx.delay_between_transmisions = 0;
        }

        // Prepare upstream data transmission at the next possible time.
        LMIC_setTxData2(fport, (uint8_t*) payload, payload_len, (u1_t) confirmed_uplink);
        userInput.println(F("Packet queued"));
    }
}

void LoRaWANManager::RepeatLast(int times, unsigned delay) {
    last_tx.repititions = times;
    last_tx.delay_between_transmisions = delay;
    //kick off the first transmission of the chain
    LoRaWANManager::ScheduleRepeated(&repeat_job);
}

void LoRaWANManager::ScheduleRepeated(osjob_t* j) {
    ScheduleTX(last_tx.fport, last_tx.payload, last_tx.payload_len, last_tx.confirmed, false);
}

void LoRaWANManager::CheckRepeatedTransmit() {
    if(last_tx.repititions > 0 || last_tx.repititions == -1) {
        userInput.println("Still have "+ (last_tx.repititions == -1 ? String("infinite") : String(last_tx.repititions)) + " repititions, rescheduling in " + String(last_tx.delay_between_transmisions) + " sec.");
        if(last_tx.repititions > 0)
            last_tx.repititions--;
        os_setTimedCallback(&repeat_job, os_getTime()+sec2osticks(last_tx.delay_between_transmisions), &LoRaWANManager::ScheduleRepeated);
    } else if(last_tx.repititions == 0) {
        userInput.println("Last repeated transmission has been completed successfully");      
    }
}