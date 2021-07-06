#pragma once

#include <stdint.h>
#include <stddef.h>
#include <ConfigManager.h>
#include <arduino_lmic.h>

/* for repeatable transissions */
struct TXSettings {
    uint8_t payload[256];
    size_t payload_len; 
    uint8_t fport; 
    bool confirmed;
    int repititions; /* -1 means infinite */
    unsigned delay_between_transmisions; /* seconds */
};

/* Responsible for interacting with LMIC.
 * configuring it, sending uplink, receiving downlink
 * continuous receive? repeated send? QOS? Confirmed up? Link Check? Output
 * power? channel?
 */
class LoRaWANManager {
public:
    static bool Init(GenericNodeConfig& cfg);
    static void ScheduleTX(uint8_t fport, const uint8_t* payload, size_t payload_len, bool confirmed_uplink, bool reset_repeat = true);
    static void RepeatLast(int times, unsigned delay);
    static void ContinuousWave(uint32_t freq_hz);
    static void JoinOTAA();
    static void Process();
    /* when switching from LoRaWAN to LoRa mode we might need to do some de-init / detaching? */
    static void Detach(); 
    /* loaded params: TX SF, RX2 SF, keys, TX power, */
    static void ApplyLoRaWANConfig(GenericNodeConfig& cfg);

    static u1_t ConvertSFIntoLMICDr(int sf); 
    static void CheckRepeatedTransmit();
    static TXSettings* GetLastTX() { return &last_tx; }
private:
    static void ScheduleRepeated(osjob_t* j);

    static TXSettings last_tx;
    static osjob_t repeat_job;
};