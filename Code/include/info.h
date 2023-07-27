

/*
######################  LAN8720 Ethernet module - ESP32 R1 R32 Pinout  ######################

    * GPIO02    -   NC (From NC > Osc enable pin on Lan8720 module, ETH_POWER_PIN) 
    * GPIO22    -   TX1                                      
    * GPIO19    -   TX0                                      
    * GPIO21    -   TX_EN                                    
    * GPIO26    -   RX1                                     
    * GPIO25    -   RX0                                      
    * GPIO27    -   CRS                                      
    * GPIO00    -   nINT/REFCLK     (4k7 Pullup > 3.3V)    
    * GPIO23    -   MDC             (ETH_MDC_PIN)                                    
    * GPIO18    -   MDIO            (ETH_MDIO_PIN)                                 
    * 3V3       -   VCC
    * GND       -   GND                                           
    
- ETH_CLOCK_GPIO0_IN   - default: external clock from crystal oscillator
- ETH_CLOCK_GPIO0_OUT  - 50MHz clock from internal APLL output on GPIO00 - possibly an inverter is needed for LAN8720
- ETH_CLOCK_GPIO16_OUT - 50MHz clock from internal APLL output on GPIO16 - possibly an inverter is needed for LAN8720
- ETH_CLOCK_GPIO17_OUT - 50MHz clock from internal APLL inverted output on GPIO17 - tested with LAN8720

- https://github.com/flusflas/esp32-ethernet
- https://github.com/SensorsIot/ESP32-Ethernet/blob/main/EthernetPrototype/EthernetPrototype.ino#L44



########  Battery ADC Calibration  ########

# Node 1
    ADC max = 973!
    ADC min = 766!

# Node n
    ADC max = !
    ADC min = !



########  Radio24 Message Package  ########

0.                 // To who
1.               // From who
2.                    // "Int"
3.                  // "Float"
4.                  // "Bool"
5.                   // Time

RF24_package[5] = 3333 = "Send again"



########  Misc  ########
    
unsubscribe("/hello");

*/


