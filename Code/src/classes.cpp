

// classes.cpp
#include <Arduino.h>
#include "classes.h"
#include "secrets.h"
extern RF24Radio radio_24; 
extern HardwareClass hardware;
extern MQTT mqtt;   
                  
       

// ##################  RF24Radio  ##################

// Begin
bool RF24Radio::begin_object() {
    
    // Initialize 
    if (!(begin(&spi_bus_1, SPI_1_CE, SPI_1_CSN) && isChipConnected())) {
        return false; 
    } 
    // Config
    setPALevel(RF24_OUTPUT, 1);                             // Transmitter output strength   
    setChannel(RF24_CHANNEL);                               // Radio channel
    setDataRate(RF24_DATARATE);                             // Datarate: RF24_2MBPS, RF24_1MBPS, RF24_250KBPS
    openReadingPipe(0, address[RF24_THIS_DEV_ADDRESS]);     // What radio pipe to listen on
    #if SERIAL_ENABLED
        printDetails();
    #endif
    startListening();

    return true;
}


// Send message 
bool RF24Radio::send_message(const uint16_t* local_RF24_package_ptr) {
    for (uint8_t i = 0; i < 6; i++) {

        // Insert Msg ID in the upper 4 bits
        uint16_t payload = (i << 12) | local_RF24_package_ptr[i];

        // Send message(s)
        if (!write(&payload, sizeof(payload))) { 

            // Retry loop foreach index
            for (uint8_t j = 0; j < 3; j++) {                       
                delay(25);
                
                // Exit retry loop if successful
                if (write(&payload, sizeof(payload))) {     
                    break;                                      
                }
                // Failed
                else if (j == 2) {
                    return false;
                }  
            }
        }
        delay(15); // Important! Takes about 8ms for Attinys to read content of radio buffer  
    }
    return true;
}


// Try to send message (parameter hell)
bool RF24Radio::try_send_message(uint16_t arg_to_who, uint16_t arg_int, uint16_t arg_float, uint16_t arg_state, uint16_t arg_time) {
    stopListening();
    openWritingPipe(address[arg_to_who]);

    // Create local array  
    uint16_t local_RF24_package[6] = {}; 
    
    // Attiny84s have 2 bytes for Tx and Rx buffers...
    // splitting up package into small chunks of data                                
    local_RF24_package[0] = arg_to_who;                 // To who
    local_RF24_package[1] = RF24_THIS_DEV_ADDRESS;      // From who
    local_RF24_package[2] = arg_int;                    // "Int"
    local_RF24_package[3] = arg_float;                  // "Float"
    local_RF24_package[4] = arg_state;                  // "Bool"
    local_RF24_package[5] = arg_time;                   // "Time"

    for (uint8_t i = 0; i < 10; i++) {
        if (send_message(local_RF24_package)) {
            print("Message successful! \n");
            startListening(); 
            return true;
        } 
        delay(175); // Eventually falls inside a nodes "listening window"
    }
    print("Message failed! \n");
    startListening();
    return false;
} 


// Grab available RF24 message
bool RF24Radio::get_available_message(uint8_t& sending_node_1, uint8_t& sending_node_2) {
    uint32_t timer = millis() + 500UL; //TODO
    uint16_t payload;
    uint8_t first_node_ID;
    uint8_t loop_node_ID;
    bool is_error = false;
    
    // Grab 6 packages in quick succession
    for (uint8_t i = 0; i < 6; i++) { 
        read(&payload, sizeof(payload));

        // node_ID of the first message
        if (i == 0) {
            first_node_ID = (payload >> 12) & 0x0F;             
            loop_node_ID = first_node_ID; 

        // node_ID for the rest
        } else {
            loop_node_ID = (payload >> 12) & 0x0F;                       
        }
        // "Decode" message
        message_package[i] = payload & 0x0FFF;                       
        print("ID: %-2i    I: %i   Msg: %-4i \n", loop_node_ID, i, message_package[i]);

        // Wait for next message. Break if time is up
        while (i < 5 && !available()) {                                     
            if (millis() > timer) {
                print("Error! Times up \n");
                stopListening();
                is_error = true;
                break;
            }
        }  
    }
    // Node_ID didnt match! (2 or more Nodes sending at once)
    if (first_node_ID != loop_node_ID) {
        print("Error! Node ID missmatch \n");
        stopListening();
        is_error = true; 
        }
    if (is_error) {
        sending_node_1 = first_node_ID;
        sending_node_2 = loop_node_ID;
        return false;
    }
    return true; 
} 


// Respond to radio 24 Node 
void RF24Radio::respond_to_node(uint8_t radio_24_node) {
    print_func(0);
    print(" [%i] ", radio_24_node); 
    uint16_t current_time;

    // Convert current time into hhmm format
    #if !FAKE_TIME_ENABLED
        if (year() > 2000) {
            current_time = (hour() * 100 + minute());
        } else { 
            current_time = 1200;
        }
    #else
        current_time = 1200; 
    #endif

    // Respond to Node with current time
    delay(10);
    try_send_message(radio_24_node, 0, 0, 0, current_time);
}


