
/*
######################  NRF Node 1 Functions  ######################
*/

// Functions_Node_1
#pragma once
namespace Functions {

// Setup 
void Setup_everything() {

    // Serial init
    print_init(9600);
    while (!Serial) {
        delay(5);  
    }
    // Fill console field with /n
    #if SERIAL_ON
        for(uint8_t i=0; i<10; i++) {
            print();
        }
        // Radio init
        if (!radio.begin()) {
            print("Radio hardware is not responding!!");
            while (true) {}  
        } 
        else {
            print("Radio online!!");
        }
        if (radio.isChipConnected()) {
            print("nRF24L01 chip is connected.");
        } 
        else {
            print("nRF24L01 chip is NOT connected. Check wiring.");
        }
    #else
        if (!radio.begin()) {
            while (true) {}  
        }  
    #endif

    radio.setPALevel(Radio_output);                                         // Transmitter strength   
    radio.setChannel(Radio_channel);                                        // Radio channel (above wifi) 
    radio.setDataRate(RF24_2MBPS);                                          // Datarate: RF24_2MBPS, RF24_1MBPS, RF24_250KBPS
    radio.openWritingPipe(address[Master_node_address]);                    // Send to master = 0
    radio.openReadingPipe(This_dev_address, address[This_dev_address]);     // What pipe to listen on
    
    // Pins 
    pinMode(PUMP_PIN, OUTPUT);
    digitalWrite(PUMP_PIN, LOW);
    pinMode(ADC_PIN, INPUT);
    delay(5000);

    // Watchdog init
    #if ATtiny84_ON
        wdt_enable(WDTO_8S); 
    #endif 
}


// Package debug
#if !ATtiny84_ON
void Print_all() {
    print(Package.Msg_to_who);                    
    print(Package.Msg_time);                       
    print(Package.Msg_int);                       
    print(Package.Msg_float);                                                                                  
    print(Package.Msg_state);
}
#endif


// Message received, start waterpump for x seconds
void Start_water_pump(uint8_t How_long = 2){
    print(__func__);
    #if ATtiny84_ON
        wdt_reset();
    #endif
    if (How_long > 6) {
        How_long = 1;
    }
    digitalWrite(PUMP_PIN, HIGH);
    delay(How_long);
    digitalWrite(PUMP_PIN, LOW);
    delay(2);  
}


// Watchdog ISR TODO
#if ATtiny84_ON
ISR (WDT_vect) {
    wdt_disable();
    Deepsleep_count ++;
}


// ATtiny84 + NRF24 deepsleep TODO
void Deepsleep() {

    // Turn off everything 
    radio.stopListening();
    radio.powerDown();
    delay(10);
    static byte prevADCSRA = ADCSRA;
    ADCSRA = 0;
    set_sleep_mode(SLEEP_MODE_PWR_DOWN);
    sleep_enable();

    // Wake by watchdog every 8s, go back to sleep if Deepsleep_count < 5400. About 12h
    while (Deepsleep_count < 5400) {
        noInterrupts();
		sleep_bod_disable();

		// Clear various "reset" flags
		MCUSR = 0; 	
		WDTCSR = bit (WDCE) | bit(WDE);                 // WDT mode "interrupt" instead of "reset" 
		WDTCSR = bit (WDIE) | bit(WDP3) | bit(WDP0);    // WDT time interval (max 8s)
        wdt_reset(); 
		interrupts();
		sleep_cpu(); // ZZZZZzzzzzzZZZZzzzzzZZZZZ 
	}
    // 12 hours passed
	sleep_disable();        // Sleep disable
	Deepsleep_count = 0;    // Reset deepsleep counter
	ADCSRA = prevADCSRA;    // Re-enable ADC
    delay(25);
} 
#else


// Pico Fake_deepsleep for testing 
void Fake_deepsleep(){
    print(__func__);
    radio.stopListening();
    radio.powerDown();
    delay(5000);
    print("Aight, back!");
}
#endif


// Hardware reset TODO
void Reset_board_after_15min() {
    print( __func__);
    #if ATtiny84_ON
        unsigned long Timeout = 1000UL*60*15;
        wdt_disable();
        delay(Timeout);
        wdt_enable(WDTO_15MS);
        delay(1000); // Wait for watchdog to reset board
    #endif
}


// Get ADC reading from battery, calc average, convert to %
float Calc_battery_charge() {
    print(__func__);
    float ADC_value = 0;
    for (uint8_t i = 0; i < 100; i++){
        float Value = analogRead(ADC_PIN); 
        ADC_value += Value;
        delay(2);
    }
    ADC_value = ADC_value / 100; 
    float Charge_remaining = (ADC_value - Min_ADC_reading) / (Max_ADC_reading - Min_ADC_reading);   // (x - min) / (max - min) = %
    return Charge_remaining;
}


// Send message 
bool Send_message(uint8_t address = Master_node_address) {
    float Charge_remaining = Calc_battery_charge();
    print(__func__);

    // Insert variables
    Package.Msg_to_who = address;                       // Who is this message for?
    Package.Msg_time = 0;                               // 
    Package.Msg_int = 0;                                // 
    Package.Msg_float = Charge_remaining;               // Battery charge remaining
    Package.Msg_state = false;                          // 
    
    // Send message
    return radio.write( &Package, sizeof(Package) );      
} 


// Try to send message 4 times
bool Try_send_message(uint8_t address = Master_node_address){
    print(__func__);
    for (int i = 0; i < 4; i++) {
        if (Send_message(address)){
            print("Message successful!!");
            return true;
        }
        delay(500);
    }
    print("Message failed!!");
    return false;
}


// Wait for message 
bool Wait_for_message(uint16_t Offset) {
    unsigned long Msg_wait_timer = (Current_millis + Offset);
    uint8_t Pipe;

    // Start listening
    while (true) {
        if (radio.available(&Pipe) && Pipe == This_dev_address) {
            return true;
        }
        // Break after millis ms
        if (millis() > Msg_wait_timer) {
            return false;
        }
    }
}


// Catchy
void Send_ADC_get_NTP() {
    print(__func__);

    // Get NTP time from master or hard reset after 15min
    if (!Try_send_message()) {
        Reset_board_after_15min();
        }

    // Start listening
    radio.flush_rx();
    radio.flush_tx();
    radio.startListening();
    delay(5);

    #if ATtiny84_ON
        wdt_reset();
    #endif 

    // Wait for return message
    Current_millis = millis();
    if (Wait_for_message(2000)) {
        print("Return message successful!!");
        radio.read(&Package, sizeof(Package));
   
        // Expected package size?
        if (sizeof(Package) > 20) {
                radio.flush_rx();
        }
        // Message to variables
        else if (Package.Msg_to_who == This_dev_address) {
            setTime(Package.Msg_time);
        } 
    } 
    else {
        Reset_board_after_15min();
    }
}
}

