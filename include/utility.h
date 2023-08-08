

// utility.h
#pragma once
#include "config.h"
#include "classes/RF24Radio.h"


namespace utility {

// Calculate a radio 24 nodes remaining battery charge
uint16_t calc_remaining_battery_charge(uint8_t RF24_node, uint16_t node_voltage);

// Set current time 
void set_current_time(const String& payload);


// #######################  DEBUG  #######################
#if SERIAL_ENABLED

// Macro: PRINT_FUNC(...) 
void print_function(const char* func_name, bool end_line = true, uint8_t above_lines = 0);

// Macro PRINT_PACKAGE() (hacked together)
void debug_print_package(RF24Radio::DataPackage& pkg, bool is_error);

#endif // SERIAL_ENABLED
} // namespace utility