// Calculate to which node and send message
void RF24Radio::calculate_destination_node_and_send(const String& topic, const String& payload) { 
    print_func(1, 1);
    uint16_t topic_length = topic.length();
    uint8_t radio_24_node;

    // Which node? (1-99)
    if (topic_length == 10) {
        radio_24_node = topic.substring(9, 10).toInt();
    } else if (topic_length == 11) {
        radio_24_node = topic.substring(9, 11).toInt();
    } else {
        return;
    }
    // Parse the JSON data + Switch block
    StaticJsonDocument<128> jsonDoc;
    deserializeJson(jsonDoc, payload);

    switch (radio_24_node) {     
        case 1:
        case 2: {         
            uint8_t pump_time = jsonDoc["pump_time"];
            uint8_t node_state = jsonDoc["node_state"];
            radio_24.try_send_message(radio_24_node, pump_time, 0, node_state, 0);
            break;
            }

        case 11: {      
            uint8_t node_state = jsonDoc["node_state"];
            radio_24.try_send_message(radio_24_node, 0, 0, node_state, 0);
            break;
            } 
            
        default:
            break;
    }
}


// Handle incoming radio messages
bool RF24Radio::handle_incoming_radio(uint32_t& main_loop_iteration, uint8_t& sending_node_1, uint8_t& sending_node_2) {
    print_func(1, 2);
    
    // Grab message, return true in case of error
    if (!radio_24.get_available_message(sending_node_1, sending_node_2)) {
        main_loop_iteration = 0;
        delay(5000);  
        return true;
    }
    // Easier reading
    uint16_t to_who = message_package[0];
    uint16_t from_who = message_package[1];
    uint16_t request_time = message_package[5];
    print("\n");

    // Node status update
    if (to_who == RF24_THIS_DEV_ADDRESS && !request_time) {
        print("Node status update! \n");

    // Node request current time
    } else {
        print("Node requests the current time! \n");
        radio_24.respond_to_node(from_who);
    }
    // Send node data to InfluxDB
    mqtt.send_node_data_to_db(from_who);

    return false;
}




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


// Send node data to DB
void MQTT::send_node_data_to_db(uint8_t radio_24_node) {
    print_func(0);
    print(" [%i] \n", radio_24_node);
    StaticJsonDocument<128> json_doc;
    uint8_t bat_percent = 0;
    bool send_MQTT = true;
    
    // Insert radio_24.message_package variables, dependant on destination node
    json_doc["Node"] = radio_24_node;
    switch (radio_24_node) {

        case 1:
            bat_percent = utility::calc_remaining_battery_charge(radio_24_node);
            json_doc["water_level_percent"] = radio_24.message_package[2];   
            json_doc["battery_percent"] = bat_percent;
            print("Node %i status: \n- Water level = %i%% \n- Battery charge = %i%% \n", radio_24_node, radio_24.message_package[2], bat_percent);
            break;

        case 11:
            json_doc["coffee_count"] = radio_24.message_package[3];
            json_doc["coffee_machine_state"] = radio_24.message_package[4];
            print("Node %i status: \n- Coffee count = %i \n- Coffee state = %s \n", 
            radio_24_node, radio_24.message_package[3], (radio_24.message_package[4]) ? "On" : "Off");
            break;

        default:
            send_MQTT = false;
            break;
    }
    // Send Json doc over MQTT
    if (send_MQTT) {
        String topic = "/from/rf24/" + String(radio_24_node);
        String payload;
        serializeJson(json_doc, payload);
        publish(topic.c_str(), payload.c_str(), false, 2);
    } 
} 
  

// Grab available MQTT message
void MQTT::handle_incoming_MQTT(String& topic, String& payload) {
    print_func();
    try {
        uint16_t payload_length = payload.length();

        // Message to radio 24 Node
        if ((topic.substring(0, 9) == "/to/rf24/") && (payload_length >= 5 && payload_length <= 60)) {
            radio_24.calculate_destination_node_and_send(topic, payload);
        
        // Message to 433Mhz radio
        } else if ((topic.substring(0, 10) == "/to/rf433/") && (payload_length == 1)) {
            uint8_t remote_code = hardware.calculate_rf433_remote_codes(topic);
            hardware.transmit_RF433_remote_codes(remote_code);
        
        // Set current time
        } else if (topic == "/from/current_time" && payload_length == 10) {
            utility::set_current_time(payload);
            mqtt.prev_timestamp = millis();
        }
        
    } catch (...) {
        print("Error! Caught in handle_incoming_MQTT \n"); 
    }
} 


