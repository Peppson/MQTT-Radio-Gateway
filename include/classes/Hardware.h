

// Hardware.h
#pragma once
#include "config.h"
#include "secrets/secrets.h"


// Hardware "hardware" 
class Hardware {
    public:
        // Transmit 433Mhz remote codes
        // Buttons: 1_on, 1_off, 2_on, 2_off, 3_on, 3_off, All_off
        void send_RF433_remote_codes(uint8_t code);

        // Calculate which RF433 code to send
        uint8_t calc_RF433_remote_codes(const String& topic);

        // Ethernet debug
        static void ETH_event(WiFiEvent_t event);

        // Reboot each monday at ca 04:00
        void reboot_once_a_week();

        // Hardware tests
        bool setup_hardware();

        // Setup
        bool setup();

}; 
