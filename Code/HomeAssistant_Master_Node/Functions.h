
/*
########################  Master Functions  #########################
*/


// Functions.h/cpp
#pragma once
#include "Config.h"


namespace MQTT {
void Send_node_data_to_db(uint8_t nrf_node);
}


namespace NRF24 {

// Send message 
bool Send_message(uint16_t Local_NRF24_package[6]) {
    println(__func__); 

    for (uint8_t i = 0; i < 6; i ++) { 
        // Insert Msg ID in the upper 4 bits
        uint16_t Payload = (i << 12) | Local_NRF24_package[i];
 
        // Send message(s)
        if (!radio.write(&Payload, sizeof(Payload))) {          
            for (uint8_t j = 0; j < 3; j ++) {                  // Retry loop foreach [i]        
                delay(25);
                if(radio.write(&Payload, sizeof(Payload))) {    // Exit retry loop if successful
                    break;                                      
                }
                else if (j == 2) {
                    return false;
                }  
            }
        }
        delay(5);      
    }
    return true;
} 


// Try to send message (parameter hell)
bool Try_send_message(uint16_t arg_to_who = 0, uint16_t arg_int = 0, uint16_t arg_float = 0, uint16_t arg_state = 0, uint16_t arg_time = 0) {
    print(__func__);
    print(" to Node: ");
    println(arg_to_who);
    radio.stopListening();
    radio.openWritingPipe(address[arg_to_who]);

    // Create local copy of NRF24_package[]  
    uint16_t Local_NRF24_package[6]; 
    
    // Attiny84s have 2 bytes for Tx and Rx buffers...
    // splitting up package into small chunks of data                                
    Local_NRF24_package[0] = arg_to_who;                // To who
    Local_NRF24_package[1] = This_dev_address;          // From who
    Local_NRF24_package[2] = arg_int;                   // "Int"
    Local_NRF24_package[3] = arg_float;                 // "Float"
    Local_NRF24_package[4] = arg_state;                 // "Bool"
    Local_NRF24_package[5] = arg_time;                  // Time

    for (uint8_t i = 0; i < 10; i ++) {
        if (NRF24::Send_message(Local_NRF24_package)) {
            println("Message successful!");
            radio.startListening(); 
            return true;
        } 
        delay(175); // Eventually falls inside a nodes "listening window"
    }
    println("Message failed!");
    return false;
} 


// Grab available NRF24 message
bool Get_available_message() {
    println();
    println(__func__);
    unsigned long Msg_timer = millis() + 4 * 1000UL ;
    uint16_t First_node_ID;
    uint16_t Loop_node_ID;
    uint16_t Message;
        
    // Grab 6 packages in rapid succession
    for (uint8_t i = 0; i < 6; i ++) {
        if (radio.available()) {     
            radio.read(&Message, sizeof(Message));
            if (i == 0) {
                First_node_ID = (Message >> 12) & 0x0F;     // Get only Node_ID of the first message
                Loop_node_ID = First_node_ID;
            } 
            else {
                Loop_node_ID = (Message >> 12) & 0x0F;      // Get only Node_ID     
            }
            NRF24_package[i] = Message & 0x0FFF;            // Get only message 
            radio.flush_rx();

            // Node_ID didnt match! (2 Nodes sending at once)
            if (First_node_ID != Loop_node_ID) {
                println("Error! Node ID missmatch");
                return false;
            }
            // Wait for next NRF24_package[i]. Break if time is up
            else if (i < 5) {                                    
                while (!radio.available()) {
                    if (millis() > Msg_timer) {
                        println("Error! Times up");
                        return false;
                    }
                }  
            }
        }        
    }
    return true;
}


// Calculate a NRF_nodes remaining battery charge //TODO
uint16_t Calc_battery_charge_remaining() {
    println(__func__);
    // Li-ion batteries do not discharge linearly!
    // https://www.icode.com/how-to-measure-and-display-battery-level-on-micro-controller-arduino-projects/
    
    print("Node ADC value: ");
    println(NRF24_package[3]);

    // Calculate max 4.2v min? 
    /*
    uint8_t Value = NRF24_package[3];
    if (Value >= 2650) Bat_percent = 100;
    else if (Value >= 2500)    // 85-100% range
        Bat_percent = map(Value, 2500, 2650, 85, 100);
    else if (Value >= 2100)    // 10-85% range
        Bat_percent = map(Value, 2100, 2500, 10, 85);
    else if (Value >= 1700)    // 0-10% range
        Bat_percent = map(Value, 1700, 2100, 0, 10);
    else Bat_percent = 0; */


    //return Bat_percent;
    return 42;
}


// Respond to sending NRF Node 
void Respond_to_sending_node(uint8_t nrf_node = 0) {
    println(__func__);
    print("Message from Node: ");
    println(nrf_node);
    uint16_t Current_time;

    // Convert current time into hhmm format
    if (year() > 2000) {
        Current_time = (hour() * 100 + minute());
    }
    // No time acquired 
    else {
        Current_time = 1200;
    }
    // Send to which node
    switch (nrf_node) {
        case 1: 
            // Respond to NRF_Node 1 with current time
            delay(5);
            NRF24::Try_send_message(nrf_node, 0, 0, 0, Current_time);

            // Send Node data to InfluxDB or Home Assistant
            MQTT::Send_node_data_to_db(nrf_node);
            break;
        case 2:
            // Placeholder 
            break;
        default:
            break;
    }
}
}


