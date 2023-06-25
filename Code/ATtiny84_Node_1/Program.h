



#ifndef __PROGRAM_H__
#define __PROGRAM_H__
#include "Config.h"


// NRF24L01 Radio
class NRF24L01_radio : public RF24 {
    private:
        // Radio device addresses     
        const uint8_t address[2][6] = {"1Adrs", "2Adrs" /*, "3Adrs", "4Adrs", "5Adrs" */};  
        
    public:
        // Constructor inline
        NRF24L01_radio(uint8_t CE = CE_PIN, uint8_t CSN = CSN_PIN) : RF24(CE, CSN) {};

        // Begin
        bool Begin_object();

        // Send message 
        bool Send_message(uint16_t* RF24_package_ptr); 

        // Try to send message
        bool Try_send_message(/*uint16_t arg_to_who = 0, uint16_t arg_int = 0, uint16_t arg_float = 0, uint16_t arg_state = 0, uint16_t arg_time = 0*/); 
        
        // Grab available message
        bool Get_available_message(uint16_t* RF24_package_ptr);

        // Wait for incomming message 
        bool Wait_for_message(uint16_t how_long, uint16_t* RF24_package_ptr);

        // Send battery charge remaining, get back current time
        void Send_ADC_get_time();
};


namespace hardware {

// Setup 
void Setup();

// Message received, start waterpump for n seconds
void Start_water_pump(uint8_t How_long = 2);

// Get ADC reading from battery
uint16_t Battery_charge_remaining();

// Hardware reset
void Reset_after_15min(uint16_t Seconds = 60 * 15);

// Used to have <TimeLib.h> for this but ran out of memory :/
void Calc_time_until_sleep(uint16_t* RF24_package_ptr);

#if ATTINY84_ON
    // Watchdog ISR 
    ISR (WDT_vect);

    // ATtiny84 + NRF24 Deepsleep (about 5 µA)
    void Deepsleep();
#endif

} // namespace hardware


//####################  DEBUG  ####################
#if SERIAL_ON
    #if ADC_CAL_ON
        // ADC CAL DEBUG FUNC
        void ADC_CAL_FUNC();
    #endif

    #if !ATTINY84_ON
        // Print message package
        void Print_package(uint16_t* RF24_package);
    #endif
#endif

#endif // __PROGRAM_H__


