/*
 
*/
#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_INA219.h>
#include "motor.h"
#include "hardware.h"



#define DIR_CLOSE      1
#define DIR_OPEN       0 

// define number of supported valves
//#define VALVECOUNT      12

#define TIMEOUT_UNDERCURRENT    150          // cycles of "byte motorcycle (byte valvenr, byte cmd)"

// befehle motor statemachine
#define CMD_M_OPEN      'o'
#define CMD_M_CLOSE     'c'
#define CMD_M_STOP      's'
#define CMD_M_STOP_ISR  'x'
#define CMD_M_NOTHING   'n'

// befehle app statemachine
// #define CMD_A_OPEN      'o'
// #define CMD_A_CLOSE     'c'
// #define CMD_A_LEARN     'l'
// #define CMD_A_TARGET    't'


// return values motor cycle
#define M_RES_INIT        1
#define M_RES_IDLE        2
#define M_RES_OPENS       3
#define M_RES_CLOSES      4
#define M_RES_TURNING     5
#define M_RES_ENDSTOP     6
#define M_RES_STOP        7
#define M_RES_STOP_ISR    8
#define M_RES_NOCURRENT   9


void set_motor (unsigned char valvenr, unsigned char dir);
void ena_motor (unsigned char valvenr, unsigned char state);
void isr_count ();
void callback_motorstop ();
byte motorcycle (byte valvenr, byte cmd);
byte appcycle(byte cmd, byte valvenr);

enum ASTATE appstate;

Adafruit_INA219 ina219;

//byte appstate = A_INIT;
//int counter;
//unsigned long time;
float        current_mA = 0;
unsigned int isr_counter;      // ISR var for counting revolutions
byte         isr_turning;      // ISR var for state of motor
unsigned int isr_target;       // ISR var for valve target count

valves myvalves[ACTUATOR_COUNT];

char command;
byte valvenr;
byte poschangecmd;

//volatile uint32_t revcounter;

// call from setup function in main
byte appsetup () {

  // valve MUX relay
  pinMode(CTRL_MUX, OUTPUT);

  // L293 enable pins
  pinMode(CTRL_ENA0, OUTPUT);
  pinMode(CTRL_ENA1, OUTPUT);
  pinMode(CTRL_ENA2, OUTPUT);
  pinMode(CTRL_ENA3, OUTPUT);
  pinMode(CTRL_ENA4, OUTPUT);
  pinMode(CTRL_ENA5, OUTPUT);

  // L293 direction pin
  pinMode(CTRL_DIRECTION, OUTPUT); 

  // input for counting revs
  pinMode(REVINPIN, INPUT);


  // Initialize the INA219
  if (! ina219.begin()) {
    Serial3.println("Failed to find INA219 chip");
    while (1) { delay(10); }
  }
  ina219.setCalibration_16V_320mA();  

  // external interrupt for counting revs
  //attachInterrupt(digitalPinToInterrupt(REVINPIN),isr_counter,RISING);



  // init valve data
  for (int x = 0;x<ACTUATOR_COUNT;x++) {
      myvalves[x].actual_position = 0;
      myvalves[x].target_position = 0;
      myvalves[x].meancurrent = 0;
      myvalves[x].scaler = 89;
      myvalves[x].status = VLV_STATE_UNKNOWN;
  }

  appstate = A_INIT;

  return 0;
}


