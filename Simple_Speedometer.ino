// Simple Speedometer
// This is a simple, inexpensive speedometer for model railoads
// It uses an Arduino UNO or similar
// a 16x2 I2C LCD display 
// and two IR proximity sensors
// it detects the speed of a train moving either direction on a single track mainline
// constants are provided for calciulating the speed in several popular scales, 
// and in MPH or km/h
// Displayed text is factored out so it can be localized 
// or easily changed for individual preferences

// Robert Myers 2023 
// This work is licensed under a Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International License.

// the proper values of SPEED_DIVISOR, LENGTH,
// SPEEDLABEL and CAR_GAP 
// must be set for the desured scale, sensor distance,
// speed label (MPH, or km/h)
// and allowance for gaps between cars

#include <Wire.h>
#include <LCD_I2C.h>
LCD_I2C lcd(0x27,16,2);

// uncomment next line to enable serial monitor
// #define MONITOR = 1;

// these constants define the divisor for calculating speed
// uncomment one for your scale and preferred output
// const float SPEED_DIVISOR = .002032;      // Z Scale MPH
// const float SPEED_DIVISOR = .002794;      // N
const float SPEED_DIVISOR = .005133;         // HO
// const float SPEED_DIVISOR = .006985;   // S
// const float SPEED_DIVISOR = .009313;   // O
// const float SPEED_DIVISOR = .013970;   // 1
// const float SPEED_DIVISOR = .015400;   // G US Prototype
// const float SPEED_DIVISOR = .018627;   // G 1/24
// const float SPEED_DIVISOR = .019868;   // G LGB
// const float SPEED_DIVISOR = .022000;   // F

// const float SPEED_DIVISOR = .001263;   // Z Scale km/h
// const float SPEED_DIVISOR = .001263;   // N
// const float SPEED_DIVISOR = .003190;   // HO
// const float SPEED_DIVISOR = .004340;   // S
// const float SPEED_DIVISOR = .005787;   // O
// const float SPEED_DIVISOR = .008681;   // 1
// const float SPEED_DIVISOR = .009569;   // G US Prototype
// const float SPEED_DIVISOR = .011574;   // G 1/24
// const float SPEED_DIVISOR = .012346;   // G LGB
// const float SPEED_DIVISOR = .013670;   // F

// const float SPEED_DIVISOR = .002933;   // 2mm Scale MPH
// const float SPEED_DIVISOR = .003025;   // British N
// const float SPEED_DIVISOR = .005867;   // OO
// const float SPEED_DIVISOR = .009313;   // British O, 7mm/ft

// const float SPEED_DIVISOR = .001823;   // 2mm Scale km/h
// const float SPEED_DIVISOR = .001880;   // British N
// const float SPEED_DIVISOR = .003645;   // OO
// const float SPEED_DIVISOR = .006379;   // British O, 7mm/ft

// const float SPEED_DIVISOR = .003725;   // TT 1/120 Scale MPH
// const float SPEED_DIVISOR = .002315;   // TT 1/120 Scale km/h
// const float SPEED_DIVISOR = .004470;   // TT 1/100 Scale MPH
// const float SPEED_DIVISOR = .002778;   // TT 1/100 Scale km/h

// change this to match your speed trap
const float LENGTH = 300.0;     // distance between sensors in mm

// time in milliseconds to allow for gaps between cars 
// increase for larger scales or slower trains
const int CAR_GAP = 1000;     
// use appropriater for your units
const char* SPEEDLABEL = " MPH    ";
//const char* SPEEDLABEL = "km/h    ";

const char* SPEEDOMETER_LABEL = "SPEEDOMETER";
const char* DISTANCE_LABEL = "Distance: ";
const char* MM_LABEL = "mm";
const char* SPEED_DIVISOR_LABEL = " Divisor: ";
const char* CAR_GAP_LABEL = " Car gap: ";
const char* READY_LABEL = "     Ready      ";
const char* WESTBOUND_TRAIN_LABEL = "WESTBOUND TRAIN";
const char* EASTBOUND_TRAIN_LABEL = "EASTBOUND TRAIN";
const char* DETECTED_LABEL = "DETECTED";
const char* EAST_SENSOR_CLEAR_LABEL = "  CLEAR EAST   ";
const char* WEST_SENSOR_CLEAR_LABEL = "  CLEAR WEST   ";
const char* TRAIN_CLEAR_LABEL = " TRAIN CLEAR   ";
const char* SPACES = "    ";

