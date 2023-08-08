

// Hardware.cpp
#include "classes/Hardware.h"
#include "classes/RF24Radio.h"
#include "classes/MQTT.h"
#include "secrets/secrets.h"
#include "utility.h"
extern RF24Radio radio;
extern MQTT mqtt;


// ##################  Hardware  ##################

// Buttons: 1_on, 1_off, 2_on, 2_off, 3_on, 3_off, All_off
void Hardware::send_RF433_remote_codes(uint8_t code) {
    PRINT_FUNC();
    print("Button = %i \n", code); 
    if (!code) { return; }

    // Number of retries
    for (uint8_t i = 0; i < RF433_TRANSMISSION_COUNT; i++) {

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
uint8_t Hardware::calc_RF433_remote_codes(const String& topic) {
    PRINT_FUNC();
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
void Hardware::ETH_event(WiFiEvent_t event) {
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
            print("%s", ETH.macAddress().c_str());
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


// Reboot each monday at ca 04:00
void Hardware::reboot_once_a_week() {
    uint16_t current_minute = (hour() * 60) + minute();
 
    if ((weekday() == 2) && (current_minute >= 238 && current_minute <= 239)) {
        delay(2*60*1000); // Budget solution 
        ESP.restart();
    } 
}


// Hardware tests
bool Hardware::setup_hardware() {
    bool status = true;
    
    // Ethernet
    if (!mqtt.test_ETH_connection(LOCAL_ROUTER_IP, LOCAL_ROUTER_PORT)) {
        print("Router \tOffline! \n");
        status = false;      
    }
    // NRF24L01+ radio
    if (!radio.begin_object()) {
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
bool Hardware::setup() {
    
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
    WiFi.onEvent(Hardware::ETH_event);
    ETH.begin(ETH_ADDR, ETH_POWER_PIN, ETH_MDC_PIN, ETH_MDIO_PIN, ETH_TYPE, ETH_CLK_MODE);
    ETH.config(IPAddress(192, 168, 1, 25), IPAddress(192, 168, 1, 1), IPAddress(255, 255, 255, 0));
    WiFi.mode(WIFI_OFF);  

    // Test and initialize hardware objects
    bool hardware_status = setup_hardware();
        
    // 433Mhz radio physical I/O
    pinMode(RF433_TX_PIN, OUTPUT);
    pinMode(RF433_RX_PIN, INPUT);   

    // Allow things to settle down. Begin Watchdog
    delay(3000);
    esp_task_wdt_init(WATCHDOG_TIMEOUT, true);
    esp_task_wdt_add(NULL);
    radio.startListening(); 

    return hardware_status;
}

