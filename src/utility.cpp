

// utility.cpp
#include "utility.h"
#include "classes/RF24Radio.h"
extern RF24Radio radio;


namespace utility {

// Calculates a radio 24 nodes remaining battery charge
uint16_t calc_remaining_battery_charge(uint8_t RF24_node, uint16_t node_voltage) {
    // Li-ion batteries do not discharge linearly!
    // https://www.icode.com/how-to-measure-and-display-battery-level-on-micro-controller-arduino-projects/
    
    // Measured battery voltage (ADC reading, 0-1023) for each battery driven RF24 node
    uint16_t volt_max;
    uint16_t volt_min;
    switch (RF24_node) {
        case 1: 
            volt_max = 973;
            volt_min = 766;
            break;
        default:
            return 0;
    }
    // Calculate remaining battery charge 
    uint16_t voltage = node_voltage;
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


// Set current time 
void set_current_time(const String& payload) {
    setTime(payload.toInt());
    print("Time set: %02i:%02i:%02i \n", hour(), minute(), second());
}


// #######################  DEBUG  #######################
#if SERIAL_ENABLED

// Macro PRINT_FUNC(...)
void print_function(const char* func_name, bool end_line, uint8_t above_lines) {
    for (uint8_t i = 0; i < above_lines; i++) { 
        print("\n"); 
    }
    (end_line) ? print("> %s \n", func_name) : print("> %s", func_name);
}


// Macro PRINT_PACKAGE(x) (hack)
void debug_print_package(RF24Radio::DataPackage& pkg, bool is_error) {

    // Heading
    if (is_error) {
        print("\nSorting ERROR! Total nr of nodes: %i \n", pkg.num_of_nodes);
        print("Error @ node: ");

        for (uint8_t i = 0; i < 3; i++) {
            if (radio.m_error_at_node_ID[i] != 0) {
                print("%i ", radio.m_error_at_node_ID[i]);
            }
        } 
        print("\n\n");      
    } else {
        print("\nSorting complete! Total nr of nodes: %i \n\n", pkg.num_of_nodes); 
    }

    // Print package(s)
    print("#######  Node [%i]  #######          ", pkg.already_seen_node_ID[0]);
    if (pkg.num_of_nodes >= 2) { print("#######  Node [%i]  #######          ", pkg.already_seen_node_ID[1]); }
    if (pkg.num_of_nodes >= 3) { print("#######  Node [%i]  #######          ", pkg.already_seen_node_ID[2]); }   
    print("\n");

    for (uint8_t i = 0; i < 6; i++) {
        for (uint8_t j = 0; j < pkg.num_of_nodes; j++) {
            print("ID: %-2i     NR: %-2i     Msg: %-4i     ", pkg.already_seen_node_ID[j], i, pkg.message_container[j][i]);
        }
        print("\n");
    }
    print("\n"); 
}


#endif // SERIAL_ENABLED
} // namespace utility


