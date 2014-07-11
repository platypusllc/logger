#include <SD.h>

#include <Wire.h>
#include <RTClib.h>
#include <SPI.h>
#include <MS5541C.h>


//Logging configuration
#define LOG_INTERVAL 1000 //Milliseconds between log entries
#define SYNC_INTERVAL 1000 //Milliseconds between SD card flushes

//LED pin definitions
#define greenLEDpin 2
#define yellowLEDpin 5
#define redLEDpin 8

//Instantiate Pressure Sensor object
MS5541C pressureSensor;

//Set last known temp and pressure
long lastPressure = 0;
float lastTemp = 0.0f;

//Instantiate Real Time Clock object
RTC_DS1307 RTC;

//Digital pin 10 used for SD cs line
const int chipSelect = 10;

//RTC Config Filename
char rtcConfigFile[] = "RTC.cfg";

//Instantiate File object for logfile
File logfile;

//Set last sync time
uint32_t syncTime = 0;

//Instantiate variables used in reading DO probe
String sensorString = "";
bool stringComplete = false;

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
  if (!SD.exists(rtcConfigFile)){
    return true;
  }
  
  return true; 
}
void setup(void){
  /*
  MOSI: pin 11
  MISO: pin 12
  SCLK: pin 13
  MCLK: pin 9
  */
  pressureSensor.initialize(9, 13, 12, 11);
  PressureTempPair measurement = pressureSensor.getPressureTemp();
  lastPressure = measurement.pressure;
  lastTemp = measurement.temperature;
  
  
  //Set chip select pin to output
  pinMode(10, OUTPUT);
  
  //Attempt to initialize SD card
  waitForSD();
  
  //Check for RTC config file and use it to set the clock if present
  if (SD.exists(rtcConfigFile)){
    rtcConfig();
  }
  
  //Create a new file to log to
  char filename[] = "LOG00.csv";
  for (uint8_t i = 0; i < 100; i++){
    filename[3] = i/10 + '0';
    filename[4] = i%10 + '0';
    if (!SD.exists(filename)){
      logfile = SD.open(filename, FILE_WRITE);
      break;
    }
  }
  
  if (!logfile){
    sdError("ERROR: Could not create log file");
  }
  
  Wire.begin();
  if (!RTC.begin()){
    logfile.println("WARNING: RTC failed");
  }
  
  logfile.println("millis,time,pressure,temp,DO");
  /*if (logfile.writeError || !logfile.sync()){
    sdError("ERROR: Could not write header to card");
  }*/
  
  pinMode(greenLEDpin, OUTPUT);
  pinMode(yellowLEDpin, OUTPUT);
  pinMode(redLEDpin, OUTPUT);
  
  //Start Hardware serial for communication to DO Probe
  Serial.begin(38400);
  Serial.print("E\r");
  
  //Set aside some bytes for receiving data from sensor
  sensorString.reserve(30);
}

void loop(void){
  pressureSensor.generateClockSignal();
  
  DateTime now;
  
  delay((LOG_INTERVAL - 1) - (millis() % LOG_INTERVAL));
  
  digitalWrite(greenLEDpin, HIGH);
  
  uint32_t m = millis();
  logfile.print(m);
  logfile.print(",");
  
  now = RTC.now();
  logfile.print(now.year(), DEC);
  logfile.print("/");
  logfile.print(now.month(), DEC);
  logfile.print("/");
  logfile.print(now.day(), DEC);
  logfile.print(" ");
  logfile.print(now.hour(), DEC);
  logfile.print(":");
  logfile.print(now.minute(), DEC);
  logfile.print(":");
  logfile.print(now.second(), DEC);
  logfile.print(",");
  
  //Read Pressure Sensor
  //PressureTempPair measurement = pressureSensor.getPressureTemp();
  
  //Prepare Command String
  char commandString [10];
  char tempString [5];
  //dtostrf(measurement.temperature, 3, 2, tempString);
  //sprintf(commandString, "%s,0\r", tempString);
  
  
  //Read DO probe
  digitalWrite(yellowLEDpin, HIGH);
  //Serial.print(commandString);
  Serial.print("R\r");
  
  //char pressureTempString [10];
  //sprintf(pressureTempString, "%d,%s,", measurement.pressure, tempString);
  logfile.print(lastPressure);
  logfile.print(",");
  logfile.print(lastTemp);
  logfile.print(",");
  
  delay(650);
  
  while (Serial.available()||!stringComplete){
    char incomingChar = (char)Serial.read();
    sensorString += incomingChar;
    if (incomingChar == '\r'){
      stringComplete = true;
    }
  }
  
  //Serial.print("E\r");
  
  digitalWrite(yellowLEDpin, LOW);
  
  //Write sensor data to card and clear buffer and flag
  logfile.print(sensorString);
  sensorString = "";
  stringComplete = false;
  
  //Skip flush if sync interval has not been reached
  if ((millis() - syncTime) < SYNC_INTERVAL){
    return;
  }
  syncTime = millis();
  
  // blink LED to show we are syncing data to the card & updating FAT!
  digitalWrite(redLEDpin, HIGH);
  logfile.flush();
  //logfile.close();
  //SD.end();
  digitalWrite(redLEDpin, LOW);
  digitalWrite(greenLEDpin, LOW);

}   
