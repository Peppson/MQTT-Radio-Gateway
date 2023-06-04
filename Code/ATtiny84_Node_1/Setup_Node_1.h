
/*
########################  NRF Node 1 Setup  ########################
# - ATtiny84:                                                      #
#                       VCC  1|o   |14 GND                         #
#                       10   2|    |13 A0/0 (AREF)                 #
#                       9    3|    |12 A1/1                        #
#                       RST  4|    |11 A2/2 --- Radio   CE         #
#              PUMP --- PB2  5|    |10 A3/3 --- Radio  CSN         #
#              ADC --- A7/7  6|    |9  A4/4 --- Radio  SCK         #
#       Radio MISO --- A6/6  7|    |8  A5/5 --- Radio MOSI         #
#                                                                  #
# - Pico mini pinout:                                              #
# - gp0 = CE, gp1 = CSN, gp2 = SCK, gp3 = MOSI, gp4 = MISO         #
#                                                                  #
####################################################################
*/

// Setup_Node_1
#pragma once
#include <SPI.h>  
#include <nRF24L01.h>  
#include <RF24.h>
#include <TimeLib.h>


// User Controlled
#define ATtiny84_ON 0                       // ATtiny84 or Pico    
#define SERIAL_ON 1                         // Serial On/Off

#define Sleep_radio_for_ms 800              // 200ms on X(ms) off
#define Master_node_address 0               // Address of master
#define This_dev_address 1                  // Device address                                        
#define Radio_channel 124                   // Radio channel
#define Radio_output RF24_PA_MIN            // Output level: RF24_PA_MIN, RF24_PA_LOW, RF24_PA_HIGH, RF24_PA_MAX

#define Max_ADC_reading 1023                // Max ADC value, measured                   
#define Min_ADC_reading 0                   // Min ADC value, measured                    
#define Sleep_at_hour 21                    // Go to deepsleep at what time? 0-23
#define Time_interval (1000UL*60*60*4)      // How often should the mcu report its battery status? test = (1000UL*20)//
#define Main_loop_iterations 9            // How many interations, 900 = about 15mins


// Globals  
unsigned long
Prev_millis,                                    // Init
Current_millis;                                 // Init

volatile int 
Deepsleep_count = 0;                            // Counter in deepsleep

struct Data_package { 
    uint8_t Msg_to_who;                         // Who is this message for?
    uint8_t Msg_from_who;                       // Who is this message from?
    unsigned long Msg_time;                     // Current unix timestamp
    int Msg_int;                                // Var int 
    float Msg_float;                            // Var float                                                
    bool Msg_state;                             // Var bool
};                          

const uint8_t address[5][6] =  
{"1Adrs", "2Adrs", "3Adrs", "4Adrs", "5Adrs"};  // Address array for "This_dev_address"   


// Board Select
#if ATtiny84_ON 
    #define CE_PIN A2         
    #define CSN_PIN A3
    #define PUMP_PIN PB2   
    #define ADC_PIN A7
    #define Deepsleep_device() Deepsleep();
    #include <avr/wdt.h>
    #include <avr/sleep.h>  
#else
    #define CE_PIN 0        //Pico 14, arduino 9, pico mini 0
    #define CSN_PIN 1       //Pico 17, arduino 10, pico mini 1
    #define PUMP_PIN 15        
    #define ADC_PIN 29
    #define Deepsleep_device() Fake_deepsleep();
#endif


// Serial On/Off 
#if SERIAL_ON
    #define println(x) Serial.println(x)
    #define print(x) Serial.print(x)
#else
    #define println(x)
    #define print(x) 
#endif


// Init radio object and Message package
RF24 radio(CE_PIN, CSN_PIN);  
Data_package Package;


