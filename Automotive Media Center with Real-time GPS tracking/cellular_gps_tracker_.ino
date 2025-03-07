/*
 *  Description: This skecth gets the GPS coordinates and sends it 
 *  through HTTP if your password is correct. 
 * 
 *
 *  This program is free software: you can redistribute it and/or modify 
 *  it under the terms of the GNU General Public License as published by 
 *  the Free Software Foundation, either version 3 of the License, or 
 *  (at your option) any later version. 
 *  
 *  This program is distributed in the hope that it will be useful, 
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of 
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the 
 *  GNU General Public License for more details. 
 *  
 *  You should have received a copy of the GNU General Public License 
 *  along with this program.  If not, see . 
 *
 *  Version 1.1
 *  Author: Clint Bhola
 */
 
#include <PString.h>
#include <SoftwareSerial.h>
#include <TinyGPS.h>
#include <string.h>
#include <SerialGSM.h>

/*****************************************
 * Error codes
 */
const int ERROR_GPS_UNAVAIL  = 0;
const int ERROR_GPS_STALE    = 1;
const int ERROR_SIM_UNAVAIL  = 2;
const int ERROR_GPRS_FAIL    = 3;
const int ERROR_NETWORK_FAIL = 4;
const int ERROR_HOST         = 5;
const int ERROR_GPRS_UNKNOWN = 6;
const int ERROR_GSM_FAIL     = 7;
const boolean DEBUG          = true;

/******************************************
 * Pin Definitions
 */

// UART MODE
int GPS_TX = 0;
int GPS_RX = 1;

// DLINE mode
//int GPS_TX = 2;
//int GPS_RX = 3;

int GPRS_TX = 2;
int GPRS_RX = 3;

int LED_STATUS = 13;
int LED_ERROR = 12;

int GPS_RELAY = 9;

/*****************************************
 * Buffers
 */
const int BUFFSIZE = 180;
char at_buffer[BUFFSIZE];
//char buffidx;
char incoming_char = 0;
char buffer[BUFFSIZE];
char inchar;

/*****************************************
 * Registers
 */
boolean firstTimeInLoop = true;
boolean GPRS_registered = false;
boolean GPRS_AT_ready   = false;
boolean continueLoop    = true;

/*****************************************
 * Misc variables
 */
 
const int PRECISION  = 11; // Precision of lat/long coordinates
const int DIAG_DELAY = 2000; // time to pause before replaying diagnostic codes
const int SEND_DELAY = 10000; // The time to wait before sending the next coordinates

TinyGPS gps;
SoftwareSerial cell(GPRS_TX, GPRS_RX);
PString myStr(buffer, sizeof(buffer));
SerialGSM sms(2,3);

void setup() {
  Serial.println("Arduino powered GPS Tracker initializing");
    
  pinMode(LED_STATUS, OUTPUT);
  pinMode(LED_ERROR, OUTPUT);
  pinMode(GPS_RELAY, OUTPUT);
  
  delay(30000);
  Serial.begin(9600);
  cell.begin(9600);
    
  // Check LEDS
  blinkLed(LED_STATUS, 5, 100);
  blinkLed(LED_ERROR, 5, 100);
  
  cell.println("AT+CMGF=1"); // set SMS mode to text
  delay(200);
  cell.println("AT+CNMI=3,3,0,0"); // set module to send SMS data to serial out upon receipt 
  delay(200);
  
}

/*********************************************************
 * GPRS Functions
 */
 
/**
 * store the serial string in a buffer until we receive a newline
 */
static void readATString(boolean watchTimeout = false) {
  char c;
  char buffidx = 0;
  int time = millis();
    
  while (1) {
    int newTime = millis();

    if (watchTimeout) {
      // Time out if we never receive a response    
      if (newTime - time > 30000) error(ERROR_GSM_FAIL);
    }
    
    if (cell.available() > 0) {
      c = cell.read();
      if (c == -1) {
        at_buffer[buffidx] = '\0';
        return;
      }
      
      if (c == '\n') {
        continue;
      }
            
      if ((buffidx == BUFFSIZE - 1) || c == '\r') {
        at_buffer[buffidx] = '\0';
        return;
      }
      
      at_buffer[buffidx++] = c;
    } 
  }
}

/**
 * Send an AT command to the cell interface
 */
static void sendATCommand(const char* ATCom) {
  cell.println(ATCom);
  Serial.print("COMMAND: ");
  Serial.println(ATCom);
  
  while (continueLoop) {
    readATString();
    ProcessATString();
    Serial.print("RESPONSE: ");
    Serial.println(at_buffer);
  }
  
  continueLoop = true;
  delay(500);
}

