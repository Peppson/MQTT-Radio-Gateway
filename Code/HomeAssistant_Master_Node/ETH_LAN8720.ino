
/*
############################ MQTT 0 Master ############################
#                                                                     #
#   - MQTT Bridge to NRF24L01 nodes and generic 433Mhz radios         #                                          
#   - Library, TMRh20/RF24: https://github.com/tmrh20/RF24/           #     
#   - Class config: https://nrf24.github.io/RF24/classRF24.html       #     
#   - NRF24L01 Address "0"                                            #
#                                                                     #
#######################################################################
*/


// Main.ino
#include "Functions.h"


// Setup
void setup() {
    Setup_hardware();
    MQTT::Get_current_time();
    //Debug();
}


void Debug() {
    /*
    unsigned long old_millis = millis();
    println(millis() - old_millis);
    delay(5000);
    */
    
    //MQTT::Send_node_data_to_db(1);

    RF433::Transmit_RF_remote_codes(2);
    delay(2000);
    RF433::Transmit_RF_remote_codes(3);
    delay(2000);
    RF433::Transmit_RF_remote_codes(2);
    delay(2000);
    RF433::Transmit_RF_remote_codes(3);
    delay(434343000);
}


// Main loop
void loop() {
    for (uint32_t i = 0; i < Main_loop_iterations; i ++) {
        
        // Reset NRF24 Pipe and Check for MQTT message
        uint8_t Pipe;
        MQTT_client.loop();

        // New NRF24 radio message available? 
        if (radio.available(&Pipe) && Pipe == This_dev_address) {
            if(!NRF24::Get_available_message()) {
                delay(3000);
                radio.flush_rx();
                // Placeholder func "send again"
            }
            // Which Node was sending?
            else if (NRF24_package[0] == This_dev_address) {
                radio.stopListening();
                Print_package();
                NRF24::Respond_to_sending_node(NRF24_package[1]);
                radio.flush_rx(); 
                radio.flush_tx();
                radio.startListening();
            }        
        }
    }
    // MQTT connected?
    if (!MQTT_client.connected()) {
        for (uint8_t i = 0; i < 3; i ++) {
            if (MQTT::Connect()){
                break;
            }
            else if (i==2) {
                ESP.restart();
            }
            delay(5000);
        }   
    }
    // Grab time manually if something went wrong
    if (millis() - Prev_millis >= Check_time_interval) {
        MQTT::Get_current_time();
    }
    // Restart each monday at ca 04:00
    uint16_t Current_minute = hour() * 60 + minute();
    if ( (weekday() == 2) && (Current_minute >= 238 && Current_minute <= 239) ) {
        ESP.restart();
    }   
}

