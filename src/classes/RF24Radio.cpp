

// RF24Radio.cpp
#include "classes/RF24Radio.h"
#include "classes/MQTT.h"
#include "utility.h"
extern MQTT mqtt;


// ##################  RF24Radio  ##################

// Begin
bool RF24Radio::begin_object() {
    
    // Initialize 
    if (!(begin(&spi_bus_1, SPI_1_CE, SPI_1_CSN) && isChipConnected())) {
        return false; 
    } 
    // Config
    setPALevel(RF24_OUTPUT, 1);                             // Transmitter output strength   
    setChannel(RF24_CHANNEL);                               // Radio channel
    setDataRate(RF24_DATARATE);                             // Datarate: RF24_2MBPS, RF24_1MBPS, RF24_250KBPS
    openReadingPipe(0, address[RF24_THIS_DEV_ADDRESS]);     // What radio pipe to listen on
    #if SERIAL_ENABLED
        printDetails();
    #endif
    startListening();

    return true;
}


// Send message 
bool RF24Radio::send_message(const uint16_t* local_RF24_package_ptr) {
    for (uint8_t i = 0; i < 6; i++) {

        // Insert Msg ID (0-5) in the upper 4 bits
        uint16_t payload = (i << 12) | local_RF24_package_ptr[i];

        // Send message(s)
        if (!write(&payload, sizeof(payload))) { 

            // Retry loop foreach index
            for (uint8_t j = 0; j < 3; j++) {                       
                delay(25);
                
                // Exit retry loop if successful
                if (write(&payload, sizeof(payload))) {     
                    break;                                      
                }
                // Failed
                else if (j == 2) {
                    return false;
                }  
            }
        }
        delay(15); // Important! Takes about 8ms for Attinys to read content of radio buffer  
    }
    return true;
}


// Try to send message (parameter hell)
bool RF24Radio::try_send_message(uint16_t to_who, uint16_t data_0, uint16_t data_1, uint16_t state, uint16_t time) {
    PRINT_FUNC(0);
    print(" to node [%i] - ", to_who);
    stopListening();
    openWritingPipe(address[to_who]);

    // Create local array  
    uint16_t local_RF24_package[6] = {}; 
    
    // Attiny84s have 2 bytes for Tx and Rx buffers...
    // splitting up package into small chunks of data                                
    local_RF24_package[0] = RF24_THIS_DEV_ADDRESS;      // From who
    local_RF24_package[1] = to_who;                     // To who
    local_RF24_package[2] = data_0;                     // Data
    local_RF24_package[3] = data_1;                     // Data
    local_RF24_package[4] = state;                      // Bool
    local_RF24_package[5] = time;                       // Time

    for (uint8_t i = 0; i < 10; i++) {
        if (send_message(local_RF24_package)) {
            print("Message successful! \n");
            startListening(); 
            return true;
        } 
        delay(175); // Eventually falls inside a nodes "listening window"
    }
    print("Message failed! \n");
    startListening();
    return false;
} 


// Respond to radio 24 Node 
void RF24Radio::respond_to_node(uint8_t RF24_node) {
    PRINT_FUNC(0);
    print(" [%i] \n", RF24_node); 
    uint16_t current_time;

    // Convert current time into hhmm format
    #if !FAKE_TIME_ENABLED
        if (year() > 2000) {
            current_time = (hour() * 100 + minute());
        } else { 
            current_time = 1200;
        }
    #else
        current_time = 1200; 
    #endif

    // Respond to Node with current time
    delay(10);
    try_send_message(RF24_node, 0, 0, 0, current_time);
}


// Sort incoming messages into separate arrays, if multiple senders
bool RF24Radio::sort_incoming_message(DataPackage& pkg) {
    bool is_new_sender = true;

    // Have we seen this node ID before? if yes sort to which array?
    for (uint8_t i = 0; i < pkg.num_of_nodes; i++) {
        if (pkg.new_ID == pkg.already_seen_node_ID[i]) {
            pkg.row_in_array = i;
            is_new_sender = false;
            break;
        }
    }
    // Start of message package (1/6) for each new sending node, Save ID
    if (is_new_sender) {
        if (pkg.new_ID == pkg.new_message) { 
            pkg.already_seen_node_ID[pkg.num_of_nodes] = pkg.new_ID;
            pkg.row_in_array = pkg.num_of_nodes;
            pkg.num_of_nodes += 1;
        } else {
            return false; // Error
        }
    }
    // Store the incoming message in the appropriate array slots
    uint8_t store_at_column = pkg.num_of_packages[pkg.row_in_array];
    pkg.message_container[pkg.row_in_array][store_at_column] = pkg.new_message;
    pkg.num_of_packages[pkg.row_in_array] += 1;

    return true;  
}