const int eastSensor = 2;
const int westSensor = 3;
const int button = 5;
unsigned long startmillis = 0;
unsigned long endmillis = 0;

enum SPEEDOSTATE
{
  ST_IDLE,            // between trains
  ST_WEST_DETECTED,   // westbound train detected, counting time
  ST_WEST_SPEED,      // waiting to clear east sensor
  ST_WEST_CLEAR,      // waiting to clear west sensor
  ST_EAST_DETECTED,   // eastbound traindetected, counting time
  ST_EAST_SPEED,      // waiting to cleasr west sensor
  ST_EAST_CLEAR,      // waiting to clear east sensor
};

SPEEDOSTATE speedoState=ST_IDLE;


void setup() {
  // put your setup code here, to run once:
#ifdef MONITOR
  Serial.begin(9600);
  Serial.println("Simple Speedometer");
#endif
  lcd.begin();
  lcd.backlight();
  pinMode(eastSensor, INPUT);
  pinMode(westSensor, INPUT);
  pinMode(button, INPUT_PULLUP);
  reset();
}

// reset speedometer if it gets confused, say by switching
void reset() {
  lcd.clear();
  lcd.setCursor(2,0);
  lcd.print(SPEEDOMETER_LABEL);
  lcd.setCursor(0,1);
  lcd.print(DISTANCE_LABEL);
  lcd.print(int(LENGTH));
  lcd.print(MM_LABEL);
  delay(2000);
  lcd.setCursor(0,1);
  lcd.print(SPEED_DIVISOR_LABEL);
  lcd.print(SPEED_DIVISOR*10000.0);
  lcd.print(SPACES);
  delay(2000);
  lcd.setCursor(0,1);
  lcd.print(CAR_GAP_LABEL);
  lcd.print(CAR_GAP);
  lcd.print(SPACES);

 #ifdef MONITOR
  Serial.println("Speedometer Reset");
  Serial.print("Sensor Distance ");
  Serial.print(LENGTH);
  Serial.println(" mm");
  Serial.print("Speed Divisor ");
  Serial.println(SPEED_DIVISOR*10000.0);
  Serial.print("Inter-car gap time "); 
  Serial.print(CAR_GAP);
  Serial.println("ms");
#endif
  delay(2000);
  lcd.setCursor(0,1);
  lcd.print(READY_LABEL);
  startmillis = 0;
  endmillis = 0;
  speedoState = ST_IDLE;
}

void loop() {  
  if (digitalRead(button) == LOW) {
    reset();
    while (digitalRead(button) == LOW) {
      delay(1000);    // time to let button up    
    }    
  }
    switch (speedoState) {
    case ST_IDLE: checkSensors(); break;
    case ST_WEST_DETECTED: westCounting(); break;
    case ST_WEST_SPEED: westWaitClear(); break;
    case ST_WEST_CLEAR: westClear(); break;
    case ST_EAST_DETECTED: eastCounting(); break;
    case ST_EAST_SPEED: eastWaitClear(); break;
    case ST_EAST_CLEAR: eastClear(); break;
    }
    delay(1);
}

// wait for a train to tripped a sensor
void checkSensors() {
#ifdef MONITOR
  Serial.println("ST_IDLE");
#endif  
  if (digitalRead(eastSensor) == LOW) {    // train detected westbound
    startmillis = millis();
    speedoState = ST_WEST_DETECTED;
    lcd.clear(); 
    lcd.setCursor(1,0);
    lcd.print(WESTBOUND_TRAIN_LABEL);
    lcd.setCursor(5,1);
    lcd.print(DETECTED_LABEL);    
#ifdef MONITOR    
    Serial.print("Westbound train detected at ");
    Serial.println(startmillis); 
#endif
}
  if (digitalRead(westSensor) == LOW) {    // train detected westbound
    startmillis = millis();
    speedoState = ST_EAST_DETECTED;
    lcd.clear();
    lcd.setCursor(1,0);
    lcd.print(EASTBOUND_TRAIN_LABEL);
    lcd.setCursor(5,1);
    lcd.print(DETECTED_LABEL);    
#ifdef MONITOR    
    Serial.print("Eastbound train detected at ");
    Serial.println(startmillis); 
#endif
  }
}

