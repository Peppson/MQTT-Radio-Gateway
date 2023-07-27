

// utility.cpp
#include "config.h"
#include "classes.h"
extern RF24Radio radio_24;


namespace utility {

// Calculates a radio 24 nodes remaining battery charge
uint16_t calc_remaining_battery_charge(uint8_t radio_24_node) {
    // Li-ion batteries do not discharge linearly!
    // https://www.icode.com/how-to-measure-and-display-battery-level-on-micro-controller-arduino-projects/
    
    // Measured battery voltage (ADC reading, 0-1023) for each battery driven node
    uint16_t volt_max;
    uint16_t volt_min;
    switch (radio_24_node) {
        case 1: 
            volt_max = 973;
            volt_min = 766;
            break;
        default:
            return 0;
    }
    // Calculate remaining battery charge 
    uint16_t voltage = radio_24.message_package[3];
    uint16_t volt_85 = volt_max * 0.95;
    uint16_t volt_10 = volt_max * 0.8;
    uint16_t bat_percent;

    // 100% range
    if (voltage >= volt_max) bat_percent = 100;

    // 85-100% 
    else if (voltage >= volt_85)    
        bat_percent = map(voltage, volt_85, volt_max, 85, 100);

    // 10-85% 
    else if (voltage >= volt_10)    
        bat_percent = map(voltage, volt_10, volt_85, 10, 85);

    // 0-10% 
    else if (voltage >= volt_min)    
        bat_percent = map(voltage, volt_min, volt_10, 0, 10);

    // 0% :(
    else bat_percent = 0;

    return bat_percent;
}


// Error! sending "retry message" to each sending node. code 3333 = repeat message //TODO
bool handle_receive_error(uint32_t iteration, uint8_t sending_node_1, uint8_t sending_node_2) {
    print_func();

    // First node
    if (iteration == 0 && sending_node_1 != 0) {        
        radio_24.try_send_message(sending_node_1, 0, 0, 0, 3333);

    // Second node after some time
    } else if (iteration == 100000 && sending_node_2 != 0) {
        radio_24.try_send_message(sending_node_2, 0, 0, 0, 3333);
    } 
    // Assumes it worked, otherwise nodes will restart themselfs after 15mins
    if (iteration == 100000) { return false; }
    
    return true;  
}


// Set current time 
void set_current_time(const String& payload) {
    setTime(payload.toInt());
    print("Time set: %02i:%02i:%02i", hour(), minute(), second());
}



// #######################  DEBUG  #######################

#if SERIAL_ENABLED
    // Macro print_func(...)
    void print_function(const char* func_name, bool end_line, uint8_t above_lines) {
        for (uint8_t i = 0; i < above_lines; i++) { 
            print("\n"); 
        }
        if (end_line) {
            print("> %s \n", func_name);
        } else {
            print("> %s", func_name);
        }
    }
#endif


} // namespace utility


