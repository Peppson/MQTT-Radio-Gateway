
/*
######################### NRF 0 Master #########################

# EspHome custom C++ bridge for Home Assistant                                                                 
# Library, TMRh20/RF24: https://github.com/tmrh20/RF24/             
# Class config: https://nrf24.github.io/RF24/classRF24.html         
# https://github.com/Peppson/?

# Install ESPHome on device, via browser ezpz
# update new code over OTA


C:\Users\Jw\AppData\Local\Arduino15\packages\esp32\hardware\esp32\2.0.9\libraries\SPI\src\SPI.h


*/

// Set to 1 when compiling for ESP_HOME
#define ESP_HOME 1



#include <nRF24L01.h>  
#include <RF24.h>

#define CE_PIN 6
#define CSN_PIN 5

// Specifics for EspHome 
#if ESP_HOME
    #include "esphome.h"
    //#include <real_time_clock.h>
    #define print(x) ESP_LOGD("custom_component", x);
    class nrfmaster : public Component, public CustomAPIDevice {
    public:
#else
    #define print(x) Serial.println(x);
#endif

//RF24 radio(CE_PIN, CSN_PIN);  // Init radio object


// Variables
uint8_t  
Node_start,                                         // Pump cant start again within 10s (0)                                      
Pump_time = 2,                                      // How long should the pump run? in seconds
NTP_time,
Sender_address;                                     // Init
                   
const uint8_t                                           
This_dev_address = 0;                               // Which pipe + device address            User Controlled
//const address[][6] =  
//{ "1Node", "2Node", "3Node", "4Node", "5Node" };    // Address array for "This_dev_address"      

unsigned long
Prev_millis,                                        // Init
Current_millis;                                     // Init

const unsigned long
ADC_interval = 1000UL*60*60*2;                      // How often send ADC data?               User Controlled











struct Rx_package {
    String info;                                // what kind of message 
    const uint8_t from;      // Who is this message from               
    float bat_charge;                           // Battery charge left as %
  };




// Send message 
bool Send_message(uint8_t To_which_node = 0, bool On_off = false) {
    switch (To_which_node) {

        // To NRF node 1 
        case 1:
            struct Tx_package {                                
            uint8_t from;                   // From who                              
            uint8_t Pump_T;                 // How long should the pump run for                             
            unsigned long NTP_T;            // Unix NTP time                             
            bool Pump_On;                   // On/off                              
            };
            // Insert variables
            Tx_package Tx_data;
            Tx_data.from = This_dev_address;         
            Tx_data.Pump_T = Pump_time;              
            Tx_data.NTP_T = NTP_time;                
            Tx_data.Pump_On = On_off;                
            break;

        case 2:
            //node 2
            break;

        default:
            break;       
    }
    // Send message
    //return radio.write( &Tx_data, sizeof(Tx_package) );
    return true;      
}




// Try to send message 3 times
bool Try_send_message(uint8_t To_which_node = 0, bool On_off = false){
  for (int i = 0; i < 3; i++) {
      if (Send_message(To_which_node, On_off)) {
        return true;
      } 
      delay(150);
  }
  return false;
}









// Setup
#if ESP_HOME
void setup() override {
    // Wait for ESPHome to fully initalize 
    //Prev_millis = millis();
    //while (millis() > (Prev_millis + 1000*5)) {}
    register_service(&nrfmaster::NRF_node_1, "hello_world", {"Pump_time"});
    //register_service(&nrfmaster::NRF_node_2, "NRF_node_2", {"","",""...});
    //call_homeassistant_service("service_name", {{"what field", "text input"}});

    //RealTimeClock.set_timezone(UTC+10:00); 
    //RealTimeClock.get_timezone();
    //RealTimeClock.timestamp_now(); 
#else
    void setup() {
    Serial.begin(9600);
#endif

// void setup() {

    


    // Grab current time   



    /* 
    // Radio init
    radio.begin();
    delay(150);
    radio.setPALevel(RF24_PA_MIN);                                          // Transmitter strength   
    radio.setChannel(124);                                                  // Above Wifi 
    radio.setDataRate(RF24_1MBPS);                                          // RF24_2MBPS, RF24_1MBPS, RF24_250KBPS
    radio.openWritingPipe(address[1]);                                      // Only change for master
    radio.openReadingPipe(This_dev_address, address[This_dev_address]);     // What pipe to listen on ALL
    radio.startListening();
    */

}



// NRF_node_1 start call from Home Assistant
void NRF_node_1(int Pump_Time) {
    //Pump_time = Pump_Time;
    //radio.stoptListening();
    //if (Try_send_message(1, true)) {
       //radio.startListening(); 
    //}


    if (Pump_Time == 1){
        print("1");
    } else if (Pump_Time == 2) {
        print("2");
    } else {
        print("Call service");






    }

}





// Main loop
#if ESP_HOME
    void loop() override {
#else
    void loop() {
#endif

// void loop()
    // Nada
    
    //if (millis() > (Prev_millis + 1000*2)){
    //    Prev_millis = millis();
    //    print("loop");
    //}
    ESP_LOGD("custom_component", "test");
    delay(2000);
}





// For EspHome Class
#if ESP_HOME
    };
#endif

