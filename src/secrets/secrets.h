

// secrets.h
// Missing secrets.cpp? Too bad ;)
#pragma once


// MQTT
extern const char* SECRET_MQTT_SSID;
extern const char* SECRET_MQTT_PASSWORD;
extern const char* SECRET_MQTT_IP; // Router IP
extern const int SECRET_MQTT_PORT; // Router port


// RF433 radio codes
// Captured using my terrible /helpers/budget_433Mhz_radio_sniffer.ino
extern const uint16_t SECRET_RF433_REMOTE_CODES[7][132] PROGMEM;


