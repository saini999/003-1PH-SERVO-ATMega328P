/*
Project: NAS-002-1PH-SERVO
Description: Automatic Voltage Stablizer Control for Single Phase AC Supply
Project URL: https://github.com/saini999/003-1PH-SERVO-ATMega238P
Author: Harnoor Saini
Github: https://github.com/saini999
Discord: N00R#2080
Email: noor@noorautomation.in

////////////////////////////////////////////////////
//// THIS WILL NOT WORK WITH ARDUINO UNO BOARD /////
/////// Due to less I/O Pins on UNO Board //////////
////////////////////////////////////////////////////

MCU: ATMega328P (28Pin PDIP)
MCU Arduino Core: MiniCore by MCUDude //https://github.com/MCUdude/MiniCore
MCU Clock: 8Mhz (Internal Oscillator)

MCU Fuse Bits: 
 FuseLow : 0xE2
 FuseHigh: 0xD9
 For BOD 2.7v - FuseExtended: 0xFD
 For BOD 4.3v - FuseExtended: 0xFC //The one i use.

Fuses Can be Set by reading the microcontroller datasheet or from the Website: https://www.engbedded.com/fusecalc/   //Working As of 24Dec2022


WARN: This will only work with Internal Oscillator as Occillator Pins are being used as I/O.

Project Start Date: 17-Nov-2022
Last Update: 24-Dec-2022

Input Voltage: Pin PC0 (Through Voltage Divider)
Output Voltage: Pin PC1 (Through Voltage Divider)
Current CT Sensor: Pin PC2 (Through Voltage Divider)
Input AC supply for Frequency: Pin PB4 (Through Voltage Divider) ----------------+
                                                                                 | Same Pin
Servo Motor Forward: Pin PB6                                                     | Using as Frequency
Servo Motor Reverse: Pin PB7                                                     | Input as Setup Pin is No longer being used
                                                                                 | Due to I/O Limitation Issues.
ok/Menu button: Pin PC3                                                          | 
                                                                                 |
plus/Up button: Pin PC4 -----------------------+ To Open Setup Press All Buttons |
minus/Down button: Pin PC5                     |       At the Same Time!         |
setup/Settings button: Pin PB4 ----------------+---------------------------------+ 

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
        Er0b = Bugged Alarm (Possible that two Alarms are active at same time instead of all Three)

*/

#include <EEPROM.h>
#include <BlockNot.h> //https://github.com/EasyG0ing1/BlockNot  //Can be Installed in Arduino IDE's Library Manager
#include "SevSeg.h" //https://github.com/sparkfun/SevSeg  //Can't be installed by library Manager, You'll have to manually downlaod the files.


//Define Microcontroller Pins Instead of using Variables to optimize the code

#define ok PIN_PB5
#define plus PIN_PC1
#define minus PIN_PC0
#define hz PIN_PC2
#define inVolt PIN_PC3
#define outVolt PIN_PC4
#define current PIN_PC5
#define motor0Fwd PIN_PB4
#define motor0Rev PIN_PB3
#define power PIN_PB2

//Intialize the SevenSegament Display
SevSeg display1;
//Timer that updates Main screen every Second.
BlockNot refresh(1, SECONDS);
//Timer that updates all Input Variables
BlockNot checkin(250);

//All Global Variables
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
/////////////////////////////////////////////////////////////////////////////////////
/*const int ok = PIN_PC3;
const int plus = PIN_PC4;
const int minus = PIN_PC5;
///////////////////////////////////////////////////////////////
///const int setupPin = PIN_PB4; //No Longer using Setup Pin///
///////////////////////////////////////////////////////////////
const int inVolt = PIN_PC0;
const int outVolt = PIN_PC1;
const int current = PIN_PC2;                                 No Longer Being Used....
const int motor0Fwd = PIN_PB6;
const int motor0Rev = PIN_PB7;
const int power = PIN_PB5;
int encMenu = 0;*/
///////////////////////////////////////////////////////////////////////////////////////
int encMenu;
int menu;
long ontime,offtime;
float freq;   /*,hzdiff*/
int involtage,outvoltage,currentload;


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
//Setup Display Data
setupDisplay();
//Setup Inputs 
//setIN(); is a custom Function Defined at the bottom of the Program!
setIN(ok);
setIN(plus);
setIN(minus);
setIN(inVolt);
setIN(outVolt);
setIN(current);
setIN(hz);
///////////////////////////////////////////////////////
/////////comment setup pin if not needed///////////////
//setIN(setupPin); //change setup mode from RUN/SETUP//
///////////////////////////////////////////////////////

