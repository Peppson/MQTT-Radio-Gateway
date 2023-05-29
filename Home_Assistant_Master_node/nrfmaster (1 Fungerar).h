
/*
######################### NRF 0 Master #########################
#                                                              #
# - EspHome, NRF24L01 bridge to Home Assistant                 #                                          
# - Library, TMRh20/RF24: https://github.com/tmrh20/RF24/      #     
# - Class config: https://nrf24.github.io/RF24/classRF24.html  #     
# - NRF24L01 Address "0"                                       #
#                                                              #
################################################################
*/


// Set to 1 when compiling for ESP_HOME
#define ESP_HOME 1

#include <nRF24L01.h>  
#include <RF24.h>
#define CE_PIN 6
#define CSN_PIN 5

// Address array for "This_dev_address". Must be declared before "class nrfmaster"?????
const uint8_t
address[][6] = 
{ "1Node", "2Node", "3Node", "4Node", "5Node" };     

// Specifics for ESPHome 
#if ESP_HOME
    #include "esphome.h"
    #define print(x) ESP_LOGD("custom_component", x);
    class nrfmaster : public Component, public CustomAPIDevice {
    public:
#else
    #define print(x) Serial.println(x);
#endif



// Variables
uint8_t                                    
Pump_time = 2,                                      // Node 1 variable
NTP_time,                                           
Sender_address,                                     // Init
This_dev_address = 0;                               // Device address 0 = master              User Controlled

unsigned long
Prev_millis,                                        // Init
Current_millis;                                     // Init

const unsigned long
ADC_interval = 1000UL*60*60*2;                      // How often send ADC data?               User Controlled

/*
struct Rx_package {
    String info;                                // what kind of message 
    const uint8_t from;                         // Who is this message from               
    float bat_charge;                           // Battery charge left as %
  };
*/


// Setup
//RF24 radio(CE_PIN, CSN_PIN);  // Init radio object

#if ESP_HOME
void setup() override {
    register_service(&nrfmaster::NRF_node_1, "version_2", {"Pump_time"});           // Create Bridge to HA 
    //register_service(&nrfmaster::NRF_node_2, "NRF_node_2", {"","",""...});        // From HA to dev 
    //call_homeassistant_service("service_name", {{"what field", "text input"}});   // From dev to HA
#else
    void setup() {
    Serial.begin(9600);
#endif

// void setup() {

    // Grab current time
    //Prev_millis = millis();
       
    /*
    // Radio init
    radio.begin();
    delay(150);
    radio.setPALevel(RF24_PA_MIN);                                          // Transmitter strength   
    radio.setChannel(124);                                                  // Above Wifi 
    radio.setDataRate(RF24_1MBPS);                                          // RF24_2MBPS, RF24_1MBPS, RF24_250KBPS                                      
    
    
    radio.openReadingPipe(This_dev_address, address[This_dev_address]);     // What pipe to listen on ALL
    radio.startListening();
    
    // Set the addresses for all pipes to TX nodes
    for (uint8_t i = 0; i < 5; ++i)
      radio.openReadingPipe(i, address[i]);
    */
}


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
        
        // To NRF node 2 
        case 2:
            break;  

        default:
            return false;
            break;       
    }
    // Send message
    //radio.openWritingPipe(address[To_which_node]);
    //return radio.write( &Tx_data, sizeof(Tx_package) );
    return true;    
}


// Try to send message 3 times
bool Try_send_message(uint8_t To_which_node = 0, bool On_off = false){
  for (int i = 0; i < 3; i++) {
      if (Send_message(To_which_node, On_off)) {
        return true;
      } 
      delay(75); // Eventually falls inside a nodes "listening window"
  }
  return false;
}


// NRF_node_1 start, call from Home Assistant
void NRF_node_1(int Pump_time) {

    //radio.stoptListening();
    //if (Try_send_message(1, true)) {
        //print("Message successful");
    //}
    //radio.startListening();

    
    if (Pump_time == 1) {
        print("Number is 1");
    } else if (Pump_time == 2) {
        print("Number is 2");
    } else {
        // call_homeassistant_service("notify.mobile_app_iphone_jarka", {{"message", "yep!"}});   // test    
    }
}


/*
// NRF_node_2 start, call from Home Assistant
void NRF_node_2(<variable>) {
}
*/


// Main loop
#if ESP_HOME
    void loop() override {
#else
    void loop() {
#endif

// void loop()
    print(".");
    delay(2000);
}


// For EspHome Class
#if ESP_HOME
    };
#endif

// call_homeassistant_service("notify.mobile_app_iphone_jarka", {{"message", "yep!"}});   // test
