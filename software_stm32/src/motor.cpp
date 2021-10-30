/*
 
*/
#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_INA219.h>
#include "motor.h"
#include "hardware.h"
#include "STM32TimerInterrupt.h"      // library https://github.com/khoih-prog/STM32_TimerInterrupt
#include "terminal.h"
#include "eeprom.h"
#include "temperature.h"


//#define COMM_DBG				Serial3		// serial port for debugging
#define COMM_DBG				Serial6		// serial port for debugging


#define DIR_CLOSE      (int)  1
#define DIR_OPEN       (int)  0 

#define TIMER0_INTERVAL_MS        1


#define TIMEOUT_UNDERCURRENT    150          // cycles of "byte motorcycle (byte valvenr, byte cmd)"
#define THRESHOLD_UNDERCURRENT  20           // threshold for detecting undercurrent in 1/10 mA

#define ANINCURRENTGAIN         136         // µA/digit - gain analog input valve current 

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


void set_motor (int smvalvenr, int dir);
void ena_motor (int emvalvenr, int state);
void isr_count ();
void callback_motorstop ();
byte motorcycle (int mvalvenr, byte cmd);
byte appcycle(byte cmd, byte valvenr);

void TimerHandler0();

enum ASTATE appstate;

Adafruit_INA219 ina219;

// Init STM32 timer TIM1
STM32Timer ITimer0(TIM1);

//byte appstate = A_INIT;
//int counter;
//unsigned long time;
int current_mA = 0;                     // current valve motor in 1/10 mA

int analog_current = 0;                 // current valve motor in 1/10 mA read by analog pin
int analog_current_old = 0;             // filter

unsigned int idlecurrent = 2048;        // idle current adc value (digits)
unsigned int idlecurrent_old = 2048;    // filter

volatile unsigned int isr_counter;      // ISR var for counting revolutions
volatile byte         isr_turning;      // ISR var for state of motor
volatile unsigned int isr_target;       // ISR var for valve target count
volatile int          isr_valvenr;      // ISR var for valve nr
volatile byte         isr_timer_go;     // ISR var for timer pwm start
volatile byte         isr_timer_fin;    // ISR var for timer pwm finished

valves myvalves[ACTUATOR_COUNT];

char command;
int valvenr;
byte poschangecmd;
unsigned int m_meancurrent;           // mean current in mA

              

//volatile uint32_t revcounter;

