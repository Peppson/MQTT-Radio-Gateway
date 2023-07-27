

// secrets.h
#pragma once
#include "config.h"


// MQTT
const char* Secret_MQTT_ssid = "";
const char* Secret_MQTT_password = "";
const char* Secret_MQTT_ip = "";
const int Secret_MQTT_port = 0;


// RF433 radio codes
// Captured using my own terrible Budget_433Mhz_Sniffer.ino 
// https://github.com/Peppson/Radio-Nodes/blob/main/Code/Misc_helpers/Budget_433Mhz_Sniffer.ino 
const uint16_t Secret_RF_remote_codes[7][132] PROGMEM = {

    // Button 0 ON
        {111, 2222}, //...
    // Button 0 OFF
        {111, 2222}, //...     
    // Button 1 ON
        {111, 2222}, //...     
    // Button 1 OFF
        {111, 2222}, //...     
    // Button 2 ON
        {111, 2222}, //...     
    // Button 2 OFF
        {111, 2222}, //...     
    // Button ALL OFF
        {111, 2222}, //...         
};

