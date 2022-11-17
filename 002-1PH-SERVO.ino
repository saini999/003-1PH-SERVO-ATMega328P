/*
Project: NAS-002-1PH-SERVO
Description: Automatic Voltage Stablizer Control for Single Phase AC Supply
Project URL: https://github.com/saini999/002-1PH-SERVO
Author: saini999, https://github.com/saini999 // Discord: N00R#2080

Arduino Board: Arduino Mega2560 ( Changed to Mega2560 from Uno due to less IOs on Uno)

Project Start Date: 16-Nov-2022
Last Update: 16-Nov-2022

Input Voltage: Pin A0 (Through Voltage Divider)
Output Voltage: Pin A1 (Through Voltage Divider)
Current CT Sensor: Pin A2 (Through Voltage Divider)

Servo Motor Forward: Pin 5
Servo Motor Reverse: Pin 6

ok/Menu button: Pin 2
plus/Up button: Pin 3
minus/Down button: Pin 4
setup/Settings button: Pin 7

Parameters: IHu/IHv = Input High Voltage
            ILu/ILv = Input Low Voltage
            OHu/OHv = Output High Voltage
            OLu/OLv = Output Low Voltage
            SETu/SETv = Set Voltage
            OuL/OvL = Overload Current
            TOn = Relay/Contactor ON Delay
            TOff = Relay/Contactor CUTOFF Delay
            DIFF = Voltage Difference from Set Voltage
            

ALARMS: ErIn = Input Voltage Error (Either too low or too high)
        ErOt = Output Voltage Error (Either too low or too high)
        ErOL = OverLoad Error (Current drawn is more than the rated current)
        ErAL = All Alarms (Possible Reason: Board is not connected to Input and Output of Dimmer/Inverter
               Load Sensor is Not connected or this is first run and parameters are yet to be Configured)


*/

#include <EEPROM.h>
#include <BlockNot.h> //https://github.com/EasyG0ing1/BlockNot
#include "SevSeg.h" //https://github.com/sparkfun/SevSeg


SevSeg display1;

//BlockNot screen(2, SECONDS);
BlockNot refresh(1, SECONDS);
bool setupm = false;
int IHV;
int ILV;
int OHV;
int OLV;
int SETV;
int OVL;
int TON;
int TOFF;
int DIFF;
int enc;
const int ok = 2;
const int plus = 3;
const int minus = 4;
const int setupPin = 7;
const int inVolt = A0;
const int outVolt = A1;
const int current = A2;
const int motor0Fwd = 5;
const int motor0Rev = 6;
const int power = 22;
int encMenu;
int menu;
//int encMenu = 0;

bool okold = false;
bool plusold = false;
bool minusold = false;
bool resetrefresh = false;
bool alarmOnce = false;
bool onTimer = false;
bool offTimer = false;
//uncomment these variables if running without setup pin
/*
bool mode = false;
bool switched = false;
*/

BlockNot on(TON, SECONDS);
BlockNot off(TOFF, SECONDS);

void setup() {
setupDisplay();
setIN(ok);
setIN(plus);
setIN(minus);
//comment setup pin if not needed
setIN(setupPin); //change setup mode from RUN/SETUP
setIN(inVolt);
setIN(outVolt);
setIN(current);
setOUT(motor0Fwd);
setOUT(motor0Rev);
setOUT(power);


//This for Variables to read from Memory on startup
//comment these variables while testing in proteus
/**/
IHV = 2 * EEPROM.read(0);
ILV = 2 * EEPROM.read(1);
OHV = 2 * EEPROM.read(2);
OLV = 2 * EEPROM.read(3);
SETV = 2 * EEPROM.read(4);
OVL = EEPROM.read(5);
TON = EEPROM.read(6);
TOFF = EEPROM.read(7);
DIFF = EEPROM.read(8);
/**/

//uncomment these variables while testing in proteus
/*
IHV = 560;
ILV = 480;
OHV = 580;
OLV = 420;
SETV = 512;
OVL = 800;
TON = 3;
TOFF = 0;
DIFF = 5;
*/

on.setDuration(TON, SECONDS);
off.setDuration(TOFF, SECONDS);
on.reset();
off.reset();

}