namespace RF433 {

// Transmit 433Mhz. Buttons: 1_on, 1_off, 2_on, 2_off, 3_on, 3_off, All_off
void Transmit_RF_remote_codes(uint8_t button, uint8_t retries = 2) {
    println(__func__);
    radio.stopListening();

    // Number of retries
    for (uint i = 0; i < retries; i ++) {

        // Loop through the choosen array, 2 steps at the time
        for (uint8_t j = 0; j < 132; j += 2) {            
            uint16_t High_duration = Captured_RF_remote_codes[button][j];
            uint16_t Low_duration = Captured_RF_remote_codes[button][j + 1];

            // High pulse
            digitalWrite(RF433_TX_PIN, HIGH);
            delayMicroseconds(High_duration);

            // Low pulse
            digitalWrite(RF433_TX_PIN, LOW);
            delayMicroseconds(Low_duration);
        }
        delay(10); 
    }
    radio.startListening();
}
}


namespace MQTT {
    
// Connect to MQTT broker
bool Connect() {
    unsigned long Old_millis = millis();

    // Wait for connecton
    while (!MQTT_client.connect("Master_Node", MQTT_ssid, MQTT_password)) {
        print(".");
        delay(1000);
        if (millis() - Old_millis >= 15*1000UL) {
            return false;
        }
    }
    MQTT_client.subscribe("/Give/Current_time");
    MQTT_client.subscribe("/NRF_Node/1");
    MQTT_client.subscribe("/NRF_Node/2");
    //MQTT_client.unsubscribe("/hello");
    return true;
}


// Send NRF_Node data to DB
void Send_node_data_to_db(uint8_t nrf_node) {
    println(__func__);
    bool Error_flag = false;

    // Json document
    StaticJsonDocument<128> Json_doc;
    String Payload;

    // Insert variables
    switch (nrf_node) {
        case 1: 
            Json_doc["Bat_percent"] = NRF24::Calc_battery_charge_remaining(); 
            break;
        case 2: 
            break;
        default:
            Error_flag = true;
            break;
    }
    if (!Error_flag) {
        serializeJson(Json_doc, Payload);
        String Topic = "/NRF_Node/" + String(nrf_node);
        MQTT_client.publish(Topic.c_str(), Payload.c_str(), false, 2);
    }  
}


// Grab available MQTT message with some logic sprinkled ontop //TODO
void Get_message_and_respond(String &topic, String &payload) {
    //println();
    //print(__func__);
    //println(": " + topic + " - " + payload);

    // Get current time
    if (topic == "/Give/Current_time" && payload.length() == 10) {
        unsigned long Unix_timestamp = payload.toInt();
        setTime(Unix_timestamp);
        print("Time set: ");
        print(hour()); print(":"); print(minute()); print(":"); println(second());
        Prev_millis = millis();
        return;     
    }

    // Message to a RF24 Node
    else if ( (topic.substring(0, 10) == "/NRF_Node/") && (payload.length() >= 20 && payload.length() <= 40) ) {

        // Which node?
        String topic_substring = topic.substring(10, 11);
        uint8_t Which_node = topic_substring.toInt();

        // Parse JSON data + switch block
        StaticJsonDocument<128> jsonDoc;
        deserializeJson(jsonDoc, payload);

        switch (Which_node) {
            
            case 1: {         
                uint8_t Node_state = jsonDoc["Node_state"];
                uint8_t Pump_time = jsonDoc["Pump_time"];
                NRF24::Try_send_message(1, Pump_time, 0, Node_state, 0);
                break;
                } 

            case 2: {
                // Test //TODO
                if (Remove_this_var_TODO == 2) {
                    Remove_this_var_TODO = 3;
                }
                else if (Remove_this_var_TODO == 3) {
                    Remove_this_var_TODO = 2;

                RF433::Transmit_RF_remote_codes(Remove_this_var_TODO);
                }
                break;
                } 

            default:
                break;
        }
    }  
}


// Grab current time from Home Assistant
void Get_current_time() {
    println(__func__);
    MQTT_client.publish("/Get/Current_time", "time", false, 2);
}
}