// we are seeing a westgound train, wait for it to hit sensor to calculate speed
void westCounting(){        // wait to trip west sensor

  long int elapsed;
  float speed;

#ifdef MONITOR
  Serial.println("ST_WEST_DETECTED");
#endif  
  if (digitalRead(westSensor) == LOW) {
        
    speedoState = ST_WEST_SPEED;
    endmillis = millis();
    elapsed = endmillis - startmillis;
    speed = LENGTH/elapsed / SPEED_DIVISOR;
    lcd.setCursor(4,1);
    lcd.print(int(speed +.5));
    lcd.print(SPEEDLABEL);  
              
#ifdef MONITOR
    Serial.print("Westbound time ");
    Serial.println(elapsed);
    Serial.print("Westbound speed ");
    Serial.print(speed);
    Serial.println(SPEEDLABEL);
#endif
  }
}

// westbound train has tripped the west sensor, wait for east to clear
void westWaitClear() {

#ifdef MONITOR
  Serial.println("ST_WEST_SPEED");
#endif  
  if (digitalRead(eastSensor) == HIGH) {
    delay(CAR_GAP);
    if (digitalRead(eastSensor) == HIGH) {
      speedoState = ST_WEST_CLEAR;
      lcd.setCursor(1,0);
      lcd.print(EAST_SENSOR_CLEAR_LABEL);
    }      
#ifdef MONITOR
      Serial.print("East clear ");
    Serial.println(millis());
#endif
  }
}

// see if westbound train has cleared the west sensor
void westClear(){

#ifdef MONITOR
  Serial.println("ST_WEST_CLEAR");
#endif  
 if (digitalRead(westSensor) == HIGH) {
    delay(CAR_GAP);
    if (digitalRead(westSensor) == HIGH) {
      speedoState = ST_IDLE;
      lcd.setCursor(1,0);
      lcd.print(TRAIN_CLEAR_LABEL);
    }      
#ifdef MONITOR
      Serial.print("West clear ");
    Serial.println(millis());
#endif
  }
}

// we are seeing an eastbound train, wait for it to trip east sensor
void eastCounting(){

  long int elapsed;
  float speed;

#ifdef MONITOR
  Serial.println("ST_EAST_DETECTED");
#endif  
  if (digitalRead(eastSensor) == LOW) {
    speedoState = ST_EAST_SPEED;    
    endmillis = millis();
    elapsed = endmillis - startmillis;
    speed = LENGTH/elapsed / SPEED_DIVISOR;
    lcd.setCursor(4,1);
    lcd.print(int(speed + .5));
    lcd.print(SPEEDLABEL);              
#ifdef MONITOR
    Serial.print("Eastbound time ");
    Serial.println(elapsed);
    Serial.print("Eastbound speed ");
    Serial.print(speed);
    Serial.println(SPEEDLABEL);
#endif
  }
}

// see if eastbound train has cleared the west sensor
void eastWaitClear() {

#ifdef MONITOR
  Serial.println("ST_EAST_SPEED");
#endif  
  if (digitalRead(westSensor) == HIGH) {
    delay(CAR_GAP);
    if (digitalRead(westSensor) == HIGH) {

      speedoState = ST_EAST_CLEAR;
      lcd.setCursor(1,0);
      lcd.print(WEST_SENSOR_CLEAR_LABEL);      
#ifdef MONITOR
      Serial.print("West sensor clear ");
      Serial.println(millis());
#endif
    }
  }
}

// see if eastbound train has cleared the east sensor
void eastClear() {
#ifdef MONITOR
  Serial.println("ST_EAST_CLEAR");
#endif  
 if (digitalRead(eastSensor) == HIGH) {
    delay(CAR_GAP);
    if (digitalRead(eastSensor) == HIGH) {
      speedoState = ST_IDLE;
      lcd.setCursor(1,0);
      lcd.print(TRAIN_CLEAR_LABEL);
      
#ifdef MONITOR
      Serial.print("East clear ");
      Serial.println(millis());
#endif
   }
 }  
}
