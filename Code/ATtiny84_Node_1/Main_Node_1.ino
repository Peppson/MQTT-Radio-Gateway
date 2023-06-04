
/*
#########################  NRF Node 1 Main  #########################
#                                                                   #
#   - Flower_waterpump, controlled through Home Assistant           #                                          
#   - Library, TMRh20/RF24: https://github.com/tmrh20/RF24/         #     
#   - Class config: https://nrf24.github.io/RF24/classRF24.html     #     
#   - NRF24L01 Address "1"                                          #
#                                                                   #
#####################################################################
*/


// Main
#include "Setup_Node_1.h"
#include "Functions_Node_1.h"
using namespace Functions;


// Setup
void setup() {
    Functions::Setup_everything();
    Current_millis = millis();
    Prev_millis = Current_millis;
    //Functions::Send_ADC_get_NTP();

    delay(5000);
}

// Main loop
void loop() {
    // Save power, toggle transciever on/off 
    for (int i = 0; i <= Main_loop_iterations; i++) { 

        // Watchdog reset
        #if ATtiny84_ON
            wdt_reset();
        #endif

        // Powerup radio
        radio.powerUp();
        delay(5);
        radio.startListening();
        Current_millis = millis();

        // Listening for message
        if (Wait_for_message(200)) {
            println("Message recieved!");
            radio.read(&Package, sizeof(Package));

            // Expected package size?
            if (sizeof(Package) > 20) {
                radio.flush_rx();
                }
            // Message to variables
            else if ((Package.Msg_to_who == This_dev_address) && (Package.Msg_state) && (Package.Msg_from_who == Master_node_address)) {
                setTime(Package.Msg_time);
                Start_water_pump(Package.Msg_int);
                Package = {};
                radio.flush_rx(); // Necessary?
                radio.flush_tx();   
            }
        }
        // Powerdown radio
        radio.stopListening();
        if (i < Main_loop_iterations) { 
            radio.powerDown();
            delay(Sleep_radio_for_ms);
        }
    }
    // Send battery status every Time_interval
    if (Current_millis >= Prev_millis + Time_interval) {
        Send_ADC_get_NTP();
        Prev_millis = Current_millis;
    }
    // Time to sleep?
    if (hour() >= Sleep_at_hour) {
        Deepsleep_device();
    } 
}