//Set Outputs
//setOUT(); is a custom Function Defined at the bottom of the Program!
setOUT(motor0Fwd);
setOUT(motor0Rev);
setOUT(power);

//Setup Parameter Variables

//This for Variables to read from EEPROM Memory of the uC on startup
//comment these variables while testing in proteus
//uncomment when programming Arduino/MCU
/**/

//using the EEPROM.h Librabry Included by the Arduino IDE & Arduino Core

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
///////////////////////////////////////////////////////////////////////
//uncomment these variables while testing in proteus
//comment when programming Arduino/MCU
/*
IHV = 560;
ILV = 50;
OHV = 580;
OLV = 50;
SETV = 220;                           ONLY FOR TESTING PURPOSES!
OVL = 800;
TON = 3;
TOFF = 0;
DIFF = 5;
*/

//Setup Timers (BlockNot Lib)
/////////////////////////////////////////////////////////////////////////

//Setup Variable Timers after the variable is read from the Memory!
on.setDuration(TON, SECONDS);
off.setDuration(TOFF, SECONDS);
//Reset the Timers after time update.
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
////////////////////////////////////////////////////////
  //comment this if running without setup pin
  /*if(read(setupPin)){
    runSetup();
  } else {
    runNormal();
  }
*/
///////////////////////////////////////////////////////
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

//Checks the wavelenght of the Sine wave to calculate the frequency
//ontime is the time that sinewave stays Positive or above the 0v Threshold
//offtime is the time that sinewave stays negative or below the 0v Threshold
void checkhz() {
  //calculates the pulse width in milliseconds //timesout after 60ms to avoid blocking other code if no input is detected!
  ontime = pulseIn(hz, HIGH, 60);
  offtime = pulseIn(hz, LOW, 60);
  //Calculates the Frequency by dividing the ontime and offtime with 1 sec time (or 1000000 ms)
  freq = 1000000.0 / (ontime + offtime);//(ontime + offtime);
}


//Check If input voltage is within Low & High voltage Set by Parameters
bool inputVok() {
  if(involtage > ILV && involtage < IHV){
    return true;
  } else {
    return false;
  }
}
//Check If output voltage is within Low & High voltage Set by Parameters
bool outputVok() {
  if(outvoltage > OLV && outvoltage < OHV){
    return true;
  } else {
    return false;
  }
}

//Check If Current Load is lower than max current Set by Parameters
bool currentok() {
  if(currentload < OVL){
    return true;
  } else {
    return false;
  }
}