/*
 * Handle response codes
 */
static void ProcessATString() {
  
  if (DEBUG) {
    Serial.println(at_buffer);
  }
  
  if (strstr(at_buffer, "+SIND: 1" ) != 0) {
    firstTimeInLoop = true;
    GPRS_registered = false;
    GPRS_AT_ready = false;
    
    // GPS unit pulls enough current to keep the GSM shield from starting
    // So we wait until it's initialized and then power it via a digital pin
    digitalWrite(GPS_RELAY, HIGH);
  }
  
  if (strstr(at_buffer, "+SIND: 10,\"SM\",0,\"FD\",0,\"LD\",0,\"MC\",0,\"RC\",0,\"ME\",0") != 0 
    || strstr(at_buffer, "+SIND: 0") != 0) {
    error(ERROR_SIM_UNAVAIL);
  }
  
  if (strstr(at_buffer, "+SIND: 10,\"SM\",1,\"FD\",1,\"LD\",1,\"MC\",1,\"RC\",1,\"ME\",1") != 0) {
    Serial.println("SIM card Detected");
    successLED();
  }
  
  if (strstr(at_buffer, "+SIND: 7") != 0) {
    error(ERROR_NETWORK_FAIL);  
  }
  
  if (strstr(at_buffer, "+SIND: 8") != 0) {
    GPRS_registered = false;
    error(ERROR_GPRS_FAIL);
  }
  
  if (strstr(at_buffer, "+SIND: 11") != 0) {
    GPRS_registered = true;
    Serial.println("GPRS Registered");
    successLED();
  }
  
  if (strstr(at_buffer, "+SIND: 4") != 0) {
     GPRS_AT_ready = true;
     Serial.println("GPRS AT Ready");
     successLED();
  }
  
  if (strstr(at_buffer, "+SOCKSTATUS:  1,0") != 0) {
     error(ERROR_HOST);
  }
  
  if (strstr(at_buffer, "+CME ERROR: 29") != 0) {
    continueLoop = false;
    return;
  }
  
  if (strstr(at_buffer, "+CME ERROR") != 0) {
    error(ERROR_GPRS_UNKNOWN);
  }
  
  if (strstr(at_buffer, "OK") != 0 || strstr(at_buffer, "NO CARRIER") != 0) {
    continueLoop = false;
    successLED();
  }
  
  if (strstr(at_buffer, "+SOCKSTATUS:  1,1") != 0) {
    continueLoop = false;
  }
}

static void establishNetwork() {
  while (GPRS_registered == false || GPRS_AT_ready == false) {
    readATString(true);
    ProcessATString();
  }
}

static void sendData(const char* data) {
  digitalWrite(LED_STATUS, HIGH);

  Serial.println("Setting up PDP Context");
  sendATCommand("AT+CGDCONT=1,\"IP\",\"att.mvno\"");
    
  Serial.println("Configure APN");
  sendATCommand("AT+CGPCO=0,\"\",\"\", 1");
  
  Serial.println("Activate PDP Context");
  sendATCommand("AT+CGACT=1,1");
  
  // Change 0.0.0.0 to reflect the server you want to connect to
  Serial.println("Configuring TCP connection to server");
  sendATCommand("AT+SDATACONF=1,\"TCP\",\"54.84.1.153\",81");
    
  Serial.println("Starting TCP Connection");
  sendATCommand("AT+SDATASTART=1,1");
  
  delay(1000);
  
  //Serial.println("Getting status");
  //sendATCommand("AT+SDATASTATUS=1");
  
  Serial.println("Sending data");
  sendATCommand(data);
  
  //Serial.println("Getting status");
  //sendATCommand("AT+SDATASTATUS=1");
  
  Serial.println("Close connection");
  sendATCommand("AT+SDATASTART=1,0");
  
  Serial.println("Disable PDP Context");
  sendATCommand("AT+CGACT=0,1");
  
  // Clear string and flash LED
  myStr.begin();
  successLED();
  
  digitalWrite(LED_STATUS, LOW);
}

/*********************************************************
 * GPS Functions
 */
 
