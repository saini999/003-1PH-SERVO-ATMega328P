/*
Project: NAS-002-1PH-SERVO
Description: Automatic Voltage Stablizer Control for Single Phase AC Supply
Project URL: https://github.com/saini999/003-1PH-SERVO-ATMega238P
Author: saini999, https://github.com/saini999 // Discord: N00R#2080

////////////////////////////////////////////////////
//// THIS WILL NOT WORK WITH ARDUINO UNO BOARD /////
/////// Due to less I/O Pins on UNO Board //////////
////////////////////////////////////////////////////

MCU: ATMega328P (28Pin PDIP)
MCU Arduino Core: MiniCore by MCUDude //https://github.com/MCUdude/MiniCore
MCU Clock: 8Mhz (Internal Oscillator)
Will only work with Internal Oscillator as Occillator Pins are being used as I/O.

Project Start Date: 17-Nov-2022
Last Update: 17-Nov-2022

Input Voltage: Pin PC0 (Through Voltage Divider)
Output Voltage: Pin PC1 (Through Voltage Divider)
Current CT Sensor: Pin PC2 (Through Voltage Divider)
Input AC supply for Frequency: Pin PB4 (Through Voltage Divider) ----------------+
                                                                                 | Same Pin
Servo Motor Forward: Pin PB6                                                     | Using as Frequency
Servo Motor Reverse: Pin PB7                                                     | Input
                                                                                 | 
ok/Menu button: Pin PC3                                                          | 
                                                                                 | To Open Setup
plus/Up button: Pin PC4                                                          | Press all buttons
minus/Down button: Pin PC5                                                       | at same time
setup/Settings button: Pin PB4 --------------------------------------------------+ 

Display I/O are Defined in setupDisplay() function at the end of the code.

Parameters: IHu/IHv = Input High Voltage
            ILu/ILv = Input Low Voltage
            OHu/OHv = Output High Voltage
            OLu/OLv = Output Low Voltage
            SETu/SETv = Set Voltage
            OuL/OvL = Overload Current
            TOn = Relay/Contactor ON Delay
            TOff = Relay/Contactor CUTOFF Delay
            DIFF = Voltage Difference from Set Voltage

Run Mode Display: InPu/InPv = Current Input Voltage
                  Outu/Outv = Current Output Voltage
                  LoAd =  Current (in Ampere)
                  FrEq = Current Frequency (Only updates once per display cycle)           

ALARMS: ErIn = Input Voltage Error (Either too low or too high)
        ErOt = Output Voltage Error (Either too low or too high)
        ErOL = OverLoad Error (Current drawn is more than the rated current)
        ErAL = All Alarms (Possible Reason: Board is not connected to Input and Output of Dimmer/Inverter
               Load Sensor is Not connected or this is first run and parameters are yet to be Configured)


*/

#include <EEPROM.h>
#include <BlockNot.h> //https://github.com/EasyG0ing1/BlockNot
#include "SevSeg.h" //https://github.com/sparkfun/SevSeg

#define ok PIN_PB4
#define plus PIN_PC4
#define minus PIN_PC5
#define hz PIN_PC3
#define inVolt PIN_PC0
#define outVolt PIN_PC1
#define current PIN_PC2
#define motor0Fwd PIN_PB6
#define motor0Rev PIN_PB7
#define power PIN_PB5


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

/*const int ok = PIN_PC3;
const int plus = PIN_PC4;
const int minus = PIN_PC5;
///////////////////////////////////////////////////////////////
///const int setupPin = PIN_PB4; //No Longer using Setup Pin///
///////////////////////////////////////////////////////////////
const int inVolt = PIN_PC0;
const int outVolt = PIN_PC1;
const int current = PIN_PC2;
const int motor0Fwd = PIN_PB6;
const int motor0Rev = PIN_PB7;
const int power = PIN_PB5;*/
int encMenu;
int menu;
long ontime,offtime;
float freq/*,hzdiff*/;
//int encMenu = 0;

