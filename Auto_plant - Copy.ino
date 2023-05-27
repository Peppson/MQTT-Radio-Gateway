
/*
########################## NRF Address 1 ##########################
                                                                   
# Library, TMRh20/RF24: https://github.com/tmrh20/RF24/             
# Class config: https://nrf24.github.io/RF24/classRF24.html         
# https://github.com/Peppson/?                                      
                                                                   
########################## ATtiny84 Pins ##########################
#                             +-\/-+                              #
#                       VCC  1|o   |14 GND                        #
#                       10   2|    |13 AREF A0/0                  #
#                       9    3|    |12 A1/1                       #
#                       RST  4|    |11 A2/2 --- nRF24L01   CE     #
#              PUMP --- PB2  5|    |10 A3/3 --- nRF24L01  CSN     #
#              ADC --- A7/7  6|    |9  A4/4 --- nRF24L01  SCK     #
#    nRF24L01 MISO --- A6/6  7|    |8  A5/5 --- nRF24L01 MOSI     #
###################################################################
*/


#include <nRF24L01.h>  
#include <RF24.h>
#include <SPI.h>      

// Comp directives
#define ATtiny84_ON 1       // Toggle between ATtiny84 or Pi Pico
#define SERIAL_PRINT_ON 0   // Toggle Serial.print(x) on/off

// Board specifics
#if ATtiny84_ON 
  #define CE_PIN A2         
  #define CSN_PIN A3
  #define PUMP PB2    
  #define ADC A7
  #define Deepsleep_device() Deepsleep();
  #include <avr/wdt.h>
  #include <avr/sleep.h>  
#else
  #define CE_PIN 6
  #define CSN_PIN 5 
  #define PUMP 15        
  #define ADC 28
  #define Deepsleep_device() Fake_deepsleep();  
#endif

// Serial.print  
#if SERIAL_PRINT_ON
  #define print_init(x) Serial.begin(x);
  #define print(x) Serial.println(x);
#else
  #define print_init(x) 
  #define print(x) 
#endif

RF24 radio(CE_PIN, CSN_PIN);  // Init radio object


// Variables
uint8_t  
Pump_cooldown = 0,                                  // Pump cant start again within 10s (0)                                      
Pump_time,                                          // How long should the pump run? in seconds
NTP_time,                                           // Init    
Sender_address;                                     // Who is sending 
                  
const uint8_t                                           
This_dev_address = 1,                               // Which pipe + device address            User Controlled
Time_for_sleep = 9,                                 // Go to deepsleep at what time? 0-23h    User Controlled         
address[][6] =  
{ "1Node", "2Node", "3Node", "4Node", "5Node" };    // Address array for "This_dev_address"

const uint16_t
Max_ADC_reading = 10,                               // Max ADC value                          User Controlled  
Min_ADC_reading = 1;                                // Min ADC value                          User Controlled
 
unsigned long
Msg_wait_timer,                                     // Init
Prev_millis,                                        // Init
Current_millis;                                     // Init

const unsigned long
ADC_interval = 1000UL*60*60*2;                      // How often send ADC data?               User Controlled

volatile int 
sleepCount = 0;                                     // Counter in deepsleep

struct Data_Package {                                
  uint8_t from;                                     // From who
  uint8_t Pump_T;                                   // How long should the pump run for 
  unsigned long NTP_T;                              // Unix NTP time
  bool Pump_On;                                     // On/off
};
Data_Package data;



// Message received, start waterpump for x seconds
void Start_pump(){
  #if ATtiny84_ON
    wdt_reset();
  #endif
  digitalWrite(PUMP, HIGH);
  if (Pump_time >= 5){
    Pump_time = 2;
  }
  delay(Pump_time * 1000);
  digitalWrite(PUMP, LOW);
  Pump_cooldown = 10;
}


// Watchdog ISR
#if ATtiny84_ON
ISR (WDT_vect) {
    wdt_disable();
	  sleepCount ++;
}
#endif


// ATtiny84 + NRF24 deepsleep
#if ATtiny84_ON
void Deepsleep() {

  // Turn off everything for sleep
  radio.stopListening();
  radio.powerDown();
  delay(50);
  static byte prevADCSRA = ADCSRA;
	ADCSRA = 0;
  set_sleep_mode(SLEEP_MODE_PWR_DOWN);
  sleep_enable();

  // Wake by watchdog every 8s, go back to sleep if sleepcount < 5400. About 12h
  while (sleepCount < 5400) {
    noInterrupts();
		sleep_bod_disable();

		// Clear various "reset" flags
		MCUSR = 0; 	
		WDTCSR = bit (WDCE) | bit(WDE);                 // WDT mode "interrupt" instead of "reset" 
		WDTCSR = bit (WDIE) | bit(WDP3) | bit(WDP0);    // WDT time interval (max 8s)
    wdt_reset(); 
		interrupts();
		sleep_cpu();
    // ZZZZZzzzzzzZZZZzzzzzZZZZZ 
	}
  // 12 hours passed
	sleep_disable();
	sleepCount = 0;     
	ADCSRA = prevADCSRA; // Re-enable ADC
  radio.powerUp();
  delay(100);
  }
#endif


