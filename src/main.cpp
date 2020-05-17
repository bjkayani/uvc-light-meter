// library includes
#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <SD.h>
#include <JC_Button.h>
#include "RTClib.h"

// pin definitions
#define BUTTON_A  9
#define BUTTON_B  6
#define BUTTON_C  5
const int chipSelect = 10;

// constant definitions
#define LOOP_TIME 100
#define MULT_A 0.00482*4
#define MULT_B 0.00483*4

// variable definitions
double sensValA = 0;
double sensValB = 0;
double uvIntenA = 0;
double uvIntenB = 0;
double doseA = 0;
double doseB = 0;
long int lastTime;
long int lastDoseTime;
bool logging = false;

// object decarations
Adafruit_SSD1306 display = Adafruit_SSD1306(128, 32, &Wire);
File dataFile;
Button buttonA(BUTTON_A); 
Button buttonB(BUTTON_B); 
RTC_PCF8523 rtc;
DateTime time;

// function initialization
void intializeDisplay();
void displayMeasurements();
void displayMsg(String msg);
void initializeSD();
void logMeasurements();
void toggleLogging();
void initializeRtc();
void calculateDose();
void readSensor();

// ----------Main Setup and Loop----------

void setup() {
  Serial.begin(9600);
  intializeDisplay();
  initializeRtc();
  buttonA.begin();
  buttonB.begin();
  lastTime = millis();
  lastDoseTime = millis();
}

void loop() {
  buttonA.read();
  buttonB.read();
  time = rtc.now();

  if((millis() - lastTime) > LOOP_TIME){
    
    readSensor();
    calculateDose();
    displayMeasurements();
    logMeasurements();
    lastTime = millis();
  }

  if (buttonA.wasReleased()) 
    {
      toggleLogging();
    }

  if (buttonB.wasReleased()) 
    {
      doseA = 0;
      doseB = 0;
      displayMsg("Dossage Reset");
    }
  
}

// ----------Display Related Functions----------

// initialize display
void intializeDisplay(){
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.clearDisplay();
  display.display();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
}

// display measurments on screen
void displayMeasurements(){
  display.clearDisplay();
  display.setCursor(0,0);
  display.print(String(uvIntenA)+"mW/cm2 "+String(doseA)+"J/cm2");
  display.setCursor(0,10);
  display.print(String(uvIntenB)+"mW/cm2 "+String(doseB)+"J/cm2");
  display.setCursor(0,20);
  display.print("Logging:"+String(logging));
  display.display();
}

// display message on screen
void displayMsg(String msg){
  display.clearDisplay();
  display.setCursor(0,0);
  display.print(msg);
  display.display();
  delay(1000);
}

// ----------SD Card Related Functions----------

// initialize SD card and open log file
void initializeSD(){
  pinMode(SS, OUTPUT);
  if (!SD.begin(chipSelect)) {
    displayMsg("SD Card Error");
    logging = false;
  }
  else{
    dataFile = SD.open("datalog.txt", FILE_WRITE);
    if (! dataFile) {
      displayMsg("Error Opening File");
      logging = false;
    }
    else{
      displayMsg("Logging Started");
    }
  }
}

// log measurements on SD card
void logMeasurements(){
  if(logging){
    String timeString = time.timestamp(DateTime::TIMESTAMP_FULL);
    String dataString = String(uvIntenA)+","+String(uvIntenB);
    dataFile.println(timeString +","+dataString);
    dataFile.flush();
    }
  }

// turn logging on and off
void toggleLogging(){
  logging = !logging;
  if(logging){
    initializeSD();
    dataFile.println("Time Stamp,Sensor A, Sensor B");
    dataFile.flush();
  }
  else{
    dataFile.close();
    SD.end();
    displayMsg("Logging Ended");
  }
}

// ----------RTC Related Functions----------
void initializeRtc(){
  if (! rtc.begin()) {
    displayMsg("RTC Error");
  }
  if (! rtc.initialized() || rtc.lostPower()) {
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }
}

// ----------Measurement Functions----------

// dosage calculation
void calculateDose(){
  int delta_t = millis() - lastDoseTime;
  doseA += (delta_t/1000.0) * uvIntenA * (1/1000.0);
  doseB += (delta_t/1000.0) * uvIntenB * (1/1000.0);
  lastDoseTime = millis();
}

// sensor read function
void readSensor(){
  for (int i=0; i <10; i++){
   sensValA = sensValA + analogRead(A5);
   sensValB = sensValB + analogRead(A4);
  }
  sensValA = sensValA / 10;
  sensValB = sensValB / 10;

  uvIntenA = sensValA * MULT_A;
  uvIntenB = sensValB * MULT_B;
}

