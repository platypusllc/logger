#include <SoftwareSerial.h>
#include <Wire.h>
#include <RTClib.h>
#include <SPI.h>
#include <MS5541C.h>


//Logging configuration
#define LOG_INTERVAL 1000 //Milliseconds between log entries
#define SYNC_INTERVAL 1000 //Milliseconds between SD card flushes

//LED pin definitions
#define LED1 3
#define LED2 4

//Instantiate Pressure Sensor object
MS5541C pressureSensor;

//Set last known temp and pressure
long lastPressure = 0;
float lastTemp = 0.0f;

//Instantiate Real Time Clock object
RTC_DS1307 RTC;


//OpenLog pin definitions
#define RXPin 7
#define TXPin 8
#define resetPin 5

//Initialize Software Serial for SD communication
SoftwareSerial SDCardSerial(TXPin, RXPin);

//RTC Config Filename
char rtcConfigFile[] = "RTC.cfg";

//Instantiate File object for logfile
//File logfile;

//Set last sync time
uint32_t syncTime = 0;

//Instantiate variables used in reading DO probe
String sensorString = "";
bool stringComplete = false;

//SD Error Function - Lights up Red LED and halts execution
void sdError(char *str){
  //todo: display string
  
  digitalWrite(LED2, HIGH);
  
  while(1);
}

//Wait For SD function - Resets logger and waits for "<" symbol
void waitForSD(void){
  pinMode(resetPin, OUTPUT);
  SDCardSerial.begin(9600);

  //Reset OpenLog
  digitalWrite(resetPin, LOW);
  delay(100);
  digitalWrite(resetPin, HIGH);

  //Wait for OpenLog to respond with '<' to indicate it is alive and recording to a file
  while(1) {
    digitalWrite(LED1, HIGH);
    if(SDCardSerial.available()){
      if(SDCardSerial.read() == '<') break;
    }
    delay(100);
    digitalWrite(LED1, LOW);
    delay(100);
  }
  digitalWrite(LED1, LOW);

}

//RTC Configuration function - configures RTC according to info found in RTC.cfg
bool rtcConfig(){

  
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
  //pinMode(10, OUTPUT);
  
  //Attempt to initialize SD card
  waitForSD();
  
  //Check for RTC config file and use it to set the clock if present
  /*if (SD.exists(rtcConfigFile)){
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
  }*/
  
  Wire.begin();
  if (!RTC.begin()){
    SDCardSerial.println("WARNING: RTC failed");
  }
  
  //RTC.adjust(DateTime(__DATE__, __TIME__));

  
  SDCardSerial.println("Uptime(mS),DateTime,Pressure(bar),Temp(C),DO(mg/L)");
  /*if (logfile.writeError || !logfile.sync()){
    sdError("ERROR: Could not write header to card");
  }*/
  
  pinMode(LED1, OUTPUT);
  pinMode(LED2, OUTPUT);
  
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
  
  uint32_t m = millis();
  SDCardSerial.print(m);
  SDCardSerial.print(",");
  
  now = RTC.now();
  SDCardSerial.print(now.year(), DEC);
  SDCardSerial.print("/");
  SDCardSerial.print(now.month(), DEC);
  SDCardSerial.print("/");
  SDCardSerial.print(now.day(), DEC);
  SDCardSerial.print(" ");
  SDCardSerial.print(now.hour(), DEC);
  SDCardSerial.print(":");
  SDCardSerial.print(now.minute(), DEC);
  SDCardSerial.print(":");
  SDCardSerial.print(now.second(), DEC);
  SDCardSerial.print(",");
  
  //Read Pressure Sensor
  PressureTempPair measurement = pressureSensor.getPressureTemp();
  
  //Prepare Command String
  char commandString [10];
  char tempString [5];
  dtostrf(measurement.temperature, 3, 2, tempString);
  //sprintf(commandString, "%s,0\r", tempString);
  
  
  //Read DO probe
  //digitalWrite(yellowLEDpin, HIGH);
  //Serial.print(commandString);
  Serial.print("R\r");
  
  SDCardSerial.print(measurement.pressure);
  SDCardSerial.print(",");
  SDCardSerial.print(tempString);
  SDCardSerial.print(",");

  
  delay(650);
  
  while (Serial.available()||!stringComplete){
    char incomingChar = (char)Serial.read();
    sensorString += incomingChar;
    if (incomingChar == '\r'){
      stringComplete = true;
    }
  }
  
  //Serial.print("E\r");
  
  //digitalWrite(yellowLEDpin, LOW);
  
  //Write sensor data to card and clear buffer and flag
  SDCardSerial.print(sensorString);
  sensorString = "";
  stringComplete = false;

}   
