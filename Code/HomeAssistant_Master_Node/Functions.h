

/*
########################  Master Functions  #########################
#                                                                   #
#   - MQTT Bridge to NRF24L01 nodes and generic 433Mhz radios       #                                          
#   - Library, TMRh20/RF24: https://github.com/tmrh20/RF24/         #     
#   - Class config: https://nrf24.github.io/RF24/classRF24.html     #     
#   - NRF24L01 Address "1"                                          #
#                                                                   #
#####################################################################
*/


// Functions.h/cpp
#pragma once
#include "Config.h"
//#include Functions.h


// Send message 
bool Send_message(uint16_t Arg_to_who, uint16_t Arg_int, uint16_t Arg_float, uint16_t Arg_state, uint16_t Arg_time) {
    print(__func__);
    print(" to Node: ");
    println(Arg_to_who);

    // Insert variables     
    Message_package[0] = Arg_to_who;            // To who
    Message_package[1] = This_dev_address;      // From who
    Message_package[2] = Arg_int;               // int
    Message_package[3] = Arg_float;             // float
    Message_package[4] = Arg_state;             // Bool
    Message_package[5] = Arg_time;              // Time
      
    // Send message
    radio.openWritingPipe(address[Arg_to_who]);
    for (uint8_t i=0; i < 6; i++) {

        // Insert Msg ID in the upper most 4 bytes
        uint16_t Content = (i << 12) | Message_package[i];

        // Send message
        if (!radio.write(&Content, sizeof(Content))) {
            // Retry loop foreach [i]
            for (uint8_t j=0; j < 3; j++) {                     
                delay(25);
                // Exit retry loop if successful
                if(radio.write(&Content, sizeof(Content))) {
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


// Try to send message (Parameter hell)
bool Try_send_message(uint16_t Arg_to_who = 0, uint16_t Arg_int = 0, uint16_t Arg_float = 0, uint16_t Arg_state = 0, uint16_t Arg_time = 0) {
    println(__func__);
    radio.stopListening();

    for (uint8_t i = 0; i < 10; i++) {
        if (Send_message(Arg_to_who, Arg_int, Arg_float, Arg_state, Arg_time)) {
            println("Message successful!!");
            radio.startListening(); 
            return true;
        } 
        delay(175); // Eventually falls inside a nodes "listening window"
    }
    println("Message failed!!");
    return false;
} 


// Grab available message
bool Get_available_message() {
    println();
    println(__func__);
    unsigned long Msg_timer = millis() + 4 * 1000UL ;
    uint16_t First_node_ID;
    uint16_t Loop_node_ID;
    uint16_t Message;
        
    // Grab 6 packages in rapid succession
    for (uint8_t i=0; i<6; i++) {
        if (radio.available()) {     
            radio.read(&Message, sizeof(Message));
            if (i == 0) {
                First_node_ID = (Message >> 12) & 0x0F;     // Get Node_ID of the first message
                Loop_node_ID = First_node_ID;
            } 
            else {
                Loop_node_ID = (Message >> 12) & 0x0F;      // Get Node_ID for the rest       
            }
            Message_package[i] = Message & 0x0FFF;          // Get message (-) NODE ID 
            radio.flush_rx();

            // Node_ID didnt match! (2 Nodes sending at once)
            if (First_node_ID != Loop_node_ID) {
                println("Error! NodeID missmatch");
                return false;
            }
            // Wait for next Msg. Break if time is up
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


// Calculate a NRF_nodes remaining battery charge
uint16_t Calc_battery_charge_remaining() {
    println(__func__);
    // Li-ion batteries do not discharge linearly!
    // https://www.icode.com/how-to-measure-and-display-battery-level-on-micro-controller-arduino-projects/
    
    print("Battery ADC value: ");
    println(Message_package[3]);

    //TODO
    return 42;
}


// Who_was_sending_and_respond?
void Who_was_sending_and_respond(uint8_t From_who = 0) {
    println(__func__);
    print("Message from Node: ");
    println(From_who);

    // Convert current time into hhmm format (uint16_t)    
    uint16_t Current_time = (hour() * 100 + minute());
    StaticJsonDocument<128> jsonDoc; // Init json file

    // Send to which node?
    switch (From_who) {
        
        // Node_1 (Solarpowered watering flower pot)
        case 1: {
            
            // Insert variables
            jsonDoc["Bat_charge"] = Calc_battery_charge_remaining(); 
            String jsonString;
            serializeJson(jsonDoc, jsonString); 

            // Respond to NRF_Node 1 with current time
            delay(3);
            Try_send_message(From_who, 0, 0, 0, Current_time); 

            // Send info to  InfluxDB or Home Assistant
            MQTT_client.publish("/NRF_Node/1/Battery_remaining", jsonString.c_str(), false, 2);
            break;
            }

        // Node_2
        case 2: {
        break;
        }

        // Error do nothing
        default:
        break;
    }
}


// Ethernet debug
void WiFiEvent(WiFiEvent_t event) {
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
bool Test_eth_connection(const char * host, uint16_t port) {
    println(__func__);
    print("Connecting to ");
    println(host);

    if (!Wifi_client.connect(host, port)) {
        println("Connection failed");
        return false;
    }
    Wifi_client.printf("GET / HTTP/1.1\r\nHost: %s\r\n\r\n", host);
    while (Wifi_client.connected() && !Wifi_client.available());
        while (Wifi_client.available()) {
            Serial.write(Wifi_client.read());
    }
    println("Closing connection");
    Wifi_client.stop();
    return true;
}


// Connect to MQTT broker
bool MQTT_connect() {
    println(__func__);
    unsigned long old_millis = millis();

    while (!MQTT_client.connect("Master_Node", MQTT_ssid, MQTT_password)) {
        print(".");
        delay(1000);
        if (millis() - old_millis >= 15*1000UL) {
            return false;
        }
    }
    MQTT_client.subscribe("/Give/Current_time");
    MQTT_client.subscribe("/NRF_Node/1");
    //MQTT_client.subscribe("/NRF_Node/2");
    //MQTT_client.unsubscribe("/hello");
    return true;
}


// Incomming MQTT message 
void MQTT_Received(String &topic, String &payload) {
    println();
    print(__func__);
    println(": " + topic + " - " + payload);

    // Get current time
    if (topic == "/Give/Current_time" && payload.length() == 10) {
        unsigned long Unix_timestamp = payload.toInt();
        setTime(Unix_timestamp);
        Prev_millis = millis();
        print("Time set:   ");
        print(hour()); print(":"); print(minute()); print(":"); println(second());     
    }

    // Parse JSON data
    StaticJsonDocument<128> jsonDoc;
    deserializeJson(jsonDoc, payload);

    // NRF_Node 1 
    if (topic == "/NRF_Node/1" && (payload.length() >= 20 && payload.length() <= 40)) {         // {"Node_state": 1, "Pump_time": 8}              
        uint8_t Node_state = jsonDoc["Node_state"];
        uint8_t Pump_time = jsonDoc["Pump_time"];
        Try_send_message(1, Pump_time, 0, Node_state, 0);
    }
    // NRF_Node 2
    //else if (payload == "/NRF_Node/2" && (payload.length() >= 20 && payload.length() <= 40)) {
    //}
}


// Grab current time from Home Assistant
void Get_current_time() {
    println(__func__);
    MQTT_client.publish("/Get/Current_time", "time", false, 2);
    delay(5);
}


// Setup
void Setup_hardware() {
    println(__func__);

    // Serial
    #if SERIAL_ON
        // Init
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

    // Check internet connection
    if (Test_eth_connection("192.168.1.1", 80)) {
        println("Internet OK!");    
    }
    else {
        println("Internet is not responding!!");  
    }

    // MQTT
    MQTT_client.begin(MQTT_IP, 1883, Wifi_client);
    MQTT_client.onMessage(MQTT_Received);
    if (MQTT_connect()) { 
        println("MQTT OK!");
    }
    else {
        println("MQTT broker is not responding!!");  
    } 

    // 2.4Ghz Radio 
    if (radio.begin(&spiBus2, SPI_2_CE, SPI_2_CSN) && radio.isChipConnected()) {
        println("2.4Ghz Radio OK!");
    }
    else {
        println("2.4Ghz Radio hardware is not responding!!"); 
    }

    // 2.4Ghz Radio settings
    radio.setPALevel(Radio_output);                                         // Transmitter strength   
    radio.setChannel(Radio_channel);                                        // Radio channel (above wifi) 
    radio.setDataRate(RF24_2MBPS);                                          // Datarate: RF24_2MBPS, RF24_1MBPS, RF24_250KBPS
    radio.openReadingPipe(This_dev_address, address[This_dev_address]);     // What pipe to listen on
    radio.startListening();

    // Misc
    delay(4000); // Allow things to settle down
}


//##############  DEBUG  ##############
#if SERIAL_ON
void Print_package() {
    println();
    println("#########  Package  #########");
    print("To who:      ");
    println(Message_package[0]);
    print("From who:    ");
    println(Message_package[1]);
    print("Int:         ");                      
    println(Message_package[2]);
    print("Float:       ");                        
    println(Message_package[3]);
    print("Bool:        ");                                                                                   
    println(Message_package[4]);
    print("Time:        ");                   
    println(Message_package[5]);
    println();
}
#endif

