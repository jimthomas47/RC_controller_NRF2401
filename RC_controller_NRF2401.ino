/*
Wireless Controller Transmitter
 hacked cheap USB Dual Vibration PC Computer Wired Gamepad Game Controller Joystick 
 to bypass the usb circuit (leave chip in place to provide pullup's for buttons)
 to access joystick voltages and all button (except select, analog and start)
 Transmitts joystick positions and button levels via NRF24L01+ transciever
 Arduino pro-mini 5V 16Mhz
 A0-A3 inputs read joystick control levels 
 2x 74HC165 parallel to serial used in series to read pushbuttons 
 
 Note: to avoid interference with other RC controller channels
  radio.setChannel(); should be unique and match receiver channel in robot
 
 Jim Thomas 
 St. Andrew's Episcopal School
 Engineering/Robotics  2014
 jthomas@sasaustin.org 
 
 Thanks and (C) to James Coliz, Jr. manicbug@ymail.com for the NRF24L01 and RF24 library
 */

#include <SPI.h>
#include "nRF24L01.h"
#include "RF24.h"
#include "printf.h"

RF24 radio(9,10);
const uint64_t pipe = 0xE8E8F0F0E1LL;

// variables for joystick readings
// these will hold the corrected values -255 to +255 from raw readings
int joy_RUD = 0; // A0 pin value = right stick up/down
int joy_RLR = 0; // A1 pin value = right stick left/rigth
int joy_LUD = 0; // A2 pin value = left stick up/down
int joy_LLR = 0; // A3 pin value = left stick left/right

int a_RUD;
int a_RLR;
int a_LUD;
int a_LLR;

// Centered joystick calibration offset values (not used now)
int RUDCalOffset;
int RLRCalOffset;
int LUDCalOffset;
int LLRCalOffset;

// external 74HC165 shift registers control pin definitions
const int SDATA = 2;   // pin 2 input: serial data out pin from last register
const int SCLOCK = 3;  // pin 3 output: serial clock to registers
const int PLOAD = 4;   // pin 4 otuput: parallel load of shift registers

// shift register read value = state of buttons
unsigned int s_register; // 2 bytes


struct payload_t //   10 byte transmit payload structure
{
  unsigned int shiftreg;  // 2bytes
  int j_RUD;  // 2 bytes
  int j_RLR;  // 2 bytes
  int j_LUD;  // 2 bytes
  int j_LLR;  // 2 bytes
};

void setup() {
  Serial.begin(9600); // debug port
  printf_begin();
  // initialize digitial pins for external shift register
  pinMode(SDATA,INPUT);
  pinMode(SCLOCK,OUTPUT);
  pinMode(PLOAD,OUTPUT);
  digitalWrite(PLOAD,HIGH); 
  digitalWrite(SCLOCK,LOW);
  // read joytick centered values at reset time for calibration of zero point (NOT USED)
  // b/c they are not exactly half of 1023 when at rest
  delay(5);
  RUDCalOffset = (analogRead(A0)-512);
  RLRCalOffset = (analogRead(A1)-512);
  LUDCalOffset = (analogRead(A2)-512);
  LLRCalOffset = (analogRead(A3)-512);
  printf("\n Cal Values: %4i %4i %4i %4i \n",RUDCalOffset,RLRCalOffset,LUDCalOffset,LLRCalOffset);

  radio.begin();
  radio.setChannel(0x42); // change the radio channel from default (0x4C = 76) 
  radio.setDataRate (RF24_250KBPS); // lowest data rate to achieve more range and reliability
  //radio.setPayloadSize(10);
  radio.openWritingPipe(pipe);
  radio.printDetails();

}

void loop() {
  // read the joystick values 0-1023 they are inverted!
  a_RUD = analogRead(A0); 
  a_RLR = analogRead(A1); 
  a_LUD = analogRead(A2); 
  a_LLR = analogRead(A3); 
  // invert and scale to -255 to 256
  joy_RUD = (512-a_RUD)/2;
  joy_RLR = (512-a_RLR)/2;
  joy_LUD = (512-a_LUD)/2;
  joy_LLR = (512-a_LLR)/2;
  // constrain to -255 to 255
  joy_RUD = constrain(joy_RUD,-255,255);
  joy_RLR = constrain(joy_RLR,-255,255);
  joy_LUD = constrain(joy_LUD,-255,255);
  joy_LLR = constrain(joy_LLR,-255,255);

  s_register = sRegisterRead(); // read the button states
  /*
  //debug print 
   Serial.print(s_register,BIN);
   printf(" %4i %4i %4i %4i ",joy_RUD,joy_RLR,joy_LUD,joy_LLR);
   printf("\n");
   */

  // transmit the 10 byte payload
  payload_t payload = {s_register,joy_RUD,joy_RLR,joy_LUD,joy_LLR    };
  bool ok = radio.write (&payload,sizeof(payload));
  if (ok) { 
    //Serial.print(s_register,BIN);
    //printf(" %4i %4i %4i %4i ",joy_RUD,joy_RLR,joy_LUD,joy_LLR);
    //printf("\n");
  } 
  else {
    //printf(" failed\n");
  }
}

//
// sRegisterRead function - reads the external shift register
// returns serial register value (16 bits) when two successive reads are same
// to avoid switch bounce
//
unsigned int sRegisterRead() {
  unsigned int read1 = 0;
  unsigned int read2 = 1;
  while(read1 != read2) {
    digitalWrite(PLOAD,LOW);  // load shift register with switch values
    digitalWrite(PLOAD,HIGH); // parallel load on rising edge
    for (int i = 0; i < 16; i++) {
      // shift in 16 bits to sregister
      read1 = read1<<1 | !digitalRead(SDATA);  // convert active low to high
      digitalWrite(SCLOCK,HIGH); // clocks on rising edge
      digitalWrite(SCLOCK,LOW);
    }
    digitalWrite(PLOAD,LOW);  // load shift register with switch values
    digitalWrite(PLOAD,HIGH); // parallel load on rising edge
    for (int i = 0; i < 16; i++) {
      // shift in 16 bits to sregister
      read2 = read2<<1 | !digitalRead(SDATA);  // convert active low to high
      digitalWrite(SCLOCK,HIGH); // clocks on rising edge
      digitalWrite(SCLOCK,LOW);
    }
  }
  return read2;
}