byte appcycle () {
  
  byte temp;
  //byte stateresult;

  //static int cyclecnt = 0;
  //static int debouncecnt = 0;
  byte result;

  static byte pos_change;

  static unsigned int closing_count;  
  static unsigned int opening_count;
  static unsigned int deadzone_count;
  
  static unsigned int scaler;
  static byte valveindex;

  switch (appstate) {
    case A_INIT:  
                  if ( motorcycle(0, CMD_M_NOTHING) == M_RES_IDLE ) {                    
                     Serial3.println("A: init ready");
                     appstate = A_IDLE;
                     
                     //valveindex = 0;                     
                     //myvalves[valveindex].scaler = 130;
                     //myvalves[valveindex].target_position = 0;    

                     valveindex = 255;
                    
                  }
                  break;

    case A_IDLE:  
                  valveindex = valvenr;
                  if (command == CMD_A_OPEN) {                    
                    Serial3.print("A: cmd open for valve ");                  
                    Serial3.println(valveindex);                    
                    appstate = A_OPEN1;
                    pos_change = poschangecmd;
                  }
                  else if (command == CMD_A_CLOSE) {
                    Serial3.print("A: cmd close for valve ");                  
                    Serial3.println(valveindex);                                        
                    appstate = A_CLOSE1;  
                    pos_change = poschangecmd;
                  }
                  else if (command == CMD_A_LEARN) {                    
                    Serial3.print("A: cmd learn for valve ");                  
                    Serial3.println(valveindex);                    
                    appstate = A_LEARN1;  
                  }
                  else {
                    appstate = A_IDLE;  
                  }           

                  command = '\0';       // clear command for next loop call, prevents reevaluating
                  break;

    case A_OPEN1: // start valve opening
                  isr_target = myvalves[valveindex].scaler * pos_change;
                  
                  if (motorcycle (valveindex, CMD_M_OPEN) == M_RES_OPENS) {
                    Serial3.print("A: begin opening by ");                  
                    Serial3.println(pos_change);
                    myvalves[valveindex].status = VLV_STATE_OPENING;                                                            
                    appstate = A_OPEN2;
                    isr_counter=0;
                  }
                  else {
                    Serial3.print("A: cant open valve");                  
                    appstate = A_IDLE;
                  }
                  break;

    case A_OPEN2:  // wait for finish opening
                  temp = motorcycle (0, 0);
                  if (temp == M_RES_STOP) {
                    Serial3.println("A: opened valve");                  
                    appstate = A_IDLE;
                    myvalves[valveindex].actual_position += pos_change;
                    myvalves[valveindex].status = VLV_STATE_IDLE;                    
                    Serial3.print("A: new position "); Serial3.println(myvalves[valveindex].actual_position);
                  }
                  else if (temp == M_RES_NOCURRENT) {
                    Serial3.println("A: undercurrent");
                    myvalves[valveindex].status = VLV_STATE_OPENCIR;
                    appstate = A_IDLE;
                    isr_counter=0;
                  } 
                  else if (temp == M_RES_ENDSTOP) {
                    Serial3.println("A: opened valve to end stop");
                    appstate = A_IDLE;
                    myvalves[valveindex].status = VLV_STATE_IDLE;
                    myvalves[valveindex].actual_position = 100;
                    Serial3.print("A: new position "); Serial3.println(myvalves[valveindex].actual_position);
                  }                  
                  break;

    case A_CLOSE1: // start valve closing
                  isr_target = myvalves[valveindex].scaler * pos_change;
                  
                  if (motorcycle (valveindex, CMD_M_CLOSE) == M_RES_CLOSES) {
                    Serial3.print("A: begin closing by ");                  
                    Serial3.println(pos_change);       
                    myvalves[valveindex].status = VLV_STATE_CLOSING;                                                     
                    appstate = A_CLOSE2;
                    isr_counter=0;
                  }   
                  else {
                    Serial3.print("A: cant close valve");                  
                    appstate = A_IDLE;
                  }
                  break;

    case A_CLOSE2: // wait for finish closing
                  temp = motorcycle (0, 0);
                  if (temp == M_RES_STOP) {
                    Serial3.println("A: closed valve");
                    appstate = A_IDLE;
                    myvalves[valveindex].status = VLV_STATE_IDLE;
                    myvalves[valveindex].actual_position -= pos_change; 
                    Serial3.print("A: new position "); Serial3.println(myvalves[valveindex].actual_position);                   
                  }
                  else if (temp == M_RES_NOCURRENT) {
                    Serial3.println("A: undercurrent");
                    myvalves[valveindex].status = VLV_STATE_OPENCIR;
                    appstate = A_IDLE;
                    isr_counter=0;
                  } 
                  else if (temp == M_RES_ENDSTOP) {
                    Serial3.println("A: closed valve to end stop");
                    appstate = A_IDLE;
                    myvalves[valveindex].status = VLV_STATE_IDLE;
                    myvalves[valveindex].actual_position = 0;
                    Serial3.print("A: new position "); Serial3.println(myvalves[valveindex].actual_position);
                  }                  
                  break;                  
    
    case A_LEARN1: 
                  Serial3.println("A: try learning valve");                  
                  // first: closing completely
                  motorcycle (valveindex, CMD_M_CLOSE);
                  myvalves[valveindex].status = VLV_STATE_CLOSING;
                  appstate = A_LEARN2;
                  isr_target = 65535;       // max value to disable stopping
                  break;

    case A_LEARN2:
                  // waiting for closed valve
                  temp = motorcycle (valveindex, 0);
                  if (temp == M_RES_ENDSTOP) {
                    Serial3.println("A: closed valve before learning, now opening");                  
                    // second: opening completely and count rotations
                    isr_counter=0;
                    myvalves[valveindex].status = VLV_STATE_OPENING;
                    motorcycle (valveindex, CMD_M_OPEN);
                    isr_target = 65535;       // max value to disable stopping
                    appstate = A_LEARN3;
                  }
                  else if (temp == M_RES_NOCURRENT) {
                    Serial3.println("A: undercurrent");
                    myvalves[valveindex].status = VLV_STATE_OPENCIR;
                    appstate = A_IDLE;
                    isr_counter=0;
                  }               
                  break;
                  
    case A_LEARN3:
                  // waiting for opened valve
                  if (motorcycle (valveindex, 0) == M_RES_ENDSTOP) {
                    Serial3.println("A: opened valve for learning, now closing again");                  
                    opening_count = isr_counter;                    
                    // third: closing completely and count rotations
                    isr_counter=0;
                    myvalves[valveindex].status = VLV_STATE_CLOSING;
                    motorcycle (valveindex, CMD_M_CLOSE);
                    isr_target = 65535;       // max value to disable stopping
                    appstate = A_LEARN4;
                  }                  
                  break;
                  
    case A_LEARN4:
                  // waiting for closed valve again
                  if (motorcycle (valveindex, 0) == M_RES_ENDSTOP) {  
                    Serial3.println("A: closed valve for learning");                  
                    closing_count = isr_counter;                    
                                        
                    deadzone_count = closing_count - opening_count;
                    scaler = opening_count / 100;

                    Serial3.print("A: learned closing_count = "); Serial3.println(closing_count);
                    Serial3.print("A: learned opening_count = "); Serial3.println(opening_count);
                    Serial3.print("A: learned deadzone_count = "); Serial3.println(deadzone_count);
                    Serial3.print("A: learned scaler = "); Serial3.println(scaler);

                    myvalves[valveindex].closing_count = closing_count;
                    myvalves[valveindex].opening_count = opening_count;
                    myvalves[valveindex].deadzone_count = deadzone_count;
                    myvalves[valveindex].scaler = scaler;
                    myvalves[valveindex].actual_position = 0;    // because valve was closed completely                    
                    myvalves[valveindex].status = VLV_STATE_IDLE;

                    valveindex = 255;
                                                                                
                    appstate = A_IDLE;
                  }                  
                  break;

                  
    default:      appstate = A_IDLE;
                  break;  
  }
  result = 0;

  return result;
}


