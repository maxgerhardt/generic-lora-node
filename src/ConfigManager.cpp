#include <Arduino.h>
#include <ConfigManager.h>
#if defined(ARDUINO_SAMD_ZERO)
#include <FlashAsEEPROM.h>
#else
#include <EEPROM.h>
#endif
#include <StringHelper.h>

/* Sadly, different EEPROM libraries need to different calls to function correctly. 
 * We create a small abstraction layer here using unified functions.
 */
template< typename T > T &EEPROM_Get(int idx, T &t)
{
#if defined(ARDUINO_SAMD_ZERO)
    uint8_t *ptr = (uint8_t *) &t;
    for (int i=0; i < (int)sizeof(T); i++) {
        *ptr++ = EEPROM.read(idx + i);
    }
    return t;
#else 
    return EEPROM.get(idx, t);
#endif
}

#if defined(ARDUINO_SAMD_ZERO)
template< typename T > T &EEPROM_Put(int idx, T &t)
{
    const uint8_t *ptr = (const uint8_t *) &t;
    for (int i=0; i < (int)sizeof(T); i++) {
        EEPROM.update(idx + i, *ptr++);
    }
    return t;
}
#else 
#define EEPROM_Put EEPROM.put
#endif

void EEPROM_Begin() {
#if defined(ARDUINO_SAMD_ZERO)
#elif defined(ARDUINO_ARCH_ESP32)
    EEPROM.begin(sizeof(GenericNodeConfig));
#else 
    EEPROM.begin();
#endif   
}

void EEPROM_End() {
#if defined(ARDUINO_SAMD_ZERO)
#else 
    EEPROM.end();
#endif   
}

void EEPROM_Save() {
#if defined(ARDUINO_SAMD_ZERO)
    EEPROM.commit();
#else
#endif   
}

void ConfigManager::Init()
{
    /* need to do a EEPROM begin here? */
    /* check if EEPROM is all 0xFF and we need to place our own stuff there? */
    Serial.println("Initializing EEPROM");
    EEPROM_Begin();
    uint8_t user_loaded = 0;
    EEPROM_Get(0, user_loaded); /* first entry in the struct is user-loaded */
    EEPROM_End();
    Serial.println("user_loaded: 0x" + String(user_loaded, HEX));
    /* a bool should either read 0 or 1, if it is 0xff assume we are running on
   * erased flash/EEPROM and write an empty config
   */
    if (user_loaded == 0xff)
    {
        EraseConfig();
    }
}

bool ConfigManager::LoadConfig(GenericNodeConfig &loadedCfg)
{
    EEPROM_Begin();
    EEPROM_Get(0, loadedCfg);
    EEPROM_End();
    return true;
}

bool ConfigManager::SaveConfig(GenericNodeConfig &cfg, bool set_user_created_bit)
{
    if(set_user_created_bit)
        cfg.is_user_created = true;
    EEPROM_Begin();
    EEPROM_Put(0, cfg);
    EEPROM_End();
    EEPROM_Save();
    return true;
}

bool ConfigManager::EraseConfig()
{
    // overwriting with all-zeroed config
    GenericNodeConfig cfg{false};
    cfg.lora_tx_power = 14;
    cfg.lora_last_sf = 9;
    cfg.lorawan_rx2_sf = 9;
    return SaveConfig(cfg, false);
}

/* ToDo: Write some utils for byte array -> (hex) String conversion, fixed-width
 * hex number*/

void ConfigManager::PrintConfig(GenericNodeConfig &cfg, Stream &stream)
{
    stream.println("Is user created:\t" + String(cfg.is_user_created ? "true" : "false"));
    stream.println("Activation method:\t" + (cfg.last_activation_method == ACTIVATION_METHOD_OTAA ? String("OTAA") : cfg.last_activation_method == ACTIVATION_METHOD_ABP ? String("ABP") : String("none selected")));
    stream.println("Operation mode:\t\t" + (cfg.op_mode == OPMODE_LORA ? String("LoRa") : cfg.op_mode == OPMODE_LORAWAN ? String("LoRaWAN") : String("none selected")));
    stream.println("LoRaWAN ABP NWSKey:\t" +
                   StringHelper::toHexString(cfg.lorawan_abp_nwskey,
                                             sizeof(cfg.lorawan_abp_nwskey), true));
    stream.println("LoRaWAN ABP APPSKey:\t" +
                   StringHelper::toHexString(cfg.lorawan_abp_appskey,
                                             sizeof(cfg.lorawan_abp_appskey), true));
    stream.println("LoRaWAN ABP DevAddr:\t0x" +
                   StringHelper::toFixedLenHexNumber(cfg.lorawan_abp_dev_addr));
    stream.println("LoRaWAN OTAA AppEUI:\t" +
                   StringHelper::toHexString(cfg.lorawan_otaa_appeui,
                                             sizeof(cfg.lorawan_otaa_appeui), true));
    stream.println("LoRaWAN OTAA DevEUI:\t" +
                   StringHelper::toHexString(cfg.lorawan_otaa_deveui,
                                             sizeof(cfg.lorawan_otaa_deveui), true));
    stream.println("LoRaWAN OTAA AppKey:\t" +
                   StringHelper::toHexString(cfg.lorawan_otaa_appkey,
                                             sizeof(cfg.lorawan_otaa_appkey), true));
    stream.println("LoRaWAN Framecnt. Up:\t" + String(cfg.lorawan_framecnt_up));
    stream.println("LoRaWAN Framecnt. Down:\t" + String(cfg.lorawan_framecnt_down));
    stream.println("LoRaWAN ADR On:\t\t" + String(cfg.lorawan_adr_enabled ? "true" : "false"));
    stream.println("LoRaWAN OTAA DevNonce:\t" + (cfg.devnonce_choice == -1 ? String("random") : StringHelper::toFixedLenHexNumber(cfg.devnonce_choice)));
    stream.println("LoRa TX SF:\t\t" + ((cfg.lora_last_sf >= 7 && cfg.lora_last_sf <= 12) ? (String("SF") + String(cfg.lora_last_sf)) : String("not selected")));
    stream.println("LoRa RX2 SF:\t\t" + ((cfg.lorawan_rx2_sf >= 7 && cfg.lorawan_rx2_sf <= 12) ? (String("SF") + String(cfg.lorawan_rx2_sf)) : String("not selected")));
    stream.println("LoRa TX Power:\t\t" + String(cfg.lora_tx_power) + " dbM");
}