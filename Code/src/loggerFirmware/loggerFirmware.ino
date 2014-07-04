#include <RTClib.h>
#include <SD.h>
#include <Wire.h>

//Logging configuration
#define LOG_INTERVAL 1000 //Milliseconds between log entries

//LED pin definitions
#define greenLEDpin 2
#define yellowLEDpin 5
#define redLEDpin 8

//Instantiate Real Time Clock object
RTC_DS1307 RTC;

//Digital pin 10 used for SD cs line
const int chipSelect = 10;

//RTC Config Filename
const char[] rtcConfigFile = "RTC.cfg";

//Instantiate File object for logfile
File logfile;

//SD Error Function - Lights up Red LED and halts execution
void sdError(char *str){
  //todo: display string
  
  digitalWrite(redLEDpin, HIGH);
  
  while(1);
}

//Wait For SD function - Waits until a working sd card is inserted and initialized
void waitForSD(void){
  while (!SD.begin(chipSelect)){
    digitalWrite(yellowLEDpin, HIGH);
    delay(3000);
    digitalWrite(yellowLEDpin, LOW);
  }
}

//RTC Configuration function - configures RTC according to info found in RTC.cfg
bool rtcConfig(){
  
   return true; 
}
void setup(void){
  //Startup hardware serial
  Serial.begin(38400);
  
  //Set chip select pin to output
  pinMode(10, OUTPUT);
  
  //Attempt to initialize SD card
  waitForSD();
  
  //Check for RTC config file and use it to set the clock if present
  if (SD.exists(rtcConfigFile)){
    rtcConfig();
  }
  
  //Create a new file to log to
  char filename[] = "LOG
  
