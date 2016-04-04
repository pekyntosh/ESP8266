/* 
 * see also
 * https://github.com/pulkin/esp8266-injection-example/issues/1
 * https://gist.github.com/mortenjust/148b2e3403809e5f12ac
 * http://www.sharetechnote.com/html/WLAN_FrameStructure.html#Beacon_Frame
 */

#include <ESP8266WiFi.h>

extern "C" {
  #include "user_interface.h"
}

const byte SSIDS_SIZE = 5;
// max length 32 octect
String SSIDs[SSIDS_SIZE] = {
                              "Spaghetti e mandolino",
                              "Free WiFi come in",
                              "TestWiFi",
                              "Ciao, sono purpetta",
                              "SPAM*SPAM*SPAM"
                          };

const byte CHANNEL_MIN = 1;
const byte CHANNEL_MAX = 11;

const byte FRAME_SIZE = 128;
uint8_t frame[FRAME_SIZE];

byte SSIDs_index = 0;
byte channel;

const byte FRAME_HEAD_SIZE = 37;
const byte FRAME_FOOT_SIZE = 13;

// uint8_t (unsigned integer of length 8 bits)
// casting to/from uint8_t to/from char will always work when char is used for storing ASCII characters, since there are no symbol tables with negative indices.
const uint8_t frame_head[FRAME_HEAD_SIZE] = { 
  
                  // FRAME HEADER
                  
                  // 0-1 (2): frame control (non capisco bene perchè "0x80, 0x00"? A mio parere dovrebbe essere "0x00, 0x80")
                        0x80, 0x00,
                  // 2-3 (2): duration (time in microseconds)
                        0x00, 0x00,
                  // 4-9 (6): RA (receiver address / destination address), layer-2 broadcast
                        0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
                        
                  // 10-15 (6): source address
                        0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                  // 16-21 (6): BSSID
                        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
                        
                  // 22-23 (2): sequence control (4 bit: fragment number; 12 bit: sequence number, al suo incremento ci pensa wifi_send_pkt_freedom())
                        0xc0, 0x6c,

                  // FRAME BODY
                  
                  // 24-31 (8 byte): timestamp
                        0x83, 0x51, 0xf7, 0x8f, 0x0f, 0x00, 0x00, 0x00,
                  // 32-33 (2 byte): beacon interval
                        0x64, 0x00, 
                  // 34-35 (2 byte): capability info
                        0x01, 0x04,

                  // 36
                        0x00 };
 
/*
                  // 37 (1 byte): SSID length                        
                        0x06,
                  // 38-43: SSID string (max 32 octect)
                        0x72, 0x72, 0x72, 0x72, 0x72, 0x72 };
*/

const uint8_t frame_foot[FRAME_FOOT_SIZE] = { 

                  // 44-45
                  // tag number: supported rates (1); tag length (8) (ovvero sono indicati 8 parametri come supported rates)
                        0x01, 0x08,
                  // 46-53
                  // supported rates (1, 2, 5.5, 11, 18, 24, 36, 54) (occhio, non corrispondono a valori decimali)
                        0x82, 0x84, 0x8b, 0x96, 0x24, 0x30, 0x48, 0x6c,

                  // 54 (1 byte); tag number: DS parameter set (3)
                        0x03,
                  // 55 (1 byte): tag length (1)
                        0x01,
                  // 56: channel
                        0x04
                    };

void setup() {
  delay(500);

// https://github.com/esp8266/Arduino/blob/master/doc/libraries.md#wifiesp8266wifi-library
  wifi_set_opmode(STATION_MODE);
  wifi_promiscuous_enable(1);

  Serial.begin(115200);
  Serial.println("setup()");
}

int frame_composer() {
  int k, j;
  
// inizializzo il frame
  for (k = 0; k < FRAME_SIZE; k++) {
    frame[k] = 0;
  }

// copio la prima parte del prototipo del frame
  for (k = 0; k < FRAME_HEAD_SIZE; k++) {
    frame[k] = frame_head[k];
  }

/*
 * Frames are created by the server, so the server’s MAC address is the source address for frames. 
 * When frames are relayed through the access point, the access point uses its wireless interface as the transmitter address.
 * As in the previous case, the access point’s interface address is also the BSSID. Frames are ultimately sent to the client, which is both the destination and receiver.
 */
// randomize src mac address
  frame[10] = frame[16] = random(256);
  frame[11] = frame[17] = random(256);
  frame[12] = frame[18] = random(256);
  frame[13] = frame[19] = random(256);
  frame[14] = frame[20] = random(256);
  frame[15] = frame[21] = random(256);

// estrae l'SSID dal vettore SSIDs
  String SSID = SSIDs[SSIDs_index];
// length() restituisce un unsigned int
  byte SSID_length = SSID.length();
// imposta la lunghezza della stringa all'interno del frame, offset 37
  frame[37] = (uint8_t) SSID_length;

  Serial.println("\n\n\npublishing SSID: " + SSID + " (length: " + SSID_length + ")");
  
// estrae ciascun carattere dall'SSID e lo imposta nel frame vector in forma incrementale a partire dall'indice 38
  for (k = 38, j = 0; j < SSID_length; k++, j++) {
    frame[k] = (uint8_t) SSID.charAt(j);
    
//    Serial.println((String) ((uint8_t) SSID.charAt(j)) + " in posizione " + (String) + k);
  }

// dall'indice k + 1 in poi posso assemblare frame con frame_foot
  for (j = 0; j < FRAME_FOOT_SIZE; k++, j++) {
    frame[k] = (uint8_t) frame_foot[j];

//    Serial.println((String) frame_foot[j] + " in posizione " + (String) + k);
  }

//  Serial.println("\nframe length: " + (String) k + "\n");

  k--;
  frame[k] = (uint8_t) channel;
  k++;

//  print_frame(k);

// frame length
  return k;
}

void print_frame(int frame_length) {
  int k;
  
//  byte frame_length = sizeof(frame) / sizeof(uint8_t);  
  for (k = 0; k < frame_length; k++) {
    if (k % 10 == 0) { Serial.println(""); }
    Serial.print((String) frame[k] + ", ");
  }
}

void channel_gen() {
//  Serial.println("channel_gen()");
  
// remember: processing random() != ANSI c random()
// generate random channel (sets global variable)
  channel = random(CHANNEL_MIN, CHANNEL_MAX);
  wifi_set_channel(channel);
  
//  Serial.println("channel: " + (String) channel);
}

void loop() {
//  Serial.println("loop()");

  channel_gen();
  int frame_length = frame_composer();
  
/*
int wifi_send_pkt_freedom(uint8 *buf, int len,bool sys_seq)
Parameter:
uint8 *buf: pointer of frame
int len: frame length
bool sys_seq: follow the system’s 802.11 frames sequence number or not, if it is true, the sequence number will be increased 1 every time a
frame sent.
Return: 0, succeed; -1, fail.
*/
  wifi_send_pkt_freedom(frame, frame_length, 0);
  wifi_send_pkt_freedom(frame, frame_length, 0);
  wifi_send_pkt_freedom(frame, frame_length, 0);
  
  SSIDs_index++;
  if (SSIDs_index == SSIDS_SIZE) {
    SSIDs_index = 0;
  }

  delay(10);
}