//Check Voltage Difference from Set Voltage
bool diffcheck() { //(returns true if difference is more than set difference and runs motor)
  int dif = SETV - outvoltage;
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
  if(checkin.triggered()){
    //////////////////////////////
    //OVo();                    //
    //IVo();                    //
    //ampo();                   //
    ////////////////////////////// No Longer Used...!
    checkinputs();
  }
  //digitalWrite(motor0Rev, HIGH);
  if(outvoltage < SETV && diffcheck() && inputVok() && currentok()){
    digitalWrite(motor0Fwd, HIGH);
  } else {
    digitalWrite(motor0Fwd, LOW);
  }
  if(outvoltage > SETV && diffcheck() && inputVok() && currentok()){
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
  if(inputVok()) {
    if(outputVok()) {
      if(currentok()){
            return true;
      } else { return false; }
    } else { return false; }
  } else { return false; }
}

//Control Output Supply Relay

void updatePower() {
  if(checksystem()){
    if(on.triggered(false)){  
      digitalWrite(power, HIGH);
      off.reset();
    }
  }
  else {
    if(off.triggered(false)) {
      digitalWrite(power, LOW);
      on.reset();
    }
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

      OVo();
      IVo();
      ampo();


      switch (menu)
      {
        case 6:
          checkhz();
          break;
        case 0:
          IVo();
          break;
        case 2:
          OVo();
          break;
        case 4:
          ampo();
          break;
        default:
          break;
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

    switch (menu)
    {
    case 0:
      display("InPu", 0);
      break;
    case 1:
      displayVar(involtage, 0);
      break;
    case 2:
      display("Outu", 0);
      break;
    case 3:
      displayVar(outvoltage, 0);
      break;
    case 4:
      display("LoAd", 0);
      break;
    case 5:
      displayVar(currentload, 0);
      break;
    case 6:
      display("FrEq", 0);
      break;
    case 7:
      displayVar((int)freq, 0);
      break;
    case 8:
      if(status){
        menu = 0;
      } else {
        menu = -1;
      }
      break;
    }

    /*if(menu == 0){
      display("InPu", 0);
    }
    if(menu == 1){
      displayVar(involtage, 0);
    }
    if(menu == 2){
      display("Outu", 0);
    }
    if(menu == 3){
      displayVar(outvoltage, 0);
    }
    if(menu == 4){
      display("LoAd", 0);
    }
    if(menu == 5){
      displayVar(currentload, 0);
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
    }*/
  }
}
//update inputs once based on highest voltage...
////////////////////////////////////////////////////////////////
/*
void IVo() {
  involtage = 0.343 * analogRead(inVolt);
}

void OVo() {
  outvoltage = 0.343 * analogRead(outVolt);
}

void ampo() {
  currentload = 0.343 * analogRead(current); /// No Longer Used...
}
*/
/////////////////////////////////////////////////////////////////


//update inputs once based on highest voltage...
void checkinputs() {
  IVo();
  OVo();
  ampo();
}

void IVo() {
float inpv;
for(int i=0; i<10; i++) {
  if((0.343 * analogRead(inVolt)) > inpv){
    inpv = 0.343 * analogRead(inVolt);
    }
  }
involtage = inpv;
}



void OVo() {
float oppv;
for(int i=0; i<10; i++) {
  if((0.343 * analogRead(outVolt)) > oppv){
    oppv = 0.343 * analogRead(outVolt);
    }
  }
outvoltage = oppv;
}

void ampo() {
float ampov;
for(int i=0; i<10; i++) {
  if((0.343 * analogRead(current)) > ampov){
    ampov = 0.343 * analogRead(current);
    }
  }
currentload = ampov;
}

////////////////////////////////////////////////////////////
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

void menuEND() {
  display("End", 0);
}

//Change Screens/Menus on pessing OK/Menu

void runSetup() {
  digitalWrite(motor0Fwd, LOW);
  digitalWrite(motor0Rev, LOW);
  
  switch (encMenu)
  {
  case 0:
    home();
    break;
  case 1:
    menuIHV();
    break;
  case 2:
    menuILV();
    break;
  case 3:
    menuOHV();
    break;
  case 4:
    menuOLV();
    break;
  case 5:
    menuSETV();
    break;
  case 6:
    menuOVL();
    break;
  case 7:
    menuTON();
    break;
  case 8:
    menuTOFF();
    break;
  case 9:
    menuDIFF();
    break;
  case 10:
    menuEND();
    break;
  default:
    encMenu = 0;
    break;
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
  
  switch (encMenu)
  {
  case 0:
    break;
  case 1:
    enc = IHV;
    done();
    break;
  case 2:
    IHV = enc;
    enc = ILV;
    done();
    break;
  case 3:
    ILV = enc;
    enc = OHV;
    done();
    break;
  case 4:
    OHV = enc;
    enc = OLV;
    done();
    break;
  case 5:
    OLV = enc;
    enc = SETV;
    done();
    break;
  case 6:
    SETV = enc;
    enc = OVL;
    done();
    break;
  case 7:
    OVL = enc;
    enc = TON;
    done();
    break;
  case 8:
    TON = enc;
    enc = TOFF;
    on.setDuration(TON, SECONDS);
    on.reset();
    done();
    break;
  case 9:
    TOFF = enc;
    enc = DIFF;
    off.setDuration(TOFF, SECONDS);
    off.reset();
    done();
    break;
  case 10:
    DIFF = enc;
    done();
    break;
  default:
    break;
  }
}

void done() {display("donE", 0);}

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
    int displayType = COMMON_ANODE;

   int digit1 = PIN_PB1; 
   int digit2 = PIN_PD4; 
   int digit3 = PIN_PD1; 
   int digit4 = PIN_PD2; 
   
   
   int segA = PIN_PD6; 
   int segB = PIN_PD0;
   int segC = PIN_PB6; 
   int segD = PIN_PD7; 
   int segE = PIN_PB0; 
   int segF = PIN_PB7; 
   int segG = PIN_PD3; 
   int segDP = PIN_PD5; 
   
  int numberOfDigits = 4; 

  display1.Begin(displayType, numberOfDigits, digit1, digit2, digit3, digit4, segA, segB, segC, segD, segE, segF, segG, segDP);
  
  display1.SetBrightness(100); 
}