byte motorcycle (byte valvenr, byte cmd) {
  #define M_INIT      0
  #define M_IDLE      1
  #define M_OPEN      2
  #define M_CLOSE     3
  #define M_TURNING   4
  #define M_STOP      5
  #define M_STOP_ISR  6
  #define M_UNDERCURR 7

  static byte motorstate = M_INIT;
  static int cyclecnt = 0;
  static int debouncecnt = 0;
  static byte undercurrcnt = 0;
  byte result;

  // quickpass for isr state switching
  if(cmd==CMD_M_STOP_ISR) {
    motorstate = M_STOP_ISR;
    result = M_RES_STOP_ISR;
  }
  else {
    switch (motorstate) {
      case M_INIT:  MUX_OFF();         // relay MUX off
                    ena_motor(0, 0);
                    DIR_OFF();         // direction off
  
                    motorstate = M_IDLE;
                    isr_turning = 0;
                    isr_counter = 0;
                    isr_target = 0;
                    result = M_RES_INIT;
  
                    break;
  
      case M_IDLE:
                    if (cmd == CMD_M_OPEN) {
                      motorstate = M_OPEN;  
                      result = M_RES_OPENS;
                    }
                    else if (cmd == CMD_M_CLOSE) {
                      motorstate = M_CLOSE;  
                      result = M_RES_CLOSES;
                    }
                    else if (cmd == CMD_M_STOP) {
                      motorstate = M_STOP;
                      result = M_RES_STOP;  
                    }
                    else {
                      motorstate = M_IDLE;  
                      result = M_RES_IDLE;
                    }
  
                    // correct motor nr?
                    if (motorstate != M_IDLE && valvenr > ACTUATOR_COUNT) {
                      motorstate = M_IDLE;
                      result = M_RES_IDLE;
                    }

                    undercurrcnt = 0;
                    
                    break;
      
      case M_OPEN:  
                    Serial3.println("M: state open");                                  
                    set_motor(valvenr, DIR_OPEN);
                    delay(50);    // wait for relay
                    ena_motor(valvenr, 1);                  
  
                    motorstate = M_TURNING;
                    isr_counter=0;
                    cyclecnt = 0;
                    debouncecnt = 0;                  
                    result = M_RES_OPENS;
                    attachInterrupt(digitalPinToInterrupt(REVINPIN), isr_count, RISING);
                    
                    break;
                    
      case M_CLOSE:
                    Serial3.println("M: state close");                                 
                    set_motor(valvenr, DIR_CLOSE);
                    delay(50);    // wait for relay
                    ena_motor(valvenr, 1);
  
                    motorstate = M_TURNING;
                    isr_counter=0;
                    cyclecnt = 0;
                    debouncecnt = 0;
                    result = M_RES_CLOSES;
                    attachInterrupt(digitalPinToInterrupt(REVINPIN), isr_count, RISING);
  
                    break;
                    
      case M_TURNING:
                    isr_turning = 1;
                    result = M_RES_TURNING;
    
                    if (cmd == CMD_M_STOP) {
                      motorstate = M_STOP;  
                    }
                    cyclecnt++;
                                
                    if(debouncecnt<255) debouncecnt++;
                   
                    if (debouncecnt > 50 && cyclecnt > 50) {
                      cyclecnt=0;
                      Serial3.print("M: Current: "); Serial3.print(current_mA); Serial3.println(" mA");
                    }

                    //if (debouncecnt > 50 && cyclecnt > 5) {
                    if (debouncecnt > 7) {

                      current_mA = ina219.getCurrent_mA();

                      if(current_mA < 3 && current_mA > -3) {
                        undercurrcnt++;
                        if (undercurrcnt > TIMEOUT_UNDERCURRENT)
                        {
                          undercurrcnt = 0;
                          motorstate = M_UNDERCURR;
                        }                        
                      }

                      if(current_mA > 60 || current_mA < -60)
                      {
                        //motorstate = M_STOP;  
                        motorstate = M_IDLE;
                        result = M_RES_ENDSTOP;
                        ena_motor(0, 0);
                        isr_turning = 0;
                        MUX_OFF();
                    
                        Serial3.print("M: Current: "); Serial3.print(current_mA); Serial3.println(" mA");
                        Serial3.print("M: Cnt:     "); Serial3.println(isr_counter, DEC);                                 
                        Serial3.println("M: Autostop, reached end stop");
                      }
                      else 
                      {
                        //Serial3.print("M: Current: "); Serial3.print(current_mA); Serial3.println(" mA");
                      }
                      //cyclecnt=0;
                    }
                    break;
                      
      case M_STOP:         
                    //detachInterrupt(digitalPinToInterrupt(REVINPIN));
                    Serial3.println("M: state stop");    
                                                      
                    ena_motor(0, 0);
                    isr_turning = 0;
                    MUX_OFF();
                    
                    Serial3.print("M: Cnt: ");  
                    Serial3.println(isr_counter, DEC);                                 
  
                    motorstate = M_IDLE;
                    result = M_RES_STOP;                  
                    
                    break;

      case M_STOP_ISR: 
                    Serial3.println("M: target stop");  
                    motorstate = M_STOP;
                    result = M_RES_STOP_ISR;  
                    break;

      case M_UNDERCURR:         
                    detachInterrupt(digitalPinToInterrupt(REVINPIN));
                    Serial3.println("M: state undercurrent detected, stopped");    
                                                      
                    ena_motor(0, 0);
                    isr_turning = 0;
                    MUX_OFF();
                    
                    motorstate = M_IDLE;
                    result = M_RES_NOCURRENT;                  
                    
                    break;
                    
      default:      motorstate = M_IDLE;
                    result = M_RES_IDLE;
                    break;
    }  

  }
  return result;
}


