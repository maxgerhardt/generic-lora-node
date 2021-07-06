#include <Arduino.h>
#include <ConfigManager.h>
#include <UserInput.h>
#include <CommandExecutor.h>
#include <LoRaWANManager.h>
#include <arduino_lmic.h>

/* create user input interface from Serial stream.. can be anything inheriting
 * from 'Stream' */
UserInput userInput(Serial);
GenericNodeConfig cfg;

void setup()
{
  delay(500);
  //wait for serial connection (for USB devices)
  while (!Serial); 
  Serial.begin(115200);
  userInput.println("Firmware start!");
  CommandExecutor::Init(&userInput);
  ConfigManager::Init();
  ConfigManager::LoadConfig(cfg);
  //config sub-substacks
  //if(cfg.op_mode == OperationMode::OPMODE_LORAWAN) {
    userInput.println("Initializing LoRaWAN stack.");
    LoRaWANManager::Init(cfg);
  //}
}

void loop()
{
  CommandExecutor::Process();
  if(cfg.op_mode == OperationMode::OPMODE_LORAWAN) {
    LoRaWANManager::Process();
  } else if(cfg.op_mode == OperationMode::OPMODE_LORA) {
    //do processing for raw LoRa library    
  }
 // os_runloop_once();
}