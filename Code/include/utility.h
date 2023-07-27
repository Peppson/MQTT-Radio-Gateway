

// utility.h
#pragma once
#include "config.h"


namespace utility {

// Calculate a radio 24 nodes remaining battery charge
uint16_t calc_remaining_battery_charge(uint8_t radio_24_node);

// Error! sending "retry message" to each sending node.
bool handle_receive_error(uint32_t iteration, uint8_t sending_node_1 = 0, uint8_t sending_node_2 = 0);

// Set current time 
void set_current_time(const String& payload);



// #######################  DEBUG  #######################

#if SERIAL_ENABLED
    // Macro: print_func(...) 
    void print_function(const char* func_name, bool end_line = true, uint8_t above_lines = 0);
#endif


} // namespace utility


