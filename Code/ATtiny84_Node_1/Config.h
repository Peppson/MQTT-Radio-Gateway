
/* 
#########################  Node 1 Config  ###########################
# - MCU ATtiny84                                                    #
#                        VCC  1|o   |14 GND                         #
#                        10   2|    |13 A0/0 (AREF)                 #
#                        9    3|    |12 A1/1 --- ADC_Enable_Pin     #
#         Pullup 10K --- RST  4|    |11 A2/2 --- Radio   CE         #
#           PUMP_PIN --- PB2  5|    |10 A3/3 --- Radio  CSN         #
#   ADC_Measure_PIN --- A7/7  6|    |9  A4/4 --- Radio  SCK         #
#        Radio MISO --- A6/6  7|    |8  A5/5 --- Radio MOSI         #
#                                                                   #
# - Pico zero pinout                                                #
# - gp0 = CE, gp1 = CSN, gp2 = SCK, gp3 = MOSI, gp4 = MISO          #
#                                                                   #
#####################################################################


// RF24 message package(s)                                              
// Attiny84s have 2 bytes for Tx and Rx buffers...
// splitting up package into small chunks of data 

RF24_package[0] = to_who        // To who
RF24_package[1] = from_who      // From who
RF24_package[2] = "int"         // Node_1 run pump for how long     (Only on RX)
RF24_package[3] = "float"       // Node_1 Bat charge remaining      (ADC reading 0-1023)
RF24_package[4] = "bool"        // Bool on/off                      (Only on RX)
RF24_package[5] = "time"        // Current time                     (Only on RX)

uint16_t RF24_package[6] = {Msg_to_who, Msg_from_who, Msg_int, Msg_float, Msg_state, Msg_time};
*/ 


#ifndef __CONFIG_H__
#define __CONFIG_H__
#include <Arduino.h>
#include <stdint.h>
#include <SPI.h>  
#include <nRF24L01.h>  
#include <RF24.h>


// User Controlled
#define ATTINY84_ON 0                       // ATtiny84 or other MCUs   
#define SERIAL_ON 1                         // Serial toggle
#define ADC_CAL_ON 0                        // Enter ADC_CAL_FUNC (Req SERIAL_ON)

#define THIS_DEV_ADDRESS 1                  // Device address 
#define MASTER_NODE_ADDRESS 0               // Address of master
#define SLEEP_RADIO_TIME 800                // 200ms on, (ms) off                                   
#define RADIO_CHANNEL 124                   // Radio channel
#define RADIO_OUTPUT RF24_PA_MIN            // Output level: RF24_PA_MIN, RF24_PA_LOW, RF24_PA_HIGH, RF24_PA_MAX

#define PUMP_RUNTIME_MAX 15                 // Maximum time pump is allowed to run (s)              
#define DEEPSLEEP_AT_THIS_TIME 2100         // Go to deepsleep at what time? (hhmm)
#define DEEPSLEEP_HOW_LONG 12               // Deepsleep how long in (h)
#define UPDATE_INTERVAL 3*60*60*1000UL      // How often should the node report its battery status? (h*m*s*ms)
#define MAIN_LOOP_ITERATIONS 20             // How many loop iterations 


// Board Select
#if ATTINY84_ON 
    #include <ATTinyCore.h>
    #include <avr/wdt.h>
    #include <avr/sleep.h> 
    #define CE_PIN A2         
    #define CSN_PIN A3
    #define PUMP_PIN PB2   
    #define ADC_MEASURE_PIN A7
    #define ADC_ENABLE_PIN A1
    #define DEEPSLEEP() hardware::Deepsleep();
    #define WDT_RESET() wdt_reset() 
    #define WDT_DISABLE() wdt_disable()
    #define WDT_ENABLE(x) wdt_enable(x)
#else
    #define CE_PIN 14        // Pico 14, pico zero 0, esp32 14
    #define CSN_PIN 5        // Pico 17, pico zero 1, esp32 5
    #define PUMP_PIN 2        
    #define ADC_MEASURE_PIN 25
    #define ADC_ENABLE_PIN 26
    #define DEEPSLEEP()
    #define WDT_RESET()
    #define WDT_DISABLE()
    #define WDT_ENABLE(x)
#endif


// Serial toggle
#if SERIAL_ON && ATTINY84_ON
    #include <SendOnlySoftwareSerial.h>
    #define print(x) Tiny_serial.print(x)
    #define println(x) Tiny_serial.println(x)
    #define Debug_print(x)
    #define Debug_println(x)
    #define SERIAL_BEGIN(x) Tiny_serial.begin(x)

#elif SERIAL_ON && !ATTINY84_ON
    #define print(x) Serial.print(x)
    #define println(x) Serial.println(x)
    #define Debug_print(x) print(x)
    #define Debug_println(x) println(x)
    #define SERIAL_BEGIN(x) Serial.begin(x)

#else // Cast print statements into nothing
    #define print(x)
    #define println(x)
    #define Debug_print(x)
    #define Debug_println(x)
    #define SERIAL_BEGIN(x)
#endif


// Global variables
extern unsigned long ADC_at_this_millis;    // Absolute timestamp
extern unsigned long Sleep_at_this_millis;  // Absolute timestamp
extern volatile int  Deepsleep_count;       // Counter while in deepsleep

#endif // __CONFIG_H__