// Grab current time from Home Assistant via MQTT
void MQTT::get_current_time() {
    print_func();
    publish("/to/current_time", "Get", false, 2);
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


// Test connection to router
bool MQTT::test_ETH_connection(const char* host, uint16_t port) {
    print("Connecting to %s \n", host);

    if (!ETH_client.connect(host, port)) {
        print("Connection failed \n");
        return false;
    }
    ETH_client.printf("GET / HTTP/1.1\r\nHost: %s\r\n\r\n", host);
    while (ETH_client.connected() && !ETH_client.available());
        while (ETH_client.available()) {
            Serial.write(ETH_client.read());
    }
    ETH_client.stop();
    return true;
}




// ################  HardwareClass  ################

// Transmit 433Mhz remote codes
// Buttons: 1_on, 1_off, 2_on, 2_off, 3_on, 3_off, All_off
void HardwareClass::transmit_RF433_remote_codes(uint8_t code, uint8_t retries) {
    print_func();
    print("Button = %i \n", code); 
    if (!code) { return; }

    // Number of retries
    for (uint8_t i = 0; i < retries; i++) {

        // Loop through the chosen "remote code" array
        for (uint16_t j = 0; j < 131; j += 2) {            
            uint16_t high_pulse = SECRET_RF433_REMOTE_CODES[code][j];
            uint16_t low_pulse = SECRET_RF433_REMOTE_CODES[code][j + 1];

            // High pulse
            digitalWrite(RF433_TX_PIN, HIGH);
            delayMicroseconds(high_pulse);

            // Low pulse
            digitalWrite(RF433_TX_PIN, LOW);
            delayMicroseconds(low_pulse);
        }
        delayMicroseconds(9300); // Simulates low pulse between messages
    }
}


// Calculate which 433Mhz code to send
uint8_t HardwareClass::calculate_rf433_remote_codes(const String& topic) {
    print_func();
    uint16_t topic_length = topic.length();
    uint8_t remote_code = 0;
    
    // 1 or 2 digits? (1-99)
    if (topic_length == 11) {
        remote_code = topic.substring(10, 11).toInt();
    } else if (topic_length == 12) {
        remote_code = topic.substring(10, 12).toInt();
    }

    return remote_code;
}


// Ethernet debug
void HardwareClass::ETH_event(WiFiEvent_t event) {
    // Copy pasta
    switch (event) {

        case SYSTEM_EVENT_ETH_START:
            print("ETH Started \n");
            // Set ETH hostname here
            ETH.setHostname("esp32-master"); 
            break;

        case SYSTEM_EVENT_ETH_CONNECTED:
            print("ETH Connected \n");
            break;

        case SYSTEM_EVENT_ETH_GOT_IP:
            print("ETH MAC: ");
            print("%s", ETH.macAddress());
            print(", IPv4: ");
            print("%i", ETH.localIP());
            if (ETH.fullDuplex()) {
                print(", FULL_DUPLEX \n");
            }
            print(", ");
            print("%i", ETH.linkSpeed());
            print("Mbps \n");
            break;

        case SYSTEM_EVENT_ETH_DISCONNECTED:
            print("ETH Disconnected \n");
            break;

        case SYSTEM_EVENT_ETH_STOP:
            print("ETH Stopped \n");
            break;

        default:
            break;
    }
}


// Hardware tests
bool HardwareClass::init_hardware_objects() {
    bool status = true;
    
    // Ethernet
    if (!mqtt.test_ETH_connection(LOCAL_ROUTER_IP, LOCAL_ROUTER_PORT)) {
        print("Router \tOffline! \n");
        status = false;      
    }
    // NRF24L01+ radio
    if (!radio_24.begin_object()) {
        print("Nrf24l01+ Offline! \n");
        status = false;  
    }
    // MQTT client
    if (!mqtt.begin_object()) {
        print("MQTT \tOffline! \n");
        status = false;   
    }
    if (status) {
        print("Router \t\tOK! \nNrf24l01+\tOK! \nMQTT \t\tOK! \n\n");
    }
    return status; 
}


// Setup
bool HardwareClass::setup() {
    
    // Serial
    #if SERIAL_ENABLED
        Serial.begin(9600);
        while (!Serial && millis() < 30*1000UL) {}

        // Fill console with \n 
        for (uint8_t i = 0; i < 30; i++) { 
            print("\n"); 
        }
    #endif

    // Ethernet
    WiFi.onEvent(HardwareClass::ETH_event);
    ETH.begin(ETH_ADDR, ETH_POWER_PIN, ETH_MDC_PIN, ETH_MDIO_PIN, ETH_TYPE, ETH_CLK_MODE);
    ETH.config(IPAddress(192, 168, 1, 25), IPAddress(192, 168, 1, 1), IPAddress(255, 255, 255, 0));
    WiFi.mode(WIFI_OFF);  

    // Test and initialize hardware objects
    bool hardware_status = init_hardware_objects();
        
    // 433Mhz radio physical I/O
    pinMode(RF433_TX_PIN, OUTPUT);
    pinMode(RF433_RX_PIN, INPUT);   

    // Allow things to settle down. Begin Watchdog
    delay(3000);
    esp_task_wdt_init(WATCHDOG_TIMEOUT, true);
    esp_task_wdt_add(NULL);

    return hardware_status;
}


