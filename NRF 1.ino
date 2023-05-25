/*

########################### NRF Address 1 ###########################


# Library: TMRh20/RF24, https://github.com/tmrh20/RF24/
# Class config, https://nrf24.github.io/RF24/classRF24.html

######################### ATtiny84 Pinout ###########################
#                             +-\/-+                                #
#                       VCC  1|o   |14 GND                          #
#                       10   2|    |13 AREF A0/0                    #
#                       9    3|    |12 A1/1                         #
#                       RST  4|    |11 A2/2 --- nRF24L01   CE       #
#              PUMP --- PB2  5|    |10 A3/3 --- nRF24L01  CSN       #
#              ADC --- A7/7  6|    |9  A4/4 --- nRF24L01  SCK       #
#    nRF24L01 MISO --- A6/6  7|    |8  A5/5 --- nRF24L01 MOSI       #
#####################################################################
*/


#include <SPI.h>
//#include <nRF24L01.h>
#include <RF24.h>

#define CE_PIN A2
#define CSN_PIN A3
#define PUMP PB2
#define ADC A7

RF24 radio(CE_PIN, CSN_PIN); // Init radio object


// Variables
int                                         
Pump_time = 0;                                      // How long should the pump run?                  

const int 
Read_address = 1,                                   // Which "channel" to listen on       User Controlled
Min_ADC = 0,                                        // Min ADC value                      User Controlled
Max_ADC = 0;                                        // Max ADC value                      User Controlled

float 
Bat_voltage = 0;                                    // 

unsigned long
Timer = 0,                                          // 
Old_millis = 0,                                     // 
Current_millis = 0,                                 // 
ADC_interval = 7200000;                             // How often send ADC data?           User Controlled

//const byte address[6] = "00001";                  // RF24 send/receive address?         User Controlled
uint8_t address[][6] = { "1Node", "2Node" };        // RF24 send/receive address?         User Controlled


// Message data structure 
struct Data_Package {
  int from;
  int Pump_T = 0;           
  int NTP_T = 0;
  bool Pump_on = false; 
};

Data_Package data; // Variable with the above structure



// Setup
void setup() {
  pinMode(PUMP, OUTPUT);
  pinMode(ADC, INPUT);

  // Radio init
  radio.powerUp();
  radio.begin();
  radio.setPALevel(RF24_PA_MIN);            
  radio.setChannel(124);
  radio.setDataRate(RF24_1MBPS);      // RF24_2MBPS, RF24_1MBPS, RF24_250KBPS
  radio.openWritingPipe(address[0]);  // Only change for master
  radio.openReadingPipe(Read_address, address[Read_address]);
  
  // Get NTP time
  Old_millis = millis();
}





// Send message 
bool Send_message(String Command){
  int ADC_value = 0;

  // Get ADC reading, calc average, convert to %
  for (int i = 0; i < 10; i++){
    int Value = analogRead(A7); 
    ADC_value += Value;
  }
  int Avg_ADC_value = ADC_value / 10;  
  Bat_voltage = (Avg_ADC_value - Min_ADC) / (Max_ADC - Min_ADC);  // (x - min) / (max - min) = %    

  // Data package structure
  struct Local_package {
    String info;                          // what kind of message 
    const int from = Read_address;        // Who is this message from               
    float bat_charge = Bat_voltage;       // Battery charge left as %
  };
  // Insert variable
  Local_package Local_data;
  Local_data.info = Command;

  // Send message
  return radio.write( &Local_data, sizeof(Local_package) );      
}



// Try to send message 3 times
void Try_send_message(String Command){
  for (int i = 0; i < 3; i++) {
      if (Send_message(Command)){
        Old_millis = Current_millis; 
        break;
      } 
      delay(150);
  }
}



// Main loop
void loop() {

  // Save dha power, toggle transciever on/off 200ms interval
  for (int i = 0; i <= 1000; i++){
    radio.startListening();
    Current_millis = millis();

    // Wait 200ms for a radio message
    Timer = (Current_millis + 200);
    while (!radio.available()) {
      if (radio.available()) {
        radio.read(&data, sizeof(Data_Package));
  
      }
      // Break after 200ms
      if (millis() > Timer){
        break;
      }
    }
    radio.stopListening();
    delay(200);    
  }

  // Send battery ADC reading every ADC_interval
  if (Current_millis >= Old_millis + ADC_interval){
    Try_send_message("ADC");
  }

  // Time to sleep?
  /*if (NTP_time == night){
    radio sleep
    radio.powerDown();
    delay(10);
    Deepsleep 8h
  }*/
}

//uint8_t pipe;                            // initialize pipe data
//radio.available(&pipe);