// Pico Fake_deepsleep for testing
#if SERIAL_PRINT_ON
void Fake_deepsleep(){
  print("Fake deepsleep!")
  radio.stopListening();
  radio.powerDown();
  delay(5000);
  print("Aight, back!")
}
#endif


// Send message 
bool Send_message(const String& Message = "ADC"){
  int ADC_value = 0;

  // Get ADC reading from battery, calc average, convert to %
  for (int i = 0; i < 10; i++){
    int Value = analogRead(ADC); 
    ADC_value += Value;
  }
  int Avg_ADC_value = ADC_value / 10;  
  float Bat_percent = (Avg_ADC_value - Min_ADC_reading) / (Max_ADC_reading - Min_ADC_reading);  // (x - min) / (max - min) = %    

  // Data package structure (from this device to master)
  struct Local_package {
    String info;                                // what kind of message 
    const uint8_t from = This_dev_address;      // Who is this message from               
    float bat_charge;                           // Battery charge left as %
  };
  // Insert variables
  Local_package Local_data;
  Local_data.info = Message;
  Local_data.bat_charge = Bat_percent;

  // Send message
  return radio.write( &Local_data, sizeof(Local_package) );      
}


// Try to send message 3 times
bool Try_send_message(const String& Message = "ADC"){
  for (int i = 0; i < 3; i++) {
      if (Send_message(Message)){
        Prev_millis = Current_millis; 
        return true;
      } 
      delay(150);
  }
  return false;
}


// Wait ms for message
bool Wait_for_message(){
  Msg_wait_timer = (Current_millis + 200);
  while (!radio.available(&Sender_address)) {
    if (radio.available() && Sender_address == 0) {
      return true;
    }
    // Break after 200ms
    else if (millis() > Msg_wait_timer){
      return false;
    }
  }
}


// Print received message 
#if SERIAL_PRINT_ON
void Serial_print_alot(){
  print("Message received!");
  print("---------------------");
  print(data.from);
  print(data.Pump_T);
  print(data.NTP_T);
  print(data.Pump_On);
}
#endif


// Hardware reset
void Reset_board_after_15min(String Message) {
  #if ATtiny84_ON
      unsigned long Timeout = 1000UL*60*15;
      wdt_disable();
      delay(Timeout);
      wdt_enable(WDTO_15MS);
      while (1) {}
  #else
    print("Failed! Fake reset!")
    print(Message); // What part failed?
  #endif
}



// Setup
void setup() {

  // Init Serial + Pins
  print_init(9600);           
  pinMode(PUMP, OUTPUT);      
  pinMode(ADC, INPUT);        

  // Radio init
  radio.begin();
  delay(150);
  radio.setPALevel(RF24_PA_MIN);                                          // Transmitter strength   
  radio.setChannel(124);                                                  // Above Wifi 
  radio.setDataRate(RF24_1MBPS);                                          // RF24_2MBPS, RF24_1MBPS, RF24_250KBPS
  radio.openWritingPipe(address[0]);                                      // Only change for master
  radio.openReadingPipe(This_dev_address, address[This_dev_address]);     // What pipe to listen on 

  // Comp directives
  #if SERIAL_PRINT_ON
    delay(1000);
    for (int i=0; i<10; i++) {
      print();
    }
    print("Starting...");
    delay(200);
    if (radio.available()) {
    print("Radio: On");  
  }
  #endif

  // Get NTP time from master or hard reset
  if (!Try_send_message()){
    Reset_board_after_15min("Send");
  }
  // Wait for return message from master
  radio.startListening();
  if (Wait_for_message()){
    print("Wakeup return message available!");
    radio.read(&data, sizeof(Data_Package));    
    // Message to var
    if (data.from == 0){                    // From master node
      Pump_time = data.Pump_T;              // How long the pump should run for in seconds
      NTP_time = (data.NTP_T / 3600) % 24;  // NTP_time in hours 0-23
      Prev_millis = millis();               // Init 
    }
  } else {
    Reset_board_after_15min("Receive");
  }
  // Watchdog init / Print message
  #if ATtiny84_ON
    wdt_enable(WDTO_8S); 
  #else
    print("Return message @ startup:");
    Serial_print_alot();      
  #endif
}


// Main loop
void loop() {

  // Watchdog reset
  #if ATtiny84_ON
    wdt_reset();
  #endif

  // Save dha power, toggle transciever on/off 
  for (int i = 0; i <= 1000; i++){
    Current_millis = millis();
    radio.startListening();
    if (Wait_for_message()){
      radio.read(&data, sizeof(Data_Package));
      #if SERIAL_PRINT_ON
        Serial_print_alot();
      #endif
      if (data.Pump_On && Pump_cooldown == 0) {
        Start_pump();
      }
    } 
    radio.stopListening();
    delay(300);
    // Reset Pump_cooldown
    if (!Pump_cooldown == 0) {
      Pump_cooldown --;
    }    
  }
  // Send battery charge every ADC_interval (2h)
  if (Current_millis >= Prev_millis + ADC_interval){
    Try_send_message();
  }
  // Time to sleep?
  if (NTP_time >= Time_for_sleep){
    Deepsleep_device();
    NTP_time = 0;
    setup();
  }
}