// call from setup function in main
byte appsetup () {

  uint8_t   numberOfDevices;
  DeviceAddress currAddress;
  uint8_t   found;
  uint8_t   valveindexlast;

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
    COMM_DBG.println("Failed to find INA219 chip");
    while (1) { delay(10); }
  }
  ina219.setCalibration_16V_320mA();  

  // external interrupt for counting revs
  //attachInterrupt(digitalPinToInterrupt(REVINPIN),isr_counter,RISING);


  // init valve data
  for (unsigned int x = 0;x<ACTUATOR_COUNT;x++) {
      myvalves[x].actual_position = 50;
      myvalves[x].target_position = 50;
      myvalves[x].meancurrent = 20;
      myvalves[x].scaler = 89;
      myvalves[x].status = VLV_STATE_UNKNOWN;
      myvalves[x].sensorindex = VALVE_SENSOR_UNKNOWN;        // marks that no slot is selected
      myvalves[x].learn_movements = LEARN_AFTER_MOVEMENTS_DEFAULT;
      // distribute learn timing equaly over valve slots
      myvalves[x].learn_time = (unsigned int) (((long)LEARN_AFTER_TIME_DEFAULT * ((long)x+1)) / (long)ACTUATOR_COUNT);
  }

  // match sensor address from eeprom with found sensors and set index/slot to valve struct
  COMM_DBG.println("Read 1-wire sensor addresses from eeprom");
  numberOfDevices = sensors.getDeviceCount();  
    
  for (unsigned int owsensorindex=0; owsensorindex<numberOfDevices; owsensorindex++)
  {
      sensors.getAddress(currAddress, owsensorindex);
      //tempsensors[i].address = currAddress;

      //COMM_DBG.print("owsensorindex ");
      //COMM_DBG.println(owsensorindex, DEC);
      printAddress(currAddress);
      //COMM_DBG.println("");
      
      // step through all possible valves
      for (unsigned int valveindex = 0;valveindex<ACTUATOR_COUNT;valveindex++) {
        valveindexlast = valveindex;

        //COMM_DBG.print("valveindex ");
        //COMM_DBG.println(valveindex, DEC);

        found = 0;
        if (eep_content.owsensors[valveindex].crc == currAddress[7]) 
        {
            found++;
            for (unsigned z = 0;z<6;z++) {
                if (eep_content.owsensors[valveindex].romcode[z] == currAddress[1+z]) {
                    found++;
                }
            }           
        }
        if (found == 7) 
        {          
          valveindex = ACTUATOR_COUNT; // end for loop
        }
      }

      if (found == 7)
      {
          COMM_DBG.print(" found at valve: ");
          // COMM_DBG.print(owsensorindex, DEC);
          COMM_DBG.println(valveindexlast, DEC);
          myvalves[valveindexlast].sensorindex = owsensorindex;
      }
      else 
      {
           COMM_DBG.println(" not found");
           //COMM_DBG.println(found, DEC);
     }
      // else 
      // {
      //     COMM_DBG.print("not found ");
      //     COMM_DBG.print(owsensorindex, DEC);
      //     COMM_DBG.println(valveindexlast, DEC);
      //     myvalves[valveindexlast].sensorindex = VALVE_SENSOR_UNKNOWN;
      // }

  }

  appstate = A_INIT;


  // Interval in microsecs
  if (ITimer0.attachInterruptInterval(TIMER0_INTERVAL_MS * 1000, TimerHandler0))
  {
    COMM_DBG.print(F("Starting ITimer0 OK, millis() = ")); COMM_DBG.println(millis());
  }
  else
    COMM_DBG.println(F("Can't set ITimer0. Select another freq. or timer"));


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

  static int idlecurrenttimer = 0;
  
  static unsigned int scaler;
  static int valveindex;

  switch (appstate) {
    case A_INIT:  
                  if ( motorcycle(0, CMD_M_NOTHING) == M_RES_IDLE ) {                    
                     COMM_DBG.println("A: init ready");
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
                    COMM_DBG.print("A: cmd open for valve ");                  
                    COMM_DBG.println(valveindex, 10);                    
                    appstate = A_OPEN1;
                    pos_change = poschangecmd;
                  }
                  else if (command == CMD_A_CLOSE) {
                    COMM_DBG.print("A: cmd close for valve ");                  
                    COMM_DBG.println(valveindex, 10);                                        
                    appstate = A_CLOSE1;  
                    pos_change = poschangecmd;
                  }
                  else if (command == CMD_A_OPEN_END) {                    
                    COMM_DBG.print("A: cmd open to endstop for valve ");                  
                    COMM_DBG.println(valveindex, 10);                    
                    appstate = A_OPEN1;
                    pos_change = 255;
                  }
                  else if (command == CMD_A_CLOSE_END) {                    
                    COMM_DBG.print("A: cmd close to endstop for valve ");                  
                    COMM_DBG.println(valveindex, 10);                    
                    appstate = A_CLOSE1;
                    pos_change = 255;
                  }
                  else if (command == CMD_A_LEARN) {                    
                    COMM_DBG.print("A: cmd learn for valve ");                  
                    COMM_DBG.println(valveindex, 10);                    
                    appstate = A_LEARN1;  
                  }
                  else {
                    appstate = A_IDLE;  
                  }           

                  // decrement learning counter
                  if(appstate == A_OPEN1 || appstate == A_CLOSE1) {
                    if(myvalves[valveindex].learn_movements) myvalves[valveindex].learn_movements--;
                  }

                  // measure idle current for offset (drift) compensation
                  if (idlecurrenttimer < 10) 
                  {
                    idlecurrenttimer++;
                  }
                  else
                  {                      
                    idlecurrenttimer = 0;
                    idlecurrent = (unsigned int) ((analogRead(ANINCURRENT) * 20 + idlecurrent_old * 980) / 1000);
                    idlecurrent_old = idlecurrent;
                  }


                  command = '\0';       // clear command for next loop call, prevents reevaluating
                  break;

    case A_OPEN1: // start valve opening
                  if(pos_change==255) isr_target = 65535;
                  else isr_target = myvalves[valveindex].scaler * pos_change;
                  m_meancurrent = myvalves[valveindex].meancurrent;
                  
                  if (motorcycle (valveindex, CMD_M_OPEN) == M_RES_OPENS) {
                    COMM_DBG.print("A: begin opening by ");                  
                    COMM_DBG.println(pos_change);
                    myvalves[valveindex].status = VLV_STATE_OPENING;                                                            
                    appstate = A_OPEN2;
                    isr_counter=0;
                  }
                  else {
                    COMM_DBG.print("A: cant open valve");                  
                    appstate = A_IDLE;
                  }
                  break;
  
    case A_OPEN2:  // wait for finish opening
                  temp = motorcycle (valveindex, 0);
                  if (temp == M_RES_STOP) {
                    COMM_DBG.println("A: opened valve");                  
                    appstate = A_IDLE;
                    myvalves[valveindex].actual_position += pos_change;
                    myvalves[valveindex].status = VLV_STATE_IDLE;                    
                    COMM_DBG.print("A: new position "); COMM_DBG.println(myvalves[valveindex].actual_position);
                  }
                  else if (temp == M_RES_NOCURRENT) {
                    COMM_DBG.println("A: undercurrent");
                    myvalves[valveindex].status = VLV_STATE_OPENCIR;
                    myvalves[valveindex].target_position = myvalves[valveindex].actual_position;
                    appstate = A_IDLE;
                    isr_counter=0;
                  } 
                  else if (temp == M_RES_ENDSTOP) {
                    COMM_DBG.println("A: opened valve to end stop");
                    appstate = A_IDLE;
                    myvalves[valveindex].status = VLV_STATE_IDLE;
                    myvalves[valveindex].actual_position = 100;
                    COMM_DBG.print("A: new position "); COMM_DBG.println(myvalves[valveindex].actual_position);
                  }                  
                  break;

    case A_CLOSE1: // start valve closing
                  if(pos_change==255) isr_target = 65535;
                  else isr_target = myvalves[valveindex].scaler * pos_change;
                  m_meancurrent = myvalves[valveindex].meancurrent;
                  
                  if (motorcycle (valveindex, CMD_M_CLOSE) == M_RES_CLOSES) {
                    COMM_DBG.print("A: begin closing by ");                  
                    COMM_DBG.println(pos_change);       
                    myvalves[valveindex].status = VLV_STATE_CLOSING;                                                     
                    appstate = A_CLOSE2;
                    isr_counter=0;
                  }   
                  else {
                    COMM_DBG.print("A: cant close valve");                  
                    appstate = A_IDLE;
                  }
                  break;

    case A_CLOSE2: // wait for finish closing
                  temp = motorcycle (valveindex, 0);
                  if (temp == M_RES_STOP) {
                    COMM_DBG.println("A: closed valve");
                    appstate = A_IDLE;
                    myvalves[valveindex].status = VLV_STATE_IDLE;
                    myvalves[valveindex].actual_position -= pos_change; 
                    COMM_DBG.print("A: new position "); COMM_DBG.println(myvalves[valveindex].actual_position);                   
                  }
                  else if (temp == M_RES_NOCURRENT) {
                    COMM_DBG.println("A: undercurrent");
                    myvalves[valveindex].status = VLV_STATE_OPENCIR;
                    myvalves[valveindex].target_position = myvalves[valveindex].actual_position;
                    appstate = A_IDLE;
                    isr_counter=0;
                  } 
                  else if (temp == M_RES_ENDSTOP) {
                    COMM_DBG.println("A: closed valve to end stop");
                    appstate = A_IDLE;
                    myvalves[valveindex].status = VLV_STATE_IDLE;
                    myvalves[valveindex].actual_position = 0;
                    COMM_DBG.print("A: new position "); COMM_DBG.println(myvalves[valveindex].actual_position);
                  }                  
                  break;                  
    
    case A_LEARN1: 
                  COMM_DBG.println("A: try learning valve");                  
                  // first: closing completely
                  motorcycle (valveindex, CMD_M_CLOSE);
                  myvalves[valveindex].status = VLV_STATE_CLOSING;
                  appstate = A_LEARN2;
                  m_meancurrent = myvalves[valveindex].meancurrent;
                  isr_target = 65535;       // max value to disable stopping
                  break;

    case A_LEARN2:  // goto start position
                  // waiting for closed valve as start position
                  temp = motorcycle (valveindex, 0);
                  if (temp == M_RES_ENDSTOP) {
                    COMM_DBG.println("A: closed valve before learning, now opening");                  
                    // second: opening completely and count rotations
                    isr_counter=0;
                    myvalves[valveindex].status = VLV_STATE_OPENING;
                    motorcycle (valveindex, CMD_M_OPEN);
                    isr_target = 65535;       // max value to disable stopping
                    appstate = A_LEARN3;
                    m_meancurrent = myvalves[valveindex].meancurrent;
                  }
                  else if (temp == M_RES_NOCURRENT) {
                    COMM_DBG.println("A: undercurrent");
                    myvalves[valveindex].status = VLV_STATE_OPENCIR;
                    myvalves[valveindex].target_position = myvalves[valveindex].actual_position;
                    appstate = A_IDLE;
                    isr_counter=0;
                  }               
                  break;
                  
    case A_LEARN3:  // learning opening way
                  // waiting for opened valve
                  temp = motorcycle (valveindex, 0);
                  if (temp == M_RES_ENDSTOP) {
                    COMM_DBG.println("A: opened valve for learning, now closing again");                  
                    opening_count = isr_counter;   
                    if(m_meancurrent > 0) {
                      myvalves[valveindex].meancurrent = m_meancurrent;
                      COMM_DBG.print("A: learned mean current = "); COMM_DBG.println(myvalves[valveindex].meancurrent);
                    }                 
                    // third: closing completely and count rotations
                    isr_counter=0;
                    myvalves[valveindex].status = VLV_STATE_CLOSING;
                    motorcycle (valveindex, CMD_M_CLOSE);
                    isr_target = 65535;       // max value to disable stopping
                    m_meancurrent = 20;
                    appstate = A_LEARN4;
                  }          
                  else if (temp == M_RES_NOCURRENT) {
                    COMM_DBG.println("A: undercurrent");
                    myvalves[valveindex].status = VLV_STATE_OPENCIR;
                    myvalves[valveindex].target_position = myvalves[valveindex].actual_position;
                    appstate = A_IDLE;
                    isr_counter=0;
                  }                       
                  break;
                  
    case A_LEARN4:  // learning closing way
                  // waiting for closed valve again
                  temp = motorcycle (valveindex, 0);
                  if (temp == M_RES_ENDSTOP) {  
                    COMM_DBG.println("A: closed valve for learning");                  
                    closing_count = isr_counter;  

                    if(m_meancurrent > 0) {
                      COMM_DBG.print("A: learned mean current = "); COMM_DBG.println(m_meancurrent);
                      myvalves[valveindex].meancurrent = (myvalves[valveindex].meancurrent + m_meancurrent) / 2;                      
                    }                      
                                        
                    deadzone_count = closing_count - opening_count;
                    scaler = opening_count / 100;

                    COMM_DBG.print("A: learned closing_count = "); COMM_DBG.println(closing_count);
                    COMM_DBG.print("A: learned opening_count = "); COMM_DBG.println(opening_count);
                    COMM_DBG.print("A: learned deadzone_count = "); COMM_DBG.println(deadzone_count);
                    COMM_DBG.print("A: learned scaler = "); COMM_DBG.println(scaler);
                    COMM_DBG.print("A: learned mean current = "); COMM_DBG.println(myvalves[valveindex].meancurrent);

                    myvalves[valveindex].closing_count = closing_count;
                    myvalves[valveindex].opening_count = opening_count;
                    myvalves[valveindex].deadzone_count = deadzone_count;
                    myvalves[valveindex].scaler = scaler;
                    myvalves[valveindex].actual_position = 0;    // because valve was closed completely                    
                    myvalves[valveindex].status = VLV_STATE_IDLE;

                    valveindex = 255;
                                                                                
                    appstate = A_IDLE;
                  }    
                  else if (temp == M_RES_NOCURRENT) {
                    COMM_DBG.println("A: undercurrent");
                    myvalves[valveindex].status = VLV_STATE_OPENCIR;
                    myvalves[valveindex].target_position = myvalves[valveindex].actual_position;
                    appstate = A_IDLE;
                    isr_counter=0;
                  }                
                  break;

                  
    default:      appstate = A_IDLE;
                  break;  
  }
  result = 0;

  return result;
}



