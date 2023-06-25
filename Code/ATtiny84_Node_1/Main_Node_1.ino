
/*
###########################  Node 1 Main  ###########################
#                                                                   #
#   - Flower_waterpump, controlled via Home Assistant               #                                          
#   - Library, TMRh20/RF24: https://github.com/tmrh20/RF24/         #     
#   - Class config: https://nrf24.github.io/RF24/classRF24.html     #     
#   - NRF24L01 Address "1"                                          #
#                                                                   #
#####################################################################
*/


#include "Config.h"
#include "Program.h"
class NRF24L01_radio RF24_radio;


// Setup
void setup() { 
    hardware::Setup();
    #if ADC_CAL_ON
        ADC_CAL_FUNC();
    #endif 
    RF24_radio.Send_ADC_get_time();
    ADC_at_this_millis = millis() + UPDATE_INTERVAL;   
}


// Main loop
void loop() {
    // Save power, toggle transciever on/off
    for (uint8_t i = 0; i <= MAIN_LOOP_ITERATIONS; i++) {
        uint16_t RF24_package[6];

        // Powerup radio, reset watchdog
        WDT_RESET();
        RF24_radio.powerUp();
        delay(2);
        RF24_radio.startListening();

        // Listen for incomming message, start waterpump?
        if (RF24_radio.Wait_for_message(200, RF24_package)) {
            if ((RF24_package[0] == THIS_DEV_ADDRESS) 
            &&  (RF24_package[1] == MASTER_NODE_ADDRESS) 
            &&  (RF24_package[4] == true)) {
                hardware::Start_water_pump(RF24_package[2]);
                RF24_radio.flush_rx(); 
                RF24_radio.flush_tx(); 
            }
        }  
        // Powerdown radio 
        RF24_radio.stopListening();
        if (i < MAIN_LOOP_ITERATIONS) {
            RF24_radio.powerDown();
            delay(SLEEP_RADIO_TIME);
        }     
    }
    // Time to deepsleep?
    unsigned long Current_millis = millis();
    if (Current_millis > Sleep_at_this_millis) {
        DEEPSLEEP(); 
            
    // Send battery status every Update_interval
    } else if (Current_millis > ADC_at_this_millis) {
        #if !ADC_CAL_ON  
            RF24_radio.Send_ADC_get_time();
            ADC_at_this_millis = millis() + UPDATE_INTERVAL;
        #endif // Memory related
    }    
}