void loop() {
  checkok();
  checkplus();
  checkminus();
  //comment this if running without setup pin
  if(read(setupPin)){
    runSetup();
  } else {
    runNormal();
  }

  /* Uncomment this for not using setup Pin
  if(mode){
    runSetup();
  } else {
    runNormal();
  }
  if(read(ok) && read(plus) && read(minus) && switched == false){
    mode = !mode;
    switched = true;
    encMenu = 0;
  }
  if(!read(ok) && !read(plus) && !read(minus) && switched == true){
    switched = false;
  }
  */
}
bool inputVok() {
  if(IV() > ILV && IV() < IHV){
    return true;
  } else {
    return false;
  }
}
bool outputVok() {
  if(OV() > OLV && OV() < OHV){
    return true;
  } else {
    return false;
  }
}
bool currentok() {
  if(amp() < OVL){
    return true;
  } else {
    return false;
  }
}

bool diffcheck() {
  int dif = SETV - OV();
  if(dif < 0){
    dif = dif * -1;
  }
  if(dif > DIFF){
    return true;
  } else {
    return false;
  }
}

void runNormal() {
  if(OV() < SETV && diffcheck() && inputVok() && currentok()){
    digitalWrite(motor0Fwd, HIGH);
  } else {
    digitalWrite(motor0Fwd, LOW);
  }
  if(OV() > SETV && diffcheck() && inputVok() && currentok()){
    digitalWrite(motor0Rev, HIGH);
  } else {
    digitalWrite(motor0Rev, LOW);
  }

  if(checksystem()){
    updateScreenData(true);
  } else {
    updateScreenData(false);
  }

  updatePower();

}
bool checksystem() {
  if(inputVok() && outputVok() && currentok()){
    return true;
  } else {
    return false;
  }
}

void updatePower() {
  if(checksystem()){
  if(on.triggered(false))  
    digitalWrite(power, HIGH);
    off.reset();
  } else if(off.triggered(false)) {
    digitalWrite(power, LOW);
    on.reset();
  }
}


void updateScreenData(bool status) {
  //uncomment !mode and comment !read(setupPin) if setupPin is not being used
  if(/*!mode*/!read(setupPin)){
    if(!resetrefresh){
      refresh.reset();
      resetrefresh = true;
    }
    if(!status && !alarmOnce){
      alarmOnce = true;
      menu == -1;
    }
    if(status && alarmOnce){
      alarmOnce = false;
      menu == 0;
    }
    if(refresh.triggered()){
      menu++;
    }
    if(!status && menu == -1){
      if(!inputVok() && !outputVok() && !currentok()){
        display("ErAL", 0);
      } else {
        if(!inputVok()){
          display("ErIn", 0);
        }
        if(!outputVok()){
          display("ErOt", 0);
        }
        if(!currentok()){
          display("ErOL", 0);
        }
      }
    }
    if(menu == 0){
      display("InUL", 0);
    }
    if(menu == 1){
      displayVar(IV(), 0);
    }
    if(menu == 2){
      display("OPUL", 0);
    }
    if(menu == 3){
      displayVar(OV(), 0);
    }
    if(menu == 4){
      display("LoAd", 0);
    }
    if(menu == 5){
      displayVar(amp(), 0);
    }
    if(menu == 6){
      if(status){
        menu = 0;
      } else {
        menu = -1;
      }
    }
  }
}


int IV() {
  return analogRead(inVolt);
}

int OV() {
  return analogRead(outVolt);
}

int amp() {
  return analogRead(current);
}

void home() {
  display("SETP", 0);
}

void menuIHV() {
  if(refresh.triggered(false)){
    displayVar(enc, 0);
  } else {
    display("IHu", 0);
  }
}

void menuILV() {
  if(refresh.triggered(false)){
    displayVar(enc, 0);
  } else {
    display("ILu", 0);
  }
}

void menuOHV() {
  if(refresh.triggered(false)){
    displayVar(enc, 0);
  } else {
    display("OHu", 0);
  }
}

void menuOLV() {
  if(refresh.triggered(false)){
    displayVar(enc, 0);
  } else {
    display("OLu", 0);
  }
}

void menuSETV() {
  if(refresh.triggered(false)){
    displayVar(enc, 0);
  } else {
    display("SETu", 0);
  }
}

void menuOVL() {
  if(refresh.triggered(false)){
    displayVar(enc, 0);
  } else {
    display("OuL", 0);
  }
}

void menuTON() {
  if(refresh.triggered(false)){
    displayVar(enc, 0);
  } else {
    display("tOn", 0);
  }
}