static void pollGPS(TinyGPS &gps) {
      unsigned long age;
      float lat, lng, speed;
      
      // Acquire data from gps
      gps.f_get_position(&lat, &lng, &age);
      speed = gps.f_speed_mph();
      
      if (lat == TinyGPS::GPS_INVALID_F_ANGLE) {
        error(ERROR_GPS_UNAVAIL);
      }
      
      /*
       * create string of "speed,latitude,longitude" to send to server
       */
      myStr.print("AT+SSTRSEND=1,\"");
      myStr.print(speed);
      myStr.print(",");
      myStr.print(lat, DEC);
      myStr.print(",");
      myStr.print(lng, DEC);
      myStr.print(",");
      myStr.print(TinyGPS::cardinal(gps.f_course()));
      myStr.print("\"");
      
}

static bool gpsAvailable() {
  while (Serial.available())
  {
    if (gps.encode(Serial.read()))
      return true;
  }
  
  return false;
}


/*********************************************************
 * Misc functions
 */
 
/**
 * Flash an LED X amount of times and write to console based on the error code given
 */
static void error(const int errorCode) {
  int flashTimes = 0;
  int i = 0;
  boolean severity = true;
  
  switch(errorCode) {
    case ERROR_GPS_UNAVAIL:
      Serial.println("ERROR: GPS Unavailable");
      // This error is not severe enough to break the main loop
      severity = false;
      break;
    case ERROR_SIM_UNAVAIL:
      flashTimes = 3;
      Serial.println("ERROR: SIM Unavailable");
      break;
    case ERROR_GPRS_FAIL:
      flashTimes = 4;
      Serial.println("ERROR: Connection to network failed");
      break;
    case ERROR_NETWORK_FAIL:
      flashTimes = 5;
      Serial.println("ERROR: Could not connect to network");
      break;
    case ERROR_HOST:
      flashTimes = 6;
      Serial.println("ERROR: Could not connect to host");
      break;
    case ERROR_GPRS_UNKNOWN:
      flashTimes = 7;
      Serial.println("ERROR: Unknown");
      break;
    case ERROR_GSM_FAIL:
      flashTimes = 8;
      Serial.println("ERROR: GSM Timeout");
      break;  
  }
  
  digitalWrite(LED_STATUS, LOW);

  // Setup an infinite loop to require reset if something goes wrong
  while (severity) {
    blinkLed(LED_ERROR, flashTimes, 500);
    delay(1000);
  }
}

/** 
 * Blink a few times so we know all is right with the world
 */
static void successLED() {
  blinkLed(LED_STATUS, 5, 100);
}

/**
 * Blink LED
 */
static void blinkLed(int lPin, int flashTimes, int dly) {
  for (int i = 0; i < flashTimes; i++) {
    digitalWrite(lPin, HIGH); 
    delay(dly);
    digitalWrite(lPin, LOW);
    delay(dly);
  } 
  delay(DIAG_DELAY);
}

void sendSMS(){
  sms.Verbose(true); // used for debugging
  sms.Boot(); 
  sms.FwdSMS2Serial();
  sms.Rcpt("xxxxxxxxx"); // replace xxxxxxxxx with the recipient's cellular number
  sms.Message("test");
  sms.SendSMS();
}

void sendData(){
  if (firstTimeInLoop) {
  firstTimeInLoop = false;
  Serial.println("Establishing Connection"); 
  //establishNetwork();
  }
  
  bool newdata = false;
  unsigned long start = millis();
  
  while (millis() - start < 1000)	  	
  {	  	
    if (gpsAvailable())	  	
      newdata = true;
  }

  // Retrieve GPS Data
  pollGPS(gps);
 
  // Send data to cell network
  sendData(myStr);
 
 //send data top preset phone #
 //sendSMS();

  delay(SEND_DELAY);
}

/*********************************************************
 * Main Loop
 */
void loop() {
  //If a character comes in from the cellular module...
  if(cell.available() >0)
  {
    inchar=cell.read(); 
    if (inchar=='#')
    {
      delay(10);
      inchar=cell.read(); 
      if (inchar=='w')
      {
        delay(10);
        inchar=cell.read();
        if (inchar=='h')
        {
          delay(10);
          inchar=cell.read();
        if (inchar=='e')
        {
          delay(10);
          inchar=cell.read(); 
        if (inchar=='r')
        {
          delay(10);
          inchar=cell.read();
          if (inchar=='e')
          {
            delay(10);
            inchar=cell.read();
          if (inchar=='r')
          {
          delay(10);
          inchar=cell.read(); 
          if (inchar=='u')
          {
            delay(10);
            sendData();
          }
          cell.println("AT+CMGD=1,4"); // delete all SMS
                }
              }
            }
          }
        }
      }
    }
  }
}



