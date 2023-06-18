
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
    Get_current_time(); 
}

/*
    unsigned long old_millis = millis();
    println(millis() - old_millis);
    delay(5000);
*/

// Main loop
void loop() {
    for (uint32_t i=0; i<Main_loop_iterations; i++) {

        // Reset radio Pipe and check for MQTT
        uint8_t Pipe;
        MQTT_client.loop();

        // New 2.4Ghz radio message available? 
        if (radio.available(&Pipe) && Pipe == This_dev_address) {
            if(!Get_available_message()) {
                delay(3000);
                radio.flush_rx();
                println("Sending 'Send again' message");
                // Placeholder func  
            }
            // Which Node was sending?
            else if (Message_package[0] == This_dev_address) {
                radio.stopListening();
                Print_package();
                Who_was_sending_and_respond(Message_package[1]);
                radio.flush_rx(); 
                radio.flush_tx();
                radio.startListening();
                //MQTT_client.publish("/Node1", "vaknade");
            }        
        }
    }
    // MQTT connected?
    if (!MQTT_client.connected()) {
        MQTT_connect();
    }
    // Grab time manually if something went wrong
    if (millis() - Prev_millis >= Check_time_interval) {
        Get_current_time();
    } 
}
