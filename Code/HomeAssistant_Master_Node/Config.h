

// Config.h/cpp
#pragma once
#include "Secrets.h"
#include <SPI.h>  
#include <nRF24L01.h>  
#include <RF24.h>
#include <TimeLib.h>
#include <ETH.h>
#include <MQTT.h>
#include <ArduinoJson.h>


// User Controlled
#define SERIAL_ON 1                                 // Serial On/Off
#define This_dev_address  0                         // Device address                                        
#define Radio_channel  124                          // Radio channel
#define Radio_output RF24_PA_MAX                    // Output level: RF24_PA_MIN, RF24_PA_LOW, RF24_PA_HIGH, RF24_PA_MAX
#define Main_loop_iterations 500000                 // 
#define Check_time_interval (14*60*60*1000UL)       // How often to grab current time (Should never happen) 


// Serial On/Off 
#if SERIAL_ON
    #define print(x) Serial.print(x)
    #define println(x) Serial.println(x)
    #define Print_package() Print_debug_package()
#else
    //Cast Print statements into nothing
    #define print(x)
    #define println(x)
    #define Print_package() 
#endif 


/*
######################  LAN8720 Ethernet module - ESP32 R1 R32  ######################
- Using default SPI pins with VSPI

    * GPIO02    -   NC (From NC > Osc enable pin on Lan8720 module)     
    * GPIO22    -   TX1                                      
    * GPIO19    -   TX0                                      
    * GPIO21    -   TX_EN                                    
    * GPIO26    -   RX1                                     
    * GPIO25    -   RX0                                      
    * GPIO27    -   CRS                                      
    * GPIO00    -   nINT/REFCLK (4k7 Pullup to 3.3V)    
    * GPIO23    -   MDC                                      
    * GPIO18    -   MDIO                                     
    * 3V3       -   VCC
    * GND       -   GND                                           
    
- ETH_CLOCK_GPIO0_IN   - default: external clock from crystal oscillator
- ETH_CLOCK_GPIO0_OUT  - 50MHz clock from internal APLL output on GPIO00 - possibly an inverter is needed for LAN8720
- ETH_CLOCK_GPIO16_OUT - 50MHz clock from internal APLL output on GPIO16 - possibly an inverter is needed for LAN8720
- ETH_CLOCK_GPIO17_OUT - 50MHz clock from internal APLL inverted output on GPIO17 - tested with LAN8720

- https://github.com/flusflas/esp32-ethernet
- https://github.com/SensorsIot/ESP32-Ethernet/blob/main/EthernetPrototype/EthernetPrototype.ino#L44

######################################################################################
*/


// Black magic!
#define ETH_TYPE        ETH_PHY_LAN8720
#define ETH_CLK_MODE    ETH_CLOCK_GPIO0_IN    
#define ETH_ADDR        1                       
#define ETH_POWER_PIN   2                        
#define ETH_MDC_PIN     23      
#define ETH_MDIO_PIN    18


// NRF24L01, H SPI
#define SPI_2_MISO   12
#define SPI_2_MOSI   13
#define SPI_2_SCLK   14
#define SPI_2_CE     16 
#define SPI_2_CSN    5 


// Globals
unsigned long 
Prev_millis;

// Message package
uint16_t Msg_to_who;                        // Attiny84 only have 2 bytes for Tx and Rx buffers...
uint16_t Msg_from_who;                      // Splitting up packages into small chunks of data 
uint16_t Msg_int;                           // uint16_t (2 Bytes) is max
uint16_t Msg_float; 
uint16_t Msg_state; 
uint16_t Msg_time;
uint16_t Message_package[6] = {Msg_to_who, Msg_from_who, Msg_int, Msg_float, Msg_state, Msg_time};                        

// Address array for "This_dev_address"     
const uint8_t address[5][6] = {"1Adrs", "2Adrs", "3Adrs", "4Adrs", "5Adrs"};     


// Init Radio, both SPIbuses and ETH client
RF24 radio(SPI_2_CE, SPI_2_CSN);
SPIClass spiBus1(VSPI); 
SPIClass spiBus2(HSPI);
WiFiClient Wifi_client;
MQTTClient MQTT_client;