void menuTOFF() {
  if(refresh.triggered(false)){
    displayVar(enc, 0);
  } else {
    display("tOFF", 0);
  }
}

void menuDIFF() {
  if(refresh.triggered(false)){
    displayVar(enc, 0);
  } else {
    display("dIFF", 0);
  }
}

void runSetup() {
  if(encMenu == 0) {
    home();
  }
  if(encMenu == 1) {
    menuIHV();
  }
  if(encMenu == 2) {
    menuILV();
  }
  if(encMenu == 3) {
    menuOHV();
  }
  if(encMenu == 4) {
    menuOLV();
  }
  if(encMenu == 5) {
    menuSETV();
  }
  if(encMenu == 6) {
    menuOVL();
  }
  if(encMenu == 7) {
    menuTON();
  }
  if(encMenu == 8) {
    menuTOFF();
  }
  if(encMenu == 9) {
    menuDIFF();
  }
  if(encMenu > 9) {
    encMenu = 0;
  }
  if(encMenu < 0) {
    encMenu = 9;
  }
}



void checkok() {
  if(read(ok) && okold == !read(ok)){
  okold = read(ok);
  encMenu++;
  refresh.reset();
  encUpdate();
  eepromUpdate();
  }
  if(read(ok) == false){
  okold = read(ok);
  }
}

void eepromUpdate() {
  EEPROM.update(0, IHV/2);
  EEPROM.update(1, ILV/2);
  EEPROM.update(2, OHV/2);
  EEPROM.update(3, OLV/2);
  EEPROM.update(4, SETV/2);
  EEPROM.update(5, OVL);
  EEPROM.update(6, TON);
  EEPROM.update(7, TOFF);
  EEPROM.update(8, DIFF);
}

void encUpdate() {
  if(encMenu == 0) {
    DIFF = enc;
  }
  if(encMenu == 1) {
    enc = IHV;
    }
  if(encMenu == 2) {
    IHV = enc;
    enc = ILV;
    }
  if(encMenu == 3) {
    ILV = enc;
    enc = OHV;
    }
  if(encMenu == 4) {
    OHV = enc;
    enc = OLV;
    }
  if(encMenu == 5) {
    OLV = enc;
    enc = SETV;
    }
  if(encMenu == 6) {
    SETV = enc;
    enc = OVL;
    }
  if(encMenu == 7) {
    OVL = enc;
    enc = TON;
    }
  if(encMenu == 8) {
    TON = enc;
    enc = TOFF;
    on.setDuration(TON, SECONDS);
    on.reset();
    }
  if(encMenu == 9) {
    TOFF = enc;
    enc = DIFF;
    off.setDuration(TOFF, SECONDS);
    off.reset();
    }
}

void displayVar(int var, int deci) {
  char buffer[5];
  sprintf(buffer, "%04d", var);
  display1.DisplayString(buffer, deci);
}

void checkplus() {
  if(read(plus) && plusold == !read(plus)){
  plusold = read(plus);
  enc++;
  }
  if(read(plus) == false){
  plusold = read(plus);
  }
}

void checkminus() {
  if(read(minus) && minusold == !read(minus)){
  minusold = read(minus);
  enc--;
  }
  if(read(minus) == false){
  minusold = read(minus);
  }
}

void setIN(int PIN) {
  pinMode(PIN, INPUT);
}

void setOUT(int PIN) {
  pinMode(PIN, OUTPUT);
}

bool read(int PIN) {
  if(digitalRead(PIN)) {
    return true;
  } else {
    return false;
  }
}

void display(String str, int deci) {
  int strl = str.length();
  if(strl < 4) {
    str = char(16) + str;
  }

  int str_len = str.length() + 1;
  char data[str_len];
  str.toCharArray(data, str_len);
  display1.DisplayString(data, deci);
}

void setupDisplay() {
    int displayType = COMMON_CATHODE;

   int digit1 = 50; 
   int digit2 = 51; 
   int digit3 = 52; 
   int digit4 = 53; 
   
   
   int segA = 42; 
   int segB = 43;
   int segC = 44; 
   int segD = 45; 
   int segE = 46; 
   int segF = 47; 
   int segG = 48; 
   int segDP = 49; 
   
  int numberOfDigits = 4; 

  display1.Begin(displayType, numberOfDigits, digit1, digit2, digit3, digit4, segA, segB, segC, segD, segE, segF, segG, segDP);
  
  display1.SetBrightness(100); 
}