bool okold = false;
bool plusold = false;
bool minusold = false;
bool resetrefresh = false;
bool alarmOnce = false;
bool onTimer = false;
bool offTimer = false;
bool hzold = false;
//uncomment these variables if running without setup pin
/**/
bool mode = false;
bool switched = false;
/**/

BlockNot on(TON, SECONDS);
BlockNot off(TOFF, SECONDS);

void setup() {
//setup 7Seg Display
setupDisplay();
//Setup Inputs
setIN(ok);
setIN(plus);
setIN(minus);
setIN(inVolt);
setIN(outVolt);
setIN(current);
setIN(hz);
//comment setup pin if not needed
//setIN(setupPin); //change setup mode from RUN/SETUP


//Set Outputs
setOUT(motor0Fwd);
setOUT(motor0Rev);
setOUT(power);

//Setup Parameter Variables

//This for Variables to read from Memory on startup
//comment these variables while testing in proteus
//uncomment when programming Arduino/MCU
/*
IHV = 2 * EEPROM.read(0);
ILV = 2 * EEPROM.read(1);
OHV = 2 * EEPROM.read(2);
OLV = 2 * EEPROM.read(3);
SETV = 2 * EEPROM.read(4);
OVL = EEPROM.read(5);
TON = EEPROM.read(6);
TOFF = EEPROM.read(7);
DIFF = EEPROM.read(8);
*/

//uncomment these variables while testing in proteus
//comment when programming Arduino/MCU
/**/
IHV = 560;
ILV = 480;
OHV = 580;
OLV = 420;
SETV = 512;
OVL = 800;
TON = 3;
TOFF = 0;
DIFF = 5;
/**/

//Setup Timers (BlockNot Lib)

on.setDuration(TON, SECONDS);
off.setDuration(TOFF, SECONDS);
on.reset();
off.reset();

}

void loop() {
  //Check OK/Menu Button
  checkok();
  //Check Plus/Up Button
  checkplus();
  //Check Minus/Down Button
  checkminus();

  //comment this if running without setup pin
  /*if(read(setupPin)){
    runSetup();
  } else {
    runNormal();
  }
*/

  //Switch to Parameter Edit/Run Mode
  /* Uncomment this for not using setup Pin*/
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
  /**/
}

//Check Input Frequency


void checkhz() {
  ontime = pulseIn(hz, HIGH);
  //offtime = pulseIn(hz, LOW, 30000);
  freq = 1000000.0 / (ontime * 2.0) * (5.0/5.7);//(ontime + offtime);
}

//Check If input voltage is within Low & High voltage Set by Parameters
bool inputVok() {
  if(IV() > ILV && IV() < IHV){
    return true;
  } else {
    return false;
  }
}
//Check If output voltage is within Low & High voltage Set by Parameters
bool outputVok() {
  if(OV() > OLV && OV() < OHV){
    return true;
  } else {
    return false;
  }
}

//Check If Current Load is lower than max current Set by Parameters
bool currentok() {
  if(amp() < OVL){
    return true;
  } else {
    return false;
  }
}


