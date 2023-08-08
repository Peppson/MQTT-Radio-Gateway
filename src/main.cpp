
/*
########################  MQTT Radio Gateway  ########################
#                                                                    #
#   - MQTT Bridge for NRF24L01+ nodes and generic 433Mhz radios      #
#   - https://github.com/Peppson/MQTT-Radio-Gateway                  #
#   - Target Mcu/Board: ESP32 D1 R32                                 #
#   - NRF24L01+ Address "0" "Master"                                 #
#   - Uploaded version: 0.1.3                                        #
#   - Build version: 0.1.6                                           #
#                                                                    #
######################################################################
*/


// main.cpp
#include "config.h"
#include "classes/Hardware.h"
#include "classes/RF24Radio.h"
#include "classes/MQTT.h"
Hardware hardware;
RF24Radio radio;
MQTT mqtt;


// Setup
void setup() {
    if (hardware.setup()) { mqtt.get_current_time(); }
}


// Main loop
void loop() {
    bool is_error = false;
    for (uint32_t i = 0; i < LOOP_ITERATIONS; i++) {
        esp_task_wdt_reset();
        
        // New MQTT message? 
        if (mqtt.loop()) {
            // Calls mqtt.handle_incoming_MQTT() 
        }
        // New radio 2.4Ghz message? 
        if (radio.available()) {
            is_error = radio.handle_incoming_radio();     
        }
        // Receive error
        if (is_error) { 
            radio.handle_receive_error(); 
        }
    }
    // MQTT connected?
    if (!mqtt.connected()) {
        mqtt.reconnect_or_reboot();    
    } 
    // Grab time manually in case something went wrong
    if (millis() - mqtt.prev_timestamp > UPDATE_INTERVAL) {       
        mqtt.get_current_time(); 
    }
    // Reboot each monday at ca 04:00
    hardware.reboot_once_a_week();
    radio.startListening(); 
}


