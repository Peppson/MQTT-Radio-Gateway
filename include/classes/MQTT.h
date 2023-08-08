

// MQTT.h
#pragma once
#include "config.h"
#include "classes/RF24Radio.h"


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

        // Reconnect or reboot ESP32 board
        void reconnect_or_reboot();

        // Grab current time from Home Assistant
        void get_current_time();

        // Test connection to router
        bool test_ETH_connection(const char * host, uint16_t port);

        // Populate Json doc for sending over MQTT
        inline bool populate_Json_document(RF24Radio::DataPackage& pkg, 
                                            StaticJsonDocument<128>& json_doc, 
                                            uint8_t RF24_id, 
                                            uint8_t index);

        // Send node data to DB (InfluxDB)
        void send_RF24_data_to_db(uint8_t index, RF24Radio::DataPackage& pkg);

        // Calculate to which node and send message
        void route_MQTT_to_RF24_nodes(const String& topic, const String& payload); 
        
        // Grab available MQTT message 
        static void handle_incoming_MQTT(String& topic, String& payload);
    
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