//Check Voltage Difference from Set Voltage
bool diffcheck() { //(returns true if difference is more than set difference and runs motor)
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

//Run Mode

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

//Check if Input,Output Voltage and current is within the set range

bool checksystem() {
  if(inputVok() && outputVok() && currentok()){
    return true;
  } else {
    return false;
  }
}

//Control Output Supply Relay

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

//Update Run Mode Screen

void updateScreenData(bool status) {
  //uncomment !mode and comment !read(setupPin) if setupPin is not being used
  if(!mode/*!read(setupPin)*/){
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
      if(menu == 6){
        checkhz();
      }
      menu++;
    }

    //Show Error if Available
    
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
      display("InPu", 0);
    }
    if(menu == 1){
      displayVar(IV(), 0);
    }
    if(menu == 2){
      display("Outu", 0);
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
      display("FrEq", 0);
    }
    if(menu == 7){
      displayVar((int)freq, 0);
    }
    if(menu == 8){
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

//Setp display on Setup Mode

void home() {
  display("SETP", 0);
}

//Setup Input High Voltage

void menuIHV() {
  if(refresh.triggered(false)){
    displayVar(enc, 0);
  } else {
    display("IHu", 0);
  }
}

//Setup Input low Voltage

void menuILV() {
  if(refresh.triggered(false)){
    displayVar(enc, 0);
  } else {
    display("ILu", 0);
  }
}

//Setup Output High Voltage

void menuOHV() {
  if(refresh.triggered(false)){
    displayVar(enc, 0);
  } else {
    display("OHu", 0);
  }
}

//Setup Output Low Voltage

void menuOLV() {
  if(refresh.triggered(false)){
    displayVar(enc, 0);
  } else {
    display("OLu", 0);
  }
}

//Setup Set Voltage

void menuSETV() {
  if(refresh.triggered(false)){
    displayVar(enc, 0);
  } else {
    display("SETu", 0);
  }
}

//Setup Overload

void menuOVL() {
  if(refresh.triggered(false)){
    displayVar(enc, 0);
  } else {
    display("OuL", 0);
  }
}

//Setup Waiting time for Output Relay to turn on

void menuTON() {
  if(refresh.triggered(false)){
    displayVar(enc, 0);
  } else {
    display("tOn", 0);
  }
}

//Setup Waiting time for Output Relay to turn off

void menuTOFF() {
  if(refresh.triggered(false)){
    displayVar(enc, 0);
  } else {
    display("tOFF", 0);
  }
}

//setup voltage difference from set voltage to move motor

void menuDIFF() {
  if(refresh.triggered(false)){
    displayVar(enc, 0);
  } else {
    display("dIFF", 0);
  }
}

//Change Screens/Menus on pessing OK/Menu

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

//Check OK Button Pressed

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

//Save Parameters to MCU EEPROM Memory (only if changed)

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

//Update Parameters on Menu Change

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

//Display INT Variable

void displayVar(int var, int deci) {
  char buffer[5];
  sprintf(buffer, "%4d", var);
  display1.DisplayString(buffer, deci);
}

//Check Plus Button Pressed

void checkplus() {
  if(read(plus) && plusold == !read(plus)){
  plusold = read(plus);
  enc++;
  }
  if(read(plus) == false){
  plusold = read(plus);
  }
}

//Check Minus Button Pressed

void checkminus() {
  if(read(minus) && minusold == !read(minus)){
  minusold = read(minus);
  enc--;
  }
  if(read(minus) == false){
  minusold = read(minus);
  }
}

// Setup Inputs

void setIN(int PIN) {
  pinMode(PIN, INPUT);
}

//Setup Outputs

void setOUT(int PIN) {
  pinMode(PIN, OUTPUT);
}

//Read Input

bool read(int PIN) {
  if(digitalRead(PIN)) {
    return true;
  } else {
    return false;
  }
}

//Display String Variable

void display(String str, int deci) {
  int strl = str.length();
  if(strl < 4) {
    //char16 = no display on screen
    str = char(16) + str;
  }

  int str_len = str.length() + 1;
  char data[str_len];
  str.toCharArray(data, str_len);
  display1.DisplayString(data, deci);
}

//Setup Sevenseg Display

void setupDisplay() {
    int displayType = COMMON_CATHODE;

   int digit1 = PIN_PB0; 
   int digit2 = PIN_PB1; 
   int digit3 = PIN_PB2; 
   int digit4 = PIN_PB3; 
   
   
   int segA = PIN_PD0; 
   int segB = PIN_PD1;
   int segC = PIN_PD2; 
   int segD = PIN_PD3; 
   int segE = PIN_PD4; 
   int segF = PIN_PD5; 
   int segG = PIN_PD6; 
   int segDP = PIN_PD7; 
   
  int numberOfDigits = 4; 

  display1.Begin(displayType, numberOfDigits, digit1, digit2, digit3, digit4, segA, segB, segC, segD, segE, segF, segG, segDP);
  
  display1.SetBrightness(100); 
}
