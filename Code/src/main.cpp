
/*
########################  MQTT Radio Gateway  ########################
#                                                                    #
#   - MQTT Bridge for NRF24L01+ nodes and generic 433Mhz radios      #
#   - https://github.com/Peppson/MQTT-Radio-Gateway                  #
#   - Target Mcu/Board: ESP32 D1 R32                                 #
#   - NRF24L01+ Address "0" "Master"                                 #
#                                                                    #
######################################################################
*/


// main.cpp
#include "config.h"
#include "classes.h"
#include "utility.h"
HardwareClass hardware;
RF24Radio radio_24;
MQTT mqtt;


// Setup
void setup() {
    if (hardware.setup()) { mqtt.get_current_time(); }
}


// Main loop
void loop() {
    bool is_error = false;
    uint8_t sending_Node_1;
    uint8_t sending_Node_2;
    radio_24.startListening();
    
    // Loop
    for (uint32_t i = 0; i < LOOP_ITERATIONS; i++) {
        esp_task_wdt_reset();
        
        // New MQTT message?
        mqtt.loop();
        // Calls mqtt.handle_incoming_MQTT()

        // New radio_24 message? 
        if (radio_24.available()) {
            is_error = radio_24.handle_incoming_radio(i, sending_Node_1, sending_Node_2);     
        }
        
        /*
        // radio_24 receive error!
        if (is_error) {
            is_error = utility::handle_receive_error(i, sending_Node_1, sending_Node_2);
        } 
        */ 
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
    uint16_t current_minute = (hour() * 60) + minute();
    if ((weekday() == 2) && (current_minute >= 238 && current_minute <= 239)) {
        delay(2*60*1000); // Cheap solution 
        ESP.restart();
    }   
}


