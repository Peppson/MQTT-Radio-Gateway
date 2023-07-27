

// classes.h
#pragma once
#include "config.h"
#include "utility.h"


// NRF24L01+ Radio "radio_24"
class RF24Radio : public RF24 {
    public:
        // Constructor
        RF24Radio(uint8_t ce = SPI_1_CE, uint8_t csn = SPI_1_CSN) : RF24(ce, csn) {
            spi_bus_1.begin(SPI_1_SCLK, SPI_1_MISO, SPI_1_MOSI, SPI_1_CE);
        };
        // Begin
        bool begin_object();

        // Send message 
        bool send_message(const uint16_t* local_RF24_package_ptr); 

        // Try to send message (parameter hell)
        bool try_send_message(uint16_t arg_to_who = 0, uint16_t arg_int = 0, uint16_t arg_float = 0, uint16_t arg_state = 0, uint16_t arg_time = 0); 

        // Grab available RF24 message
        bool get_available_message(uint8_t& sending_node_1, uint8_t& sending_node_2);

        // Respond to radio 24 Node
        void respond_to_node(uint8_t radio_24_node = 0);

        // Calculate to which node and send message
        void calculate_destination_node_and_send(const String& topic, const String& payload);

        // Handle incoming radio messages
        bool handle_incoming_radio(uint32_t& main_loop_iteration, uint8_t& sending_node_1, uint8_t& sending_node_2);

        // Radio message package
        uint16_t message_package[6] = {};

    private:
        // SPI interface bus 1
        SPIClass spi_bus_1;

        // Radio device addresses     
        const uint8_t address[13][6] = {
            "0Adrs", "1Adrs", "2Adrs", "3Adrs", "4Adrs", "5Adrs", 
            "6Adrs", "7Adrs", "8Adrs", "9Adrs", "10Adr", "11Adr", 
            "12Adr" //...
        };        
};



// MQTT Client "mqtt"
class MQTT : public MQTTClient {
    public:
        // Constructor
        MQTT() { 
            spi_bus_2.begin(); 
            prev_timestamp = 0; 
        };  
        // Begin
        bool begin_object();

        // Connect to MQTT broker
        bool connect_to_broker();

        // Send node data to DB (InfluxDB)
        void send_node_data_to_db(uint8_t radio_24_node); 

        // Grab available MQTT message 
        static void handle_incoming_MQTT(String& topic, String& payload);

        // Grab current time from Home Assistant
        void get_current_time();

        // Reconnect or reboot ESP32 board
        void reconnect_or_reboot();

        // Test connection to router
        bool test_ETH_connection(const char * host, uint16_t port);

        // Previous millis() timestamp
        uint32_t prev_timestamp;

    private:
        // SPI interface bus 2
        SPIClass spi_bus_2;

        // Ethernet RJ45 connection
        WiFiClient ETH_client;

        // Subscribed MQTT topics
        const char* MQTT_TOPICS[4] = {
            "/from/current_time", 
            "/to/rf24/#", 
            "/to/rf433/#",
            nullptr
        }; 
};



// Hardware Class "hardware" 
class HardwareClass {
    public:
        // Transmit 433Mhz remote codes
        // Buttons: 1_on, 1_off, 2_on, 2_off, 3_on, 3_off, All_off
        inline void transmit_RF433_remote_codes(uint8_t button, uint8_t retries = 5);

        // Calculate which RF433 code to send
        uint8_t calculate_rf433_remote_codes(const String& topic);

        // Ethernet debug
        static void ETH_event(WiFiEvent_t event);

        // Hardware tests
        bool init_hardware_objects();

        // Setup
        bool setup();
}; 