// sets direction and mux relay
void set_motor (unsigned char valvenr, unsigned char dir) {

  if (valvenr % 2) { MUX_OFF(); }
  else { MUX_ON(); }
  
  if (dir == DIR_OPEN) { DIR_OFF(); } 
  else { DIR_ON(); }
}


// enables motor 
void ena_motor (unsigned char valvenr, unsigned char state) {

  if (state == 0) { 
    ENA0_OFF(); ENA1_OFF(); ENA2_OFF(); ENA3_OFF(); ENA4_OFF(); ENA5_OFF(); 
  }
  else {
    if (valvenr == 0 || valvenr == 1) { ENA0_ON(); }
    else if (valvenr == 2 || valvenr == 3) { ENA1_ON(); }
    else if (valvenr == 4 || valvenr == 5) { ENA2_ON(); }
    else if (valvenr == 6 || valvenr == 7) { ENA3_ON(); }
    else if (valvenr == 8 || valvenr == 9) { ENA4_ON(); }
    else if (valvenr == 10 || valvenr == 11) { ENA5_ON(); }
  }
}


void isr_count () {
  if (isr_turning) isr_counter++;
  if (isr_target > 0) isr_target--;
  else callback_motorstop();
}


// call from isr to stop motor immediately
void callback_motorstop () {
  detachInterrupt(digitalPinToInterrupt(REVINPIN));
  ena_motor(0, 0);
  isr_turning = 0;
  motorcycle(0, CMD_M_STOP_ISR);  
}



// returns state of application  state machine
enum ASTATE appgetstate () {
  return appstate;
}



int16_t appsetaction(char cmd, byte valveindex, byte posdelta) {

  if(appstate == A_IDLE) {
    valvenr = valveindex;
    command = cmd;
    poschangecmd = posdelta;
    return 0;
  }
  else return -1;

}