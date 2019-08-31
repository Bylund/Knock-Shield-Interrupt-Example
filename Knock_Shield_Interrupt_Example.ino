/*
    Example code compatible with the Knock Shield for Arduino to demonstrate interrupts such as tach signals.
    
    Copyright (C) 2018 - 2019 Bylund Automotive AB
    
    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.
    
    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
    
    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
    
    Contact information of author:
    http://www.bylund-automotive.com/
    
    info@bylund-automotive.com

    Version history:
    2019-08-31        v1.0.0        First release to Github.
*/

//Define included headers.
#include <SPI.h>

//Define parameters used.
#define           SPU_SET_PRESCALAR_6MHz         0b01000100    /* 6MHz prescalar with SDO active. */
#define           SPU_SET_CHANNEL_1              0b11100000    /* Setting active channel to 1. */
#define           SPU_SET_BAND_PASS_FREQUENCY    0b00101010    /* Setting band pass frequency to 7.27kHz. */
#define           SPU_SET_PROGRAMMABLE_GAIN      0b10100010    /* Setting programmable gain to 0.381. */
#define           numberofCylinders              4             /* Defines the amount of cylinders for calculating engine speed. */
#define           SPU_SET_MIN_TIME               0b11000000    /* Defines the time constant at high speed to 40 μs.*/
#define           SPU_SET_MAX_TIME               0b11011000    /* Defines the time constant at low speed to 320 μs.*/

//Define pin assignments.
#define           SPU_NSS_PIN                    10            /* Pin used for SPI communication. */
#define           SPU_TEST_PIN                   5             /* Pin used for SPU communication. */
#define           SPU_HOLD_PIN                   4             /* Pin used for defining the knock window. */
#define           TACH_INPUT_PIN                 2             /* Pin used for tach signal from ECU. */
#define           LED_STATUS                     7             /* Pin used for power the status LED, indicating we have power. */
#define           LED_LIMIT                      6             /* Pin used for the limit LED. */
#define           UA_ANALOG_INPUT_PIN            0             /* Analog input for knock. */

//Global Variables.
uint16_t adcValue_UA = 0;
unsigned long previousTime = 0;
unsigned long currentTime;
uint8_t timeConstant = 0;

//Function for transfering SPI data to the SPU.
byte COM_SPI(byte TX_data) {

  //Set chip select pin low, chip not in use.
  digitalWrite(SPU_NSS_PIN, LOW);
  
  //Transmit and receive SPI data.
  byte Response = SPI.transfer(TX_data);

  //Set chip select pin high, chip in use.
  digitalWrite(SPU_NSS_PIN, HIGH);

  //Print SPI response.
  //Serial.print("SPU_SPI: 0x");
  //Serial.print(Response, HEX);
  //Serial.print("\n\r");

  //Return SPI response.
  return Response;
}

//Function to set up device for operation.
void setup() {
  
  //Set up serial communication.
  Serial.begin(9600);

  //Set up SPI.
  SPI.begin();
  SPI.setClockDivider(SPI_CLOCK_DIV16);
  SPI.setBitOrder(MSBFIRST);
  SPI.setDataMode(SPI_MODE1);
  
  //Set up digital output pins.
  pinMode(SPU_NSS_PIN, OUTPUT);  
  pinMode(SPU_TEST_PIN, OUTPUT);  
  pinMode(SPU_HOLD_PIN, OUTPUT);  
  pinMode(TACH_INPUT_PIN, INPUT);  
  pinMode(LED_STATUS, OUTPUT);
  pinMode(LED_LIMIT, OUTPUT);

  //Set initial values.
  digitalWrite(SPU_NSS_PIN, HIGH);
  digitalWrite(SPU_TEST_PIN, HIGH);
  digitalWrite(SPU_HOLD_PIN, LOW);

  //Start of operation. (Flash LED's).
  Serial.print("Device reset.\n\r");
  digitalWrite(LED_STATUS, HIGH);
  digitalWrite(LED_LIMIT, HIGH);
  delay(200);
  digitalWrite(LED_STATUS, LOW);
  digitalWrite(LED_LIMIT, LOW);
  delay(200);

  //Configure SPU (Initialize).  
  COM_SPI(SPU_SET_PRESCALAR_6MHz);
  COM_SPI(SPU_SET_CHANNEL_1);
  COM_SPI(SPU_SET_BAND_PASS_FREQUENCY);
  COM_SPI(SPU_SET_PROGRAMMABLE_GAIN);
  COM_SPI(SPU_SET_MIN_TIME);
  
  //Enable interrupt.
  attachInterrupt(digitalPinToInterrupt(TACH_INPUT_PIN), interrupt, RISING);
}

//Main operation function.
void loop() {
  
    //Set status LED on.
    digitalWrite(LED_STATUS, HIGH);
    delay(20);

    //Set status LED off.
    digitalWrite(LED_STATUS, LOW);
    delay(80);
    
    //Convert ADC-value to percentage of knock level.
    float knock_percentage = ((float)adcValue_UA / 1023) * 100;

    //Set Limit LED if knock percentage reaches 80%.
    if (knock_percentage >= 80) digitalWrite(LED_LIMIT, HIGH); else digitalWrite(LED_LIMIT, LOW);

    //Calculate engine speed.
    float engineSpeed = 60000 / (currentTime * numberofCylinders / 2000);

    //Calculate time constant.
    timeConstant = map(engineSpeed, 1000, 9000, SPU_SET_MAX_TIME, SPU_SET_MIN_TIME);
      
    //Update SPU.
    COM_SPI(SPU_SET_PRESCALAR_6MHz);
    COM_SPI(SPU_SET_CHANNEL_1);
    COM_SPI(SPU_SET_BAND_PASS_FREQUENCY);
    COM_SPI(SPU_SET_PROGRAMMABLE_GAIN);
    COM_SPI(timeConstant);
    
    //Display output.
    Serial.print("SPU KNOCK LEVEL: ");
    Serial.print(knock_percentage, 0);
    Serial.print("%, ENGINE SPEED: ");
    Serial.print(engineSpeed, 0);
    Serial.print(" RPM, KNOCK WINDOW: ");
    Serial.print(currentTime, DEC);
    Serial.print("us, TIME CONSTANT: 0x");
    Serial.print(timeConstant, HEX);
    Serial.print("\n\r");
}

//Interrupt to handle tach signal.
void interrupt() {
      
      //The measurement window ends.
      digitalWrite(SPU_HOLD_PIN, LOW);
      
      //Calculate knock window time.
      currentTime = micros() - previousTime;
      
      //Update time when knock window started.
      previousTime = micros();
      
      //The SPU output voltage is updated.
      adcValue_UA = analogRead(UA_ANALOG_INPUT_PIN);
      
      //The measurement window starts.
      digitalWrite(SPU_HOLD_PIN, HIGH);
   
}
