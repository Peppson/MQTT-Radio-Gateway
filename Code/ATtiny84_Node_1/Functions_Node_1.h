
/*
######################  NRF Node 1 Functions  ######################
*/

// Functions_Node_1
#pragma once
namespace Functions {


// Setup 
void Setup_everything() {

    // Reset Watchdog on wakeup
    #if ATtiny84_ON
        MCUSR &= ~(1<<WDRF);
        wdt_disable();
    #endif

    // Serial init
    #if SERIAL_ON
        SERIAL_BEGIN(9600);
        while (!Serial) {} 

        // Fill console field with /n
        for(uint8_t i=0; i<25; i++) {
            println();
        }
        // Radio init
        if (!radio.begin() || !radio.isChipConnected()) {
            println("Radio hardware is not responding!!");
            while (true) {}    
        }
        println("Radio OK!"); 
    #else
        if (!radio.begin() || !radio.isChipConnected()) {
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
    pinMode(ADC_PIN, INPUT);
    digitalWrite(PUMP_PIN, LOW);
    delay(1000);
    
    // Watchdog init
    #if ATtiny84_ON
        //wdt_enable(WDTO_8S); 
    #endif  
}


// Message received, start waterpump for x seconds
void Start_water_pump(uint8_t How_long = 2){
    println(__func__);
    WDT_RESET();

    if (How_long >= 8) {
        How_long = 1;
    }
    digitalWrite(PUMP_PIN, HIGH);
    delay(How_long * 1000);
    digitalWrite(PUMP_PIN, LOW);
    delay(2);  
}


// Get ADC reading from battery, calc average, convert to % (0-100%)
uint16_t Battery_charge_remaining() {
    println(__func__);
    WDT_RESET();
    radio.stopListening();
    radio.powerDown();
    delay(10);

    // Grab battery reading while radio is powered off
    float Sum = 0;
    for (uint8_t i = 0; i < 100; i++) {
        int Value = analogRead(ADC_PIN); 
        Sum += Value;
        delay(10);
    }
    float Average = Sum / 100; 
    float Percentage = (Average - Min_ADC_reading) / (Max_ADC_reading - Min_ADC_reading);  // (x - min) / (max - min) = %
    uint16_t Charge_remaining = Percentage * 100;

    radio.powerUp();
    delay(10);
    return Charge_remaining;
}
  

// Send message 
bool Send_message(uint16_t Arg_float) {
    println(__func__);

    // Insert variables and NODE ID in the upper most 4 bytes
    Message_package[0] = (This_dev_address << 12) | Master_node_address;    // Who is this message for?
    Message_package[1] = (This_dev_address << 12) | This_dev_address;       // Who is this message from?
    Message_package[2] = (This_dev_address << 12) | 0;                      // Node_1 how long the pump should run for
    Message_package[3] = (This_dev_address << 12) | Arg_float;              // Battery charge remaining (float[0.00 - 1.00] x 100)
    Message_package[4] = (This_dev_address << 12) | 0;                      // State
    Message_package[5] = (This_dev_address << 12) | 0;                      // Time

    // Send message 
    for (uint8_t i=0; i < 6; i++) {
        if (!radio.write(&Message_package[i], sizeof(Message_package[i]))) {
            // Retry loop foreach [i]
            for (uint8_t j=0; j < 3; j++) { 
                delay(25);
                if(radio.write(&Message_package[i], sizeof(Message_package[i]))) {
                    break; // Exit retry loop if successful
                }
                else if (j == 2) {
                    return false;
                }  
            }
        }
        delay(1);      
    }
    return true;    
} 


// Try to send message n times
bool Try_send_message() {
    uint16_t Charge_remaining = Battery_charge_remaining();
    for (uint8_t i = 0; i < 4; i++) {
        if (Send_message(Charge_remaining)) {
            println("Message successful!!");
            return true;
        }
        delay(200);
    }
    println("Message failed!!");
    return false;
}


// Debug rm
void Send_debug() {
    radio.stopListening();
    Try_send_message();
    radio.startListening();
}


// Watchdog ISR 
#if ATtiny84_ON
ISR (WDT_vect) {
    wdt_disable();
    Deepsleep_count ++;
}


// ATtiny84 + NRF24 deepsleep (about 5 µA)
void Deepsleep() {
    println(__func__);

    // Turn off everything
    WDT_RESET(); 
    radio.stopListening();
    radio.powerDown();
    delay(10);
    static byte prevADCSRA = ADCSRA;
    ADCSRA = 0;
    set_sleep_mode(SLEEP_MODE_PWR_DOWN);
    sleep_enable();

    // Wake by watchdog every 8s, go back to sleep if Deepsleep_count < 4400. About 12h. Each cycle = 9.8s
    while (Deepsleep_count < 1) {
        noInterrupts();
		sleep_bod_disable();

		// Clear various "reset" flags
		MCUSR = 0; 	
		WDTCSR = bit (WDCE) | bit(WDE);                 // WDT mode "interrupt" instead of "reset" 
		WDTCSR = bit (WDIE) | bit(WDP3) | bit(WDP0);    // WDT time interval (max 8s)
        WDT_RESET(); 
		interrupts();
		sleep_cpu(); // ZZZZZzzzzzzZZZZzzzzzZZZZZ 
	}
    // 12 hours passed
	sleep_disable();        // Sleep disable
	Deepsleep_count = 0;    // Reset deepsleep counter
	ADCSRA = prevADCSRA;    // Re-enable ADC
    delay(500);
    println("Wakeup");
    delay(2000);
    setup();                // Upon wakeup
}
#endif


// Hardware reset TODO
void Reset_board_after_15min() {
    println( __func__);
    #if ATtiny84_ON
        unsigned long Timeout = 1000UL*60*15;
        wdt_disable();
        delay(Timeout);
        wdt_enable(WDTO_15MS);
        delay(1000); // Wait for watchdog to reset board
    #endif
}


// Grab available message, No ID checking needed
bool Get_available_message() {
    unsigned long Msg_timer = millis() + 1000 * 5;
    uint16_t Message;
    uint16_t CRC_check;
    uint16_t CRC_sum = 0;
    
    // Grab 6 packages in rapid succession
    for (uint8_t i=0; i<6; i++) {
        if (radio.available()) {     
            radio.read(&Message, sizeof(Message));
            Message_package[i] = Message;
            CRC_check = (Message >> 12) & 0x0F;
            CRC_sum += CRC_check;              
        }
        // Wait for next Msg. Break if time is up
        if (i < 5) {                                        
            while (!radio.available()) {
                if (millis() > Msg_timer) {
                    return false;
                }
            }  
        }
        // "Checksum" check  
        else if (CRC_sum != 15) {
            return false;
        }
    }
    return true;
}


// Wait for message 
bool Wait_for_message(uint16_t Offset) {
    unsigned long Msg_wait_timer = (millis() + Offset);
    uint8_t Pipe;

    // Start listening
    while (true) {
        if (radio.available(&Pipe) && Pipe == This_dev_address) {
            return Get_available_message();
        }
        // Break after millis ms
        if (millis() > Msg_wait_timer) {
            return false;
        }
    }
}


// Send Bat status get currect time
void Send_ADC_get_NTP() {
    println(__func__);
    WDT_RESET();

    // Get NTP time from master or hard reset after 15min
    if (!Try_send_message()) {
        Reset_board_after_15min();
        }
    // Start listening
    radio.flush_rx();
    radio.flush_tx();
    radio.startListening();
    delay(5);
  
    // Wait for return message
    if (Wait_for_message(2000)) {
        println("Return message successful!!");

        // Message to variables
        if ((Message_package[0] == This_dev_address) && (Message_package[1] == Master_node_address)) {
            //setTime(Message_package[5]); //TODO
            println("Current time acquired!");
            print("Time:    "); println(Message_package[5]);
            radio.flush_rx();
            radio.flush_tx();
            delay(5); 
        } 
    } 
    else {
        Reset_board_after_15min(); 
    }
}


// ADC CAL
#if ADC_CAL_ON
    void ADC_CAL_FUNC() {
            float Sum = 0;
            for (uint8_t i = 0; i < 100; i++) {
                int Value = analogRead(ADC_PIN); 
                Sum += Value;
                delay(10);
            }
            float Average = Sum / 100; 
            print("ADC: ");println(Average);
            Average = 0;
        }
#endif


}
