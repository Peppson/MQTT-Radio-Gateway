

// RF24Radio.h
#pragma once
#include "config.h"


// NRF24L01+ Radio "radio"
class RF24Radio : public RF24 {
    public:
        struct DataPackage;

        // Constructor
        RF24Radio(uint8_t ce = SPI_1_CE, uint8_t csn = SPI_1_CSN) : RF24(ce, csn) {
            spi_bus_1.begin(SPI_1_SCLK, SPI_1_MISO, SPI_1_MOSI, SPI_1_CE);
        };
        // Begin
        bool begin_object();

        // Send message 
        bool send_message(const uint16_t* local_RF24_package_ptr); 

        // Try to send message (parameter hell)
        bool try_send_message(uint16_t to_who, uint16_t data_0, uint16_t data_1, uint16_t state, uint16_t time);

        // Respond to radio 24 Node
        void respond_to_node(uint8_t RF24_node = 0); 

        // Sort incoming messages into separate arrays, if multiple senders
        inline bool sort_incoming_message(DataPackage& obj);

        // Grab available RF24 message
        inline bool get_available_message(DataPackage& obj);

        // Handle incoming radio messages
        bool handle_incoming_radio();

        // Wait for incomming message
        bool wait_for_message(uint16_t ms);

        // Error! Sending retry msg to each node that failed transmission
        void handle_receive_error();

        // Helps carry around data while receiving messages (5D array?)
        struct DataPackage {
            uint16_t message_container[3][6] = {};      // Holds up to 3 different node messages arrays
            uint16_t new_message = 0;                   // Newest decoded node message
            uint8_t new_ID = 0;                         // Newest decoded node ID
            uint8_t already_seen_node_ID[3] = {};       // Holds already seen node IDs
            uint8_t num_of_packages[3] = {};            // Counts which column per node
            uint8_t num_of_nodes = 0;                   // Counts how many simultaneous sending nodes
            uint8_t row_in_array = 0;                   // Which row aka which node
        };

        // Keeps count if there was any receive errors
        uint8_t m_error_at_node_ID[3] = {};

    private:
        // SPI interface, bus 1
        SPIClass spi_bus_1;

        // Radio device addresses     
        const uint8_t address[13][6] = {
            "0Adrs", "1Adrs", "2Adrs", "3Adrs", "4Adrs", "5Adrs", 
            "6Adrs", "7Adrs", "8Adrs", "9Adrs", "10Adr", "11Adr", 
            "12Adr" //...
        };
};