// Ethernet debug
void WiFiEvent(WiFiEvent_t event) {
    // Copy pasta
    switch (event) {
        case SYSTEM_EVENT_ETH_START:
            println("ETH Started");
            ETH.setHostname("esp32-master"); // Set eth hostname here 
            break;
        case SYSTEM_EVENT_ETH_CONNECTED:
            println("ETH Connected");
            break;
        case SYSTEM_EVENT_ETH_GOT_IP:
            print("ETH MAC: ");
            print(ETH.macAddress());
            print(", IPv4: ");
            print(ETH.localIP());
            if (ETH.fullDuplex()) {
                print(", FULL_DUPLEX");
            }
            print(", ");
            print(ETH.linkSpeed());
            println("Mbps");
            break;
        case SYSTEM_EVENT_ETH_DISCONNECTED:
            println("ETH Disconnected");
            break;
        case SYSTEM_EVENT_ETH_STOP:
            println("ETH Stopped");
            break;
        default:
            break;
    }
}


// Test internet connection
bool Test_ETH_connection(const char * host, uint16_t port) {
    println(__func__);
    print("Connecting to ");
    println(host);

    if (!ETH_client.connect(host, port)) {
        println("Connection failed");
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


// Setup
void Setup_hardware() {
    println(__func__);

    // Serial
    #if SERIAL_ON
        Serial.begin(9600);
        while (!Serial) {
        } 
        // Fill console with \n 
        for(uint8_t i=0; i<25; i++) {
            println();
        }
    #endif

    // SPI
    spiBus1.begin();
    spiBus2.begin(SPI_2_SCLK, SPI_2_MISO, SPI_2_MOSI, SPI_2_CE);

    // Ethernet
    WiFi.onEvent(WiFiEvent);
    ETH.begin(ETH_ADDR, ETH_POWER_PIN, ETH_MDC_PIN, ETH_MDIO_PIN, ETH_TYPE, ETH_CLK_MODE);
    ETH.config(IPAddress(192, 168, 1, 25), IPAddress(192, 168, 1, 1), IPAddress(255, 255, 255, 0));
    WiFi.mode(WIFI_OFF);

    // Test ETH connection
    if (Test_ETH_connection("192.168.1.1", 80)) {
        println("Router OK!");    
    }
    else {
        println("Router offline");  
    }

    // MQTT
    MQTT_client.begin(MQTT_ip, 1883, ETH_client);
    MQTT_client.onMessage(MQTT::Get_message_and_respond);
    if (MQTT::Connect()) { 
        println("MQTT OK!");
    }
    else {
        println("MQTT broker is not responding!");  
    } 

    // NRF24 Radio 
    if (radio.begin(&spiBus2, SPI_2_CE, SPI_2_CSN) && radio.isChipConnected()) {
        println("NRF Radio OK!");
    }
    else {
        println("NRF Radio hardware is not responding!"); 
    }

    // NRF24 radio config
    radio.setPALevel(Radio_output);                                         // Transmitter strength   
    radio.setChannel(Radio_channel);                                        // Radio channel (above wifi) 
    radio.setDataRate(RF24_2MBPS);                                          // Datarate: RF24_2MBPS, RF24_1MBPS, RF24_250KBPS
    radio.openReadingPipe(This_dev_address, address[This_dev_address]);     // What pipe to listen on
    radio.startListening();

    // 433Mhz radio 
    pinMode(RF433_TX_PIN, OUTPUT);
    pinMode(RF433_RX_PIN, INPUT);

    // Allow things to settle down 
    delay(5000);    
}


//######  DEBUG  ######
#if SERIAL_ON
void Print_package() {
    println();
    println("#########  Package  #########");
    print("To who:      ");
    println(NRF24_package[0]);
    print("From who:    ");
    println(NRF24_package[1]);
    print("Int:         ");                      
    println(NRF24_package[2]);
    print("Float:       ");                        
    println(NRF24_package[3]);
    print("Bool:        ");                                                                                   
    println(NRF24_package[4]);
    print("Time:        ");                   
    println(NRF24_package[5]);
    println();
}
#endif