// Grab available RF24 messages
bool RF24Radio::get_available_message(DataPackage& pkg) {
    PRINT_FUNC();
    uint32_t countdown_timer = millis() + 500UL;
    uint16_t payload;
    uint8_t counter = 0;
    bool break_loop = false;
    bool is_error = false;
    
    // Grab incoming message. Sort messages into separate arrays, if multiple senders
    while (!break_loop) {
        read(&payload, sizeof(payload));
        pkg.new_message = payload & 0x0FFF;     // Decode only message
        pkg.new_ID = (payload >> 12) & 0x0F;    // Decode only ID

        if (sort_incoming_message(pkg)) { countdown_timer += 300UL; }

        // All packages received? (6/6)
        for (uint8_t i = 0; i < pkg.num_of_nodes; i++) {
            if (pkg.num_of_packages[i] == 6) { 
                counter++;
            }  
        }
        // Break or reset counter
        if (counter == pkg.num_of_nodes) { break; }
        counter = 0;

        // Wait for next message. Break if time is up
        while (!break_loop) {
            if (available()) {
                break;
            } else if (millis() > countdown_timer) {                                  
                print("Error! Times up \n");
                stopListening();
                break_loop = true;
                is_error = true;
                break;
            }       
        }
    }
    // Error? Check for incomplete message packages
    if (is_error) {
        for (uint8_t i = 0; i < pkg.num_of_nodes; i++) {
            if (pkg.num_of_packages[i] != 6) {
                m_error_at_node_ID[i] = pkg.already_seen_node_ID[i];
            }  
        }
        return true; 
    }
    return false; // No error
} 


// Handle incoming radio 24 messages
bool RF24Radio::handle_incoming_radio() {
    PRINT_FUNC(1,1);
    bool is_error = false;

    // Initialize the underlaying "data structure" to be passed around as function args
    DataPackage pkg;

    // Grab messages
    is_error = get_available_message(pkg);
    PRINT_PACKAGE(pkg, is_error);

    // Repeat for each sending node, exclude node(s) with incomplete packages 
    for (uint8_t i = 0; i < pkg.num_of_nodes; i++) {
        if (pkg.num_of_packages[i] != 6) { continue; }
            
        // Easier reading
        uint16_t from_who = pkg.message_container[i][0];
        uint16_t to_who = pkg.message_container[i][1];
        uint16_t request_time = pkg.message_container[i][5];

        // Node update or time request
        if (to_who == RF24_THIS_DEV_ADDRESS && !request_time) {
            print("Node status update! \n");
        } else {
            print("Node requests the current time! \n");
            respond_to_node(from_who);
        }
        // Send node data to InfluxDB
        mqtt.send_RF24_data_to_db(i, pkg);
    }
    return is_error;
}


// Wait for incomming message
bool RF24Radio::wait_for_message(uint16_t ms) {
    PRINT_FUNC();
    uint32_t timer = millis() + ms;
    bool has_happened = false;

    while (millis() < timer) {
        if (available()) {
            handle_incoming_radio();
            has_happened = true;
            break;     
        }
    }
    return has_happened;
}


// Error! Sending retry msg to each node that failed transmission (if any)
void RF24Radio::handle_receive_error() {
    PRINT_FUNC();
    uint8_t error_ID[3] = {};
    uint8_t failed_nodes = 0;
    
    // Save failed node ID(s) before iteration (in wait_for_message)
    for (uint8_t i = 0; i < 3; i++) {
        if (m_error_at_node_ID[i] != 0) {
            error_ID[failed_nodes] = m_error_at_node_ID[i];
            failed_nodes++;
        }
        m_error_at_node_ID[i] = 0;
    }
    // Failed node(s) could still be trying to reach
    bool got_response = wait_for_message(3000UL);

    // No response yet? Send "Retry" message to each failed node 
    if (!got_response) {
        for (uint8_t i = 0; i < failed_nodes; i++) {
            try_send_message(error_ID[i], 0, 0, 0, 3333);
            wait_for_message(3000UL);
        }
    }
    // Happy nodes now?
    for (uint8_t i = 0; i < 3; i++) {
        if (error_ID[i] != 0 && error_ID[i] == m_error_at_node_ID[i]) {
            print("Extremely unlikely, are we here? Node [%i]\n", m_error_at_node_ID[i]);
        } else {
            print("Receive errors resolved! \n");
            break;
        }
    }       
}


