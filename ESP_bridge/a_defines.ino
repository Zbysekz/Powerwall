
AsyncWebServer server(80);

//wifi
bool connectedToWifi = false;

// the IP address for the shield:
IPAddress ip(192, 168, 0, 12);

IPAddress gateway(192, 168, 0, 1);
IPAddress subnet(255, 255, 255, 0);
IPAddress ipServer(192, 168, 0, 3);  

WiFiClient wifiClient;

#define BUFFER_SIZE 50
#define QUEUE_SIZE 20


long unsigned int tmrSendDataToServer, tmrCheckForData, tmrTimeoutReceive;

uint8_t read_buffer[BUFFER_SIZE];

uint8_t tx_queue[QUEUE_SIZE][BUFFER_SIZE];
uint8_t tx_queue_sizes[QUEUE_SIZE];
uint8_t queue_ptr_actual,queue_ptr_ready;

uint8_t rx, expected_len, buffer_ptr, receive_state;
bool minutes_latch;
SoftwareSerial bridgeSerial(D2, D3); // RX, TX

// comm statistics
uint32_t serial_ok_packets, serial_invalid_packets1, serial_invalid_packets2, serial_invalid_packets3;
uint32_t tcp_ok_packets, tcp_conn_failed,queue_full_packets;