byte app_10s_loop () {

  unsigned int x = 0;

  // walk through valves and evaluate learning values
  for (x=0; x< ACTUATOR_COUNT; x++)
  { 
    // learning times
    if(myvalves[x].learn_time <= 10) {
      myvalves[x].learn_time = LEARN_AFTER_TIME_DEFAULT;
      myvalves[x].status = VLV_STATE_UNKNOWN;     // mark state as unknown, net set target req will do a learning cycle
      COMM_DBG.print("Valve "); COMM_DBG.print(x, 10); COMM_DBG.println(" will be learned soon");
    }
    else myvalves[x].learn_time -= 10;    

    // learning movements
    if(myvalves[x].learn_movements == 0) {
      myvalves[x].learn_movements = LEARN_AFTER_MOVEMENTS_DEFAULT;
      myvalves[x].status = VLV_STATE_UNKNOWN;     // mark state as unknown, net set target req will do a learning cycle
      COMM_DBG.print("Valve "); COMM_DBG.print(x, 10); COMM_DBG.println(" will be learned soon");
    }    
  }

return 0;
}


byte motorcycle (int mvalvenr, byte cmd) {
  #define M_INIT      0
  #define M_IDLE      1
  #define M_OPEN      2
  #define M_CLOSE     3
  #define M_TURNING   4
  #define M_TURNON    8
  #define M_STOP      5
  #define M_STOP_ISR  6
  #define M_UNDERCURR 7

  static byte motorstate = M_INIT;

  static int cyclecnt = 0;
  static int debouncecnt = 0;
  static int testcnt = 0;
  static byte undercurrcnt = 0;

  static int currentbound_low;              // lower current limit for detection of end stop
  static int currentbound_high;             // upper current limit for detection of end stop

  static unsigned int meancurrent_cnt;      // counts meancurrent values
  static long meancurrent_mem;              // memory for meancurrent values
  byte result;

  // quickpass for isr state switching
  if(cmd==CMD_M_STOP_ISR) {
    motorstate = M_STOP_ISR;
    //result = M_RES_STOP_ISR;
  }
  //else {
    switch (motorstate) {
      case M_INIT:  MUX_OFF();         // relay MUX off
                    ena_motor(0, 0);
                    DIR_OFF();         // direction off
  
                    motorstate = M_IDLE;
                    isr_turning = 0;
                    isr_counter = 0;
                    isr_target = 0;
                    result = M_RES_INIT;
                    isr_timer_go = 0;
                    isr_timer_fin = 0;
  
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
                    if (motorstate != M_IDLE && mvalvenr > (int) ACTUATOR_COUNT) {
                      motorstate = M_IDLE;
                      result = M_RES_IDLE;
                    }

                    undercurrcnt = 0;
                    
                    break;
      
      case M_OPEN:  
                    COMM_DBG.println("M: state open");                                  
                    set_motor(mvalvenr, DIR_OPEN);
                    delay(50);    // wait for relay
                    //ena_motor(valvenr, 1);                  
  
                    motorstate = M_TURNON;
                    isr_valvenr = mvalvenr;
                    isr_counter=0;
                    cyclecnt = 0;
                    debouncecnt = 0;                  
                    result = M_RES_OPENS;
                    attachInterrupt(digitalPinToInterrupt(REVINPIN), isr_count, RISING);
                    
                    break;
                    
      case M_CLOSE:
                    COMM_DBG.println("M: state close");                                 
                    set_motor(mvalvenr, DIR_CLOSE);
                    delay(50);    // wait for relay
                    //ena_motor(mvalvenr, 1);
  
                    motorstate = M_TURNON;
                    isr_valvenr = mvalvenr;
                    isr_counter=0;
                    cyclecnt = 0;
                    debouncecnt = 0;
                    result = M_RES_CLOSES;
                    attachInterrupt(digitalPinToInterrupt(REVINPIN), isr_count, RISING);
  
                    break;
                    

      case M_TURNON:
                    isr_turning = 1;
                    result = M_RES_TURNING;
                    isr_timer_go = 1;

                    currentbound_low = (int) m_meancurrent * 10 * -2;
                    currentbound_high = (int) m_meancurrent * 10 * 2;

                    meancurrent_mem = 0;
                    meancurrent_cnt = 0;

                    // check if pwm is finished
                    if(isr_timer_fin) {
                      motorstate = M_TURNING;
                      isr_timer_fin = 0;
                      isr_timer_go = 0;
                    }

                    break;

      case M_TURNING:
                    isr_turning = 1;
                    result = M_RES_TURNING;
    
                    if (cmd == CMD_M_STOP) {
                      motorstate = M_STOP;  
                    }
                    cyclecnt++;
                                
                    if(debouncecnt<255) debouncecnt++;
                    if(testcnt<255) testcnt++;

                    if (debouncecnt > 7) {

                      current_mA = (int) (ina219.getCurrent_mA() * 10);

                      analog_current = (int) ((( (int32_t) analogRead(ANINCURRENT) - (int32_t) idlecurrent)* ANINCURRENTGAIN) / 100);
                      analog_current = (int) (((int32_t) analog_current_old * 800 + (int32_t) analog_current * 200) / 1000);
                      analog_current_old = analog_current;

                      if (testcnt > 40) {
                        testcnt = 0;
                        COMM_DBG.print("A0 current ");
                        COMM_DBG.print(analog_current,DEC);
                        COMM_DBG.print(" idle ");
                        COMM_DBG.print(idlecurrent,DEC);
                        COMM_DBG.println(" digits");
                      }


                      if(current_mA < THRESHOLD_UNDERCURRENT && current_mA > -THRESHOLD_UNDERCURRENT) {
                        undercurrcnt++;
                        if (undercurrcnt > TIMEOUT_UNDERCURRENT)
                        {
                          undercurrcnt = 0;
                          motorstate = M_UNDERCURR;
                        }                        
                      }

                      if(current_mA > currentbound_high || current_mA < currentbound_low ||  
                         current_mA > 1000 || current_mA < -1000)         // safety mechanism, limit at +- 100 mA
                      {
                        //motorstate = M_STOP;
                        detachInterrupt(digitalPinToInterrupt(REVINPIN)); 
                        motorstate = M_IDLE;
                        result = M_RES_ENDSTOP;
                        ena_motor(0, 0);
                        isr_turning = 0;
                        MUX_OFF();

                        if (meancurrent_cnt > 0) m_meancurrent = abs(meancurrent_mem) / meancurrent_cnt / 10;
                        else m_meancurrent = 0;
                    
                        //COMM_DBG.print("M: Current: "); COMM_DBG.print(current_mA/10,10); COMM_DBG.println(" mA");
                        #warning fixme
                        COMM_DBG.print("M: Current: "); COMM_DBG.print(current_mA,10); COMM_DBG.println(" mA");
                        COMM_DBG.print("M: Cnt:     "); COMM_DBG.println(isr_counter, DEC);                                 
                        COMM_DBG.println("M: Autostop, reached end stop");
                      }
                      else 
                      {
                        //COMM_DBG.print("M: Current: "); COMM_DBG.print(current_mA/10, 10); COMM_DBG.println(" mA");
                      }
                      //cyclecnt=0;

                      // testmode
                      // output position and current
                      if(testmode) {
                        COMM_DBG.print("tm;"); COMM_DBG.print(current_mA,10); COMM_DBG.print(";"); COMM_DBG.println(isr_counter);

                      }
                    }

                    if (debouncecnt > 50 && cyclecnt > 50) {
                      cyclecnt=0;
                      //COMM_DBG.print("M: Current: "); COMM_DBG.print(current_mA/10, 10); COMM_DBG.println(" mA");
                      #warning fixme
                      COMM_DBG.print("M: Current: "); COMM_DBG.print(current_mA, 10); COMM_DBG.println(" mA");
                      meancurrent_mem += current_mA;
                      meancurrent_cnt++;
                    }
                    break;
                      
      case M_STOP:         
                    //detachInterrupt(digitalPinToInterrupt(REVINPIN));
                    COMM_DBG.println("M: state stop");    
                                                      
                    ena_motor(0, 0);
                    isr_turning = 0;
                    MUX_OFF();
                    
                    COMM_DBG.print("M: Cnt: ");  
                    COMM_DBG.println(isr_counter, DEC);                                 
  
                    motorstate = M_IDLE;
                    result = M_RES_STOP;                  
                    
                    break;

      case M_STOP_ISR: 
                    COMM_DBG.println("M: target stop");  
                    motorstate = M_STOP;
                    result = M_RES_STOP_ISR;  
                    break;

      case M_UNDERCURR:         
                    detachInterrupt(digitalPinToInterrupt(REVINPIN));
                    COMM_DBG.println("M: state undercurrent detected, stopped");    
                                                      
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

  //}
  return result;
}


// sets direction and mux relay
void set_motor (int smvalvenr, int dir) {

  if (smvalvenr % 2) { MUX_OFF(); }
  else { MUX_ON(); }
  
  if (dir == DIR_OPEN) { DIR_OFF(); } 
  else { DIR_ON(); }
}


// enables motor 
void ena_motor (int emvalvenr, int state) {

  if (state == 0) { 
    ENA0_OFF(); ENA1_OFF(); ENA2_OFF(); ENA3_OFF(); ENA4_OFF(); ENA5_OFF(); 
  }
  else {
    if (emvalvenr == 0 || emvalvenr == 1) { ENA0_ON(); }
    else if (emvalvenr == 2 || emvalvenr == 3) { ENA1_ON(); }
    else if (emvalvenr == 4 || emvalvenr == 5) { ENA2_ON(); }
    else if (emvalvenr == 6 || emvalvenr == 7) { ENA3_ON(); }
    else if (emvalvenr == 8 || emvalvenr == 9) { ENA4_ON(); }
    else if (emvalvenr == 10 || emvalvenr == 11) { ENA5_ON(); }
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
  isr_turning = 0;
  ena_motor(0, 0);  
  motorcycle(0, CMD_M_STOP_ISR);  
}


// returns state of application  state machine
enum ASTATE appgetstate () {
  return appstate;
}


int16_t appsetaction(char cmd, unsigned int valveindex, byte posdelta) {

  if(appstate == A_IDLE) {
    valvenr = (int) valveindex;
    command = cmd;
    poschangecmd = posdelta;
    return 0;
  }
  else return -1;

}


void TimerHandler0()
{
  static unsigned int rampcnt = 0;      // makes duty cycle of pwm
  static unsigned int cnt = 0;          // makes periode time of pwm

  if(isr_timer_go && !isr_timer_fin) {
    
    if(cnt<=rampcnt) {        
      //ENA0_ON();
      ena_motor(isr_valvenr, 1);
    }
    else {
      //ENA0_OFF();
      ena_motor(isr_valvenr, 0);
    }

    // do counters
    if(cnt<16) cnt++;
    else {
      cnt = 0;  
      
      if(rampcnt<16) rampcnt++;
      else {    // finish & reset
        isr_timer_fin = 1;
        cnt = 0;
        rampcnt = 0;
      }
    }
  }
}