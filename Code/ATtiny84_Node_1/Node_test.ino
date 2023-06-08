
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

// Main_Node_1
#include "Setup_Node_1.h"
#include "Functions_Node_1.h"
using namespace Functions;


// Setup
void setup() {
    Setup_everything();
    //Send_ADC_get_NTP();
    //Prev_hour = hour();
    #if ADC_CAL_ON
        while (1) {ADC_CAL_FUNC();}
    #endif

    digitalWrite(PUMP_PIN, HIGH);
    delay(1000);
    digitalWrite(PUMP_PIN, LOW);
    Send_ADC_get_NTP();
    delay(543543534);
    //Send_debug() 
    
}


// Main loop
void loop() {
    // Save power, toggle transciever on/off
    for (uint16_t i = 0; i <= Main_loop_iterations; i++) { 

        // Powerup radio, reset watchdog
        WDT_RESET();
        radio.powerUp();
        delay(2);
        radio.startListening();

        // Listening for message
        if (Wait_for_message(200)) {
            println("Message recieved!");
            // Start Pump command 
            if ( (Message_package[0] == This_dev_address) && (Message_package[1] == Master_node_address) && (Message_package[4] == 1) ) {
                Start_water_pump(Message_package[2]);
                radio.flush_rx(); 
                radio.flush_tx();   
            }
        }
        // Powerdown radio
        radio.stopListening();
        if (i < Main_loop_iterations) {
            radio.powerDown();
            delay(Sleep_radio_for_ms);
        }     
    } /*
    // Send battery status every Time_interval
    time_t Current_hour = hour();
    if (Current_hour >= Prev_hour + Time_interval) {
        Send_ADC_get_NTP();
        Prev_hour = hour();
    }
    // Time to sleep?
    if (Current_hour >= Sleep_at_hour) {
        Deepsleep_device();
    } */
}

