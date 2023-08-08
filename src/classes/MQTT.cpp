

// MQTT.cpp
#include "classes/MQTT.h"
#include "classes/RF24Radio.h"
#include "classes/Hardware.h"
#include "secrets/secrets.h"
#include "utility.h"
extern MQTT mqtt;
extern RF24Radio radio;
extern Hardware hardware;


// ##################  MQTT  ##################

// Begin
bool MQTT::begin_object() {
    begin(SECRET_MQTT_IP, SECRET_MQTT_PORT, ETH_client);
    onMessage(MQTT::handle_incoming_MQTT);

    return connect_to_broker();
}


// Connect to MQTT broker
bool MQTT::connect_to_broker() {
    uint32_t previous_timestamp = millis();

    // Wait for connection
    while (!connect("Master_Node", SECRET_MQTT_SSID, SECRET_MQTT_PASSWORD)) {
        print(".");
        delay(1000);
        if (millis() - previous_timestamp > 15*1000UL) {
            return false;
        }
    }
    print("\n");
    
    // Subscribe to MQTT topics
    for (uint8_t i = 0; (MQTT_TOPICS[i] != nullptr); i++) {
        subscribe(MQTT_TOPICS[i]);
    }
    
    return true;
}


// Reconnect or reboot
void MQTT::reconnect_or_reboot() {
    print("MQTT not connected :/ \n");

    for (uint8_t i = 0; i < 3; i++) {
        if (connect_to_broker()) {
            break;
        } else if (i == 2) {
            ESP.restart(); 
        }
        esp_task_wdt_reset(); // Reset Watchdog
        delay(5000);
    } 
}


// Grab current time from Home Assistant via MQTT
void MQTT::get_current_time() {
    PRINT_FUNC(0);
    publish("/to/current_time", "Get", false, 2);
}


// Test connection to router
bool MQTT::test_ETH_connection(const char* host, uint16_t port) {
    print("Connecting to %s \n", host);

    if (!ETH_client.connect(host, port)) {
        print("Connection failed \n");
        return false;
    }
    ETH_client.printf("GET / HTTP/1.1\r\nHost: %s\r\n\r\n", host);
    while (ETH_client.connected() && !ETH_client.available()) {}
        while (ETH_client.available()) {
            Serial.write(ETH_client.read());
        }
    ETH_client.stop();
    return true;
}


// Populate Json doc for sending over MQTT
bool MQTT::populate_Json_document(RF24Radio::DataPackage& pkg, StaticJsonDocument<128>& json_doc, uint8_t RF24_id, uint8_t index) {
    PRINT_FUNC();
    print("\n");
    uint8_t bat_percent = 0;
    json_doc["Node"] = RF24_id;
    switch (RF24_id) {

        // Self watering plants
        case 1:
        case 2:
        case 3: 
            bat_percent = utility::calc_remaining_battery_charge(RF24_id, pkg.message_container[index][3]);

            json_doc["water_level_percent"] = pkg.message_container[index][2];   
            json_doc["battery_percent"] = bat_percent;

            print("Node %i status: \n- Water level = %i%% \n- Battery charge = %i%% \n", 
                RF24_id, pkg.message_container[index][2], bat_percent);
            break;

        // Mocca master controller
        case 11:
            json_doc["coffee_count"] = pkg.message_container[index][3];
            json_doc["coffee_machine_state"] = pkg.message_container[index][4];

            print("Node %i status: \n- Coffee count = %i \n- Coffee state = %s \n", 
                RF24_id, pkg.message_container[index][3], (pkg.message_container[index][4]) ? "On" : "Off");
            break;

        default:
            return false;
    }
    return true;
}


// Send node data to DB
void MQTT::send_RF24_data_to_db(uint8_t index, RF24Radio::DataPackage& pkg) {
    PRINT_FUNC();
    uint8_t RF24_id = pkg.message_container[index][0];
    bool send_MQTT = true;
    
    // Populate Json doc, dependant on node
    StaticJsonDocument<128> json_doc;
    send_MQTT = populate_Json_document(pkg, json_doc, RF24_id, index); 

    // Send Json doc over MQTT
    if (send_MQTT) {
        String topic = "/from/rf24/" + String(RF24_id);
        String payload;
        serializeJson(json_doc, payload);
        publish(topic.c_str(), payload.c_str(), false, 2);
    } 
} 
  

// Calculate to which node and send message
void MQTT::route_MQTT_to_RF24_nodes(const String& topic, const String& payload) { 
    PRINT_FUNC();
    uint16_t topic_length = topic.length();
    uint8_t RF24_node;

    // Which node? (1-99)
    if (topic_length == 10) {
        RF24_node = topic.substring(9, 10).toInt();
    } else if (topic_length == 11) {
        RF24_node = topic.substring(9, 11).toInt();
    } else { return; }

    // Parse the JSON data + Switch block
    StaticJsonDocument<128> jsonDoc;
    deserializeJson(jsonDoc, payload);

    switch (RF24_node) {     
        case 1:
        case 2: {         
            uint8_t pump_time = jsonDoc["pump_time"];
            uint8_t node_state = jsonDoc["node_state"];
            radio.try_send_message(RF24_node, pump_time, 0, node_state, 0);
            break;
            }

        case 11: {      
            uint8_t node_state = jsonDoc["node_state"];
            radio.try_send_message(RF24_node, 0, 0, node_state, 0);
            break;
            } 
            
        default:
            break;
    }
}


// Grab available MQTT message
void MQTT::handle_incoming_MQTT(String& topic, String& payload) {
    PRINT_FUNC(1,1);
    print("\n\n######### MQTT ######### \ntopic = %s \npayload = %s \n\n\n", topic.c_str(), payload.c_str());
    
    try {
        uint16_t payload_length = payload.length();

        // Message to radio 24 Node
        if ((topic.substring(0, 9) == "/to/rf24/") && (payload_length >= 5 && payload_length <= 60)) {
            mqtt.route_MQTT_to_RF24_nodes(topic, payload);
        
        // Message to 433Mhz radio
        } else if ((topic.substring(0, 10) == "/to/rf433/") && (payload_length == 1)) {
            uint8_t remote_code = hardware.calc_RF433_remote_codes(topic);
            hardware.send_RF433_remote_codes(remote_code);
        
        // Set the current time
        } else if (topic == "/from/current_time" && payload_length == 10) {
            utility::set_current_time(payload);
            mqtt.prev_timestamp = millis();
        }
        
    } catch (...) {
        print("Error! Caught in handle_incoming_MQTT \n"); 
    }
} 


