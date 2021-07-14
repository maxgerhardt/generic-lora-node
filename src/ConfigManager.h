#pragma once
#include <Arduino.h>
#include <stdint.h>

enum LoRaWANActivationmethod : uint8_t {
    /* when we load from flash this is 0 and should should mean 'not yet selected'*/
    ACTIVATION_METHOD_UNSELECTED,
    /* Over-the-Air Activation*/
    ACTIVATION_METHOD_OTAA,
    /* Activation by Personalization */
    ACTIVATION_METHOD_ABP
};

enum OperationMode : uint8_t {
    OPMODE_NONE, /* do nothing */
    OPMODE_LORAWAN, /* run LoRaWAN stack */
    OPMODE_LORA, /* run LoRa (raw) stack */
};

/* settings persisted in flash / EEPROM */
struct GenericNodeConfig
{
    bool is_user_created;
    OperationMode op_mode;
    LoRaWANActivationmethod last_activation_method;
    /* ABP */
    uint8_t lorawan_abp_nwskey[16];
    uint8_t lorawan_abp_appskey[16];
    uint32_t lorawan_abp_dev_addr;
    /* OTAA */
    uint8_t lorawan_otaa_appeui[8];
    uint8_t lorawan_otaa_deveui[8];
    uint8_t lorawan_otaa_appkey[16];
    /* framecounter */
    uint32_t lorawan_framecnt_up;
    uint32_t lorawan_framecnt_down;
    /* MAC settings for LoRa and LoRaWAN */
    uint8_t lora_last_sf;
    /* e.g. +14 (dbM) */
    int8_t lora_tx_power;
    /* MAC settings for LoraWAN */
    uint8_t lorawan_rx2_sf;
    bool lorawan_adr_enabled;
    int devnonce_choice;
};

class ConfigManager
{
public:
    static void Init();
    static bool LoadConfig(GenericNodeConfig &loadedCfg);
    static bool SaveConfig(GenericNodeConfig &cfg, bool set_user_created_bit = true);
    static bool EraseConfig();

    static void PrintConfig(GenericNodeConfig &cfg, Stream &stream = Serial);
};