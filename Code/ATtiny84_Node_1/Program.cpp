



#include "Program.h"
#include "Config.h"


// Globals and Objects
volatile int Deepsleep_count = 0;
unsigned long ADC_at_this_millis = 0;     
unsigned long Sleep_at_this_millis = 0;   
extern NRF24L01_radio RF24_radio;
#if ATTINY84_ON && SERIAL_ON
    SendOnlySoftwareSerial Tiny_serial(0);
#endif



//##################  NRF24L01  ##################
bool NRF24L01_radio::Begin_object() {
    Debug_println(__func__); 

    // Init
    if (!(begin() && isChipConnected())) {
        Debug_println("RF24 Offline!"); 
        return false; 
    } 
    // Config
    setPALevel(RADIO_OUTPUT);                                         // Transmitter strength   
    setChannel(RADIO_CHANNEL);                                        // Radio channel 
    setDataRate(RF24_2MBPS);                                          // Datarate: RF24_2MBPS, RF24_1MBPS, RF24_250KBPS
    openWritingPipe(address[MASTER_NODE_ADDRESS]);                    // Send to master = 0
    openReadingPipe(THIS_DEV_ADDRESS, address[THIS_DEV_ADDRESS]);     // What pipe to listen on
    startListening();
    println("RF24 OK!");
    return true;
}


// Send message 
bool NRF24L01_radio::Send_message(uint16_t* RF24_package_ptr) {
    Debug_println(__func__); 
    for (uint8_t i=0; i < 6; i++) {

        // Insert Msg ID in the upper most 4 bits
        uint16_t Payload = (THIS_DEV_ADDRESS << 12) | RF24_package_ptr[i];
        
        // Send message(s)
        if (!write(&Payload, sizeof(Payload))) {
            for (uint8_t j = 0; j < 3; j++) {           // Retry loop foreach [i]                 
                delay(10);
                if(write(&Payload, sizeof(Payload))) {  // Exit retry loop if successful
                    break;                                      
                } else if (j == 2) {
                    return false;
                }  
            }
        }
        delay(10);      
    }
    return true;
} 


// Try to send message 
bool NRF24L01_radio::Try_send_message(/*uint16_t arg_to_who, uint16_t arg_int, uint16_t arg_float, uint16_t arg_state, uint16_t arg_time */) {
    Debug_println(__func__); 
    stopListening();
    
    // Create message array 
    uint16_t RF24_package[6]; 
    
    // Attiny84s (Nodes) have 2 bytes for Tx and Rx buffers...
    // splitting up package into small chunks of data                                
    RF24_package[0] = MASTER_NODE_ADDRESS;                  // To who
    RF24_package[1] = THIS_DEV_ADDRESS;                     // From who
    RF24_package[2] = 0;                                    // Node_1 run pump for how long     (Only on RX)
    RF24_package[3] = hardware::Battery_charge_remaining(); // Node_1 Bat charge remaining      (ADC reading 0-1023)
    RF24_package[4] = 0;                                    // Bool on/off                      (Only on RX)
    RF24_package[5] = 0;                                    // Current time                     (Only on RX)

    for (uint8_t i = 0; i < 4; i++) {
        if (Send_message(RF24_package)) {
            Debug_println("Message OK!");
            return true;
        }
        delay(200);
    }
    Debug_println("Message Failed!");
    return false;
}


// Grab available RF24 message
bool NRF24L01_radio::Get_available_message(uint16_t* RF24_package_ptr) {
    unsigned long Timer = millis() + 5 * 1000UL;
    uint8_t CRC_check = 0;
    uint8_t CRC_sum = 0;
    uint16_t Payload;

    // Grab 6 packages in rapid succession
    for (uint8_t i = 0; i < 6; i++) {   
        read(&Payload, sizeof(Payload));
        RF24_package_ptr[i] = Payload & 0x0FFF;     // Save only Msg
        CRC_check = (Payload >> 12) & 0x0F;         // Save only Msg ID
        CRC_sum += CRC_check;
        flush_rx();          

        // Wait for next Msg. Break if time is up
        if (i < 5) {                                        
            while (!available()) {
                if (millis() > Timer) {
                    Debug_println("Error! Times up");
                    return false;
                }
            }
        // Expected "Checksum" ? 
        } else if (CRC_sum != 15) {
            Debug_println("Error! CRC check");
            return false;
        }
    }
    return true;
}


// Wait for incomming RF24 message 
bool NRF24L01_radio::Wait_for_message(uint16_t how_long, uint16_t* RF24_package_ptr) {
    unsigned long Timer = (millis() + how_long);
    uint8_t Pipe;

    // Start listening
    while (true) {
        if (available(&Pipe) && Pipe == THIS_DEV_ADDRESS) {
            return Get_available_message(RF24_package_ptr);
        }
        // Break after how_long milliseconds 
        if (millis() > Timer) {
            return false;
        }
    }
}


// Send battery charge remaining, get back current time
void NRF24L01_radio::Send_ADC_get_time() {
    Debug_println(__func__);
    WDT_RESET(); 
    bool Error_flag = false;
    
    // Ask master node for time 
    Try_send_message();
    startListening();
    uint16_t RF24_package[6];
    uint8_t Loop_counter = 0;

    // Wait for response
    do {
        if (Wait_for_message(10000, RF24_package)) {
            Debug_println("Return message OK!");
            if ((RF24_package[0] == THIS_DEV_ADDRESS) && ((RF24_package[1] == MASTER_NODE_ADDRESS))) {

                // Code from master "Send again"
                if (RF24_package[5] == 3333) {
                    Send_ADC_get_time();
                // Time from master
                } else {
                    hardware::Calc_time_until_sleep(RF24_package);   
                }
                Error_flag = false;
                flush_rx();
                flush_tx();        
            }
        } else { 
            Error_flag = true; 
        }
        Loop_counter++; 
    } while (Error_flag && Loop_counter < 4); // Waiting for master 
    if (Error_flag) {
        hardware::Reset_after_15min();
    }
}



namespace hardware {

// Setup 
void Setup() {

    // Reset Watchdog on boot
    #if ATTINY84_ON
        MCUSR &= ~(1<<WDRF);
        WDT_DISABLE();
    #endif
    
    // Serial
    #if SERIAL_ON
        SERIAL_BEGIN(9600);
        while (!Serial) {} 
        for(uint8_t i = 0; i < 25; i++) {
            println();
        }
    #endif

    // Physical Pins
    pinMode(PUMP_PIN, OUTPUT);
    pinMode(ADC_ENABLE_PIN, OUTPUT);
    pinMode(ADC_MEASURE_PIN, INPUT);
    digitalWrite(ADC_ENABLE_PIN, LOW);
    digitalWrite(PUMP_PIN, LOW);

    // Begin RF24 radio
    RF24_radio.Begin_object();

    // Allow things to settle down, start Watchdog
    delay(3000);                                                            
    WDT_ENABLE(WDTO_8S); 
}


// Used to have <TimeLib.h> for this but ran out of memory :/
void Calc_time_until_sleep(uint16_t* RF24_package_ptr) {
    Debug_println(__func__);
    WDT_RESET(); 

    // Time from master formated "hhmm"     
    uint16_t Current_time = RF24_package_ptr[5];

    // Convert incomming msg to: hours and minutes left until DEEPSLEEP_AT_THIS_TIME
    int16_t Hour_left = (DEEPSLEEP_AT_THIS_TIME / 100) - (Current_time / 100);  
    int16_t Minute_left = (DEEPSLEEP_AT_THIS_TIME % 100) - (Current_time % 100);  

    if (Minute_left < 0) {      // Negativ minute?
        Hour_left--;            // Subtract 1 from Hour_left
        Minute_left += 60;      // Add 60 to Minute_left
    }
    if (Hour_left < 0) {        // Negativ hour?
        Hour_left += 24;        // Add 24
    }

    // Is Current_time greater than DEEPSLEEP_AT_THIS_TIME?
    if (Current_time > DEEPSLEEP_AT_THIS_TIME) {
        Hour_left = 0;
        Minute_left = 0;   
    }
    // Sleep at what time?
    unsigned long Millis_left = (Hour_left * 60UL + Minute_left) * 60UL * 1000UL; // Convert to millis
    Sleep_at_this_millis = Millis_left + millis(); // Add current millis() to get an absolute timestamp
    print("Time set: "); Debug_println(RF24_package_ptr[5]);  
}


// Get ADC reading from battery
uint16_t Battery_charge_remaining() { 
    Debug_println(__func__); 
    WDT_RESET();
    RF24_radio.stopListening();
    RF24_radio.powerDown();
    delay(5);
    
    // Toggle transistor pair to let current flow into voltage divider
    digitalWrite(ADC_ENABLE_PIN, HIGH);

    // Grab battery readings 
    uint32_t Sum = 0;
    for (uint8_t i = 0; i < 100; i++) {
        Sum += analogRead(ADC_MEASURE_PIN);
        delay(10); 
    }
    // Average ADC value (0-1023), all calculations are done on master
    digitalWrite(ADC_ENABLE_PIN, LOW);
    RF24_radio.powerUp();
    delay(10);
    #if ATTINY84_ON
        uint16_t Charge_remaining = Sum / 100; 
        return Charge_remaining;
    #else
        return 737;
    #endif
}


// Message received, start waterpump for n seconds
void Start_water_pump(uint8_t How_long) {
    Debug_println(__func__); 
    WDT_RESET();
    RF24_radio.stopListening();

    // To keep the pump from flooding my apartment...
    if (How_long >= PUMP_RUNTIME_MAX) {
        How_long = 2;
    }
    // Toggle transistor on PUMP_PIN
    WDT_DISABLE(); 
    digitalWrite(PUMP_PIN, HIGH);
    delay(How_long * 1000);
    digitalWrite(PUMP_PIN, LOW);
    WDT_ENABLE(WDTO_8S); 
    delay(5);  
}


// Hardware reset
void Reset_after_15min(uint16_t Seconds) { 
    Debug_println(__func__); 
    #if ATTINY84_ON
        unsigned long Timeout = 1000UL * Seconds; 
        WDT_DISABLE();
        delay(Timeout);
        WDT_ENABLE(WDTO_15MS);
        delay(1000); // Wait for watchdog to reset board
    #endif
}


#if ATTINY84_ON
    // Watchdog ISR 
    ISR (WDT_vect) {
        WDT_DISABLE();
        Deepsleep_count++;
    }


    // ATtiny84 + NRF24 deepsleep (about 5 µA)
    void Deepsleep() {
        Debug_println(__func__); 
        println("Zzz");

        // Turn off everything
        WDT_RESET(); 
        RF24_radio.stopListening();
        RF24_radio.powerDown();
        delay(10);
        static byte Prev_ADCSRA = ADCSRA;
        ADCSRA = 0;
        set_sleep_mode(SLEEP_MODE_PWR_DOWN);
        sleep_enable();

        // Wake by watchdog every 8s, go back to sleep if Deepsleep_count < DEEPSLEEP_HOW_LONG. Each cycle = 9.8s
        while (Deepsleep_count < (DEEPSLEEP_HOW_LONG * 367)) { 
            noInterrupts();
            sleep_bod_disable();

            // Clear various "reset" flags
            MCUSR = 0; 	
            WDTCSR = bit (WDCE) | bit(WDE);                 // WDT mode "interrupt" instead of "reset" 
            WDTCSR = bit (WDIE) | bit(WDP3) | bit(WDP0);    // WDT time interval (max 8s)
            WDT_RESET(); 
            interrupts();
            sleep_cpu();  
        }
        // 12 hours passed
        sleep_disable();                // Sleep disable
        Deepsleep_count = 0;            // Reset deepsleep counter
        ADCSRA = Prev_ADCSRA;           // Re-enable ADC
        delay(500);
        hardware::Reset_after_15min(1);  // millis() doesnt reset to "0" otherwise
    }
#endif

} // namespace hardware



//####################  DEBUG  ####################
#if SERIAL_ON
    #if ADC_CAL_ON
        // ADC CAL DEBUG FUNC
        void ADC_CAL_FUNC() { 
            WDT_DISABLE();
            digitalWrite(ADC_ENABLE_PIN, HIGH);

            while (1) {
                uint32_t Sum = 0;
                for (uint8_t i = 0; i < 100; i++) {
                    uint16_t Value = analogRead(ADC_MEASURE_PIN); 
                    Sum += Value;
                    delay(10); 
                }
                uint16_t ADC_average = Sum / 100;           
                print("Avg: "); println(ADC_average);
            }
        }
    #endif
    #if !ATTINY84_ON
        // Print message package
        void Print_package(uint16_t* RF24_package) {
            println();
            println("#########  Package  #########");
            print("To who:      "); println(RF24_package[0]);
            print("From who:    "); println(RF24_package[1]);
            print("Int:         "); println(RF24_package[2]);
            print("Float:       "); println(RF24_package[3]);
            print("Bool:        "); println(RF24_package[4]);
            print("Time:        "); println(RF24_package[5]);
            println();
        }
    #endif
#endif


