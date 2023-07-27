

// config.h
#pragma once
#include <Arduino.h>
#include <SPI.h>  
#include <nRF24L01.h>  
#include <RF24.h>
#include <TimeLib.h>
#include <ETH.h>
#include <MQTT.h>
#include <ArduinoJson.h> 
#include <esp_task_wdt.h>
#include <stdio.h>


// User Controlled
#define FAKE_TIME_ENABLED 0                         // Send the current time as 12:00 to RF24 nodes
#define SERIAL_ENABLED 1                            // Serial prints On/Off
#define RF24_THIS_DEV_ADDRESS 0                     // RF24 device address                                        
#define RF24_CHANNEL 120                            // RF24 radio channel
#define RF24_OUTPUT RF24_PA_MAX                     // RF24 output level: RF24_PA_MIN, RF24_PA_LOW, RF24_PA_HIGH, RF24_PA_MAX
#define RF24_DATARATE RF24_250KBPS                  // RF24_2MBPS, RF24_1MBPS, RF24_250KBPS
#define LOCAL_ROUTER_IP "192.168.1.1"               // IP
#define LOCAL_ROUTER_PORT 80                        // Port
#define WATCHDOG_TIMEOUT 300                        // Watchdog reboot timer
#define LOOP_ITERATIONS 500000                      // Main loop iterations
#define UPDATE_INTERVAL (14*60*60*1000UL)           // How often to grab current time manually (Should never happen)


// Serial toggle
#if SERIAL_ENABLED
    #define print(...) Serial.printf(__VA_ARGS__)
    #define print_func(...) utility::print_function(__func__, ##__VA_ARGS__) // end_line, above_lines
#else 
    #define print(...)
    #define print_func(...) 
#endif 


// LAN8720 module
#define ETH_TYPE        ETH_PHY_LAN8720
#define ETH_CLK_MODE    ETH_CLOCK_GPIO0_IN    
#define ETH_ADDR        1                      
#define ETH_POWER_PIN   2                        
#define ETH_MDC_PIN     23      
#define ETH_MDIO_PIN    18

// NRF24L01+ module
#define SPI_1_MISO  12
#define SPI_1_MOSI  13
#define SPI_1_SCLK  14
#define SPI_1_CE    16 
#define SPI_1_CSN   5

// 433Mhz radio I/O
#define RF433_TX_PIN 33 
#define RF433_RX_PIN 32


