/**HEADER*******************************************************************
  project : VdMot Controller
  author : Lenti84
  Comments:
  Version :
  Modifcations :
***************************************************************************
*
* THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESSED OR
* IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
* OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
* IN NO EVENT SHALL THE DEVELOPER OR ANY CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
* INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
* (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
* SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
* HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
* STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
* IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
* THE POSSIBILITY OF SUCH DAMAGE.
*
**************************************************************************
  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License.
  See the GNU General Public License for more details.
  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
  Copyright (C) 2021 Lenti84  https://github.com/Lenti84/VdMot_Controller
*END************************************************************************/

#include <Arduino.h>
#include <Wire.h>
#include "motor.h"
#include "hardware.h"
#include "STM32TimerInterrupt.h"      // library https://github.com/khoih-prog/STM32_TimerInterrupt
#include "terminal.h"
#include "app.h"
#include "eeprom.h"
#include "temperature.h"



//#define COMM_DBG				Serial3		// serial port for debugging
#define COMM_DBG				Serial6		// serial port for debugging


#define DIR_CLOSE      (int)  1
#define DIR_OPEN       (int)  0 

#define TIMER0_INTERVAL_MS        1

#define TIMEOUT_NORMALCURRENT    120*100      // 120 seconds timeout with 10 ms cycle time
#define TIMEOUT_OVERCURRENT      5            // cycles of "byte motorcycle (byte valvenr, byte cmd)"
#define TIMEOUT_UNDERCURRENT     50           // cycles of "byte motorcycle (byte valvenr, byte cmd)"
#define TIMEOUT_UNDERCURRENTTEST 20           // cycles of "byte motorcycle (byte valvenr, byte cmd)"
#define THRESHOLD_UNDERCURRENT   20           // threshold for detecting undercurrent in 1/10 mA


// commands motor statemachine
#define CMD_M_OPEN      'o'
#define CMD_M_CLOSE     'c'
#define CMD_M_STOP      's'
#define CMD_M_STOP_ISR  'x'
#define CMD_M_NOTHING   'n'
#define CMD_M_TEST      't'


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
#define M_RES_TEST        10
#define M_RES_ERROR       11


void set_motor (int smvalvenr, int dir);
void ena_motor (int emvalvenr, int state);
void isr_count ();
void callback_motorstop ();
byte motorcycle (int mvalvenr, byte cmd);
byte appcycle(byte cmd, byte valvenr);

void TimerHandler0();

enum ASTATE valvestate;

// Init STM32 timer TIM1
STM32Timer ITimer0(TIM1);

//int counter;
//unsigned long time;
int current_mA = 0;                     // current valve motor in 1/10 mA

int analog_current = 0;                 // current valve motor in 1/10 mA read by analog pin
int analog_current_old = 0;             // filter

// unsigned int idlecurrent = 2048;        // idle current adc value (digits)
// unsigned int idlecurrent_old = 2048;    // filter

volatile unsigned int isr_counter;      // ISR var for counting revolutions
volatile byte         isr_turning;      // ISR var for state of motor
volatile unsigned int isr_target;       // ISR var for valve target count
volatile int          isr_valvenr;      // ISR var for valve nr
volatile byte         isr_timer_go;     // ISR var for timer pwm start
volatile byte         isr_timer_fin;    // ISR var for timer pwm finished


valvemotor myvalvemots[ACTUATOR_COUNT];


char command;
int valvenr;
byte poschangecmd;
unsigned int m_meancurrent;           // mean current in mA

uint8_t currentbound_low_fac = 20;     // lower current limit factor for detection of end stop
uint8_t currentbound_high_fac = 20;    // upper current limit factor for detection of end stop
uint8_t startOnPower = 50;

//volatile uint32_t revcounter;

// call from setup function in main
byte valve_setup () {

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

  // external interrupt for counting revs
  //attachInterrupt(digitalPinToInterrupt(REVINPIN),isr_counter,RISING);


 if ((eep_content.currentbound_low_fac>=10) && (eep_content.currentbound_low_fac<=50))
    currentbound_low_fac = eep_content.currentbound_low_fac;
  if ((eep_content.currentbound_high_fac>=10) && (eep_content.currentbound_high_fac<=50))
    currentbound_high_fac = eep_content.currentbound_high_fac;
  if (eep_content.startOnPower<=100) startOnPower=eep_content.startOnPower; else startOnPower=50;
  #ifdef motDebug
    COMM_DBG.print("Current end stop factor low: ");
    COMM_DBG.println((float) (currentbound_low_fac)/10,DEC);
    COMM_DBG.print("Current end stop factor high: ");
    COMM_DBG.println((float) (currentbound_high_fac)/10,DEC);
  #endif

// init valve data
  for (unsigned int x = 0;x<ACTUATOR_COUNT;x++) {
    myvalvemots[x].actual_position = startOnPower;
    myvalvemots[x].target_position = startOnPower;
    myvalvemots[x].meancurrent = 20;       // 20 mA
    myvalvemots[x].scaler = 89;
  }

  valvestate = A_INIT;

  // Interval in microsecs
  if (ITimer0.attachInterruptInterval(TIMER0_INTERVAL_MS * 1000, TimerHandler0))
  {
    #ifdef motDebug 
        COMM_DBG.print(F("Starting ITimer0 OK, millis() = ")); COMM_DBG.println(millis());
    #endif
  }
  else {
    #ifdef motDebug
      COMM_DBG.println(F("Can't set ITimer0. Select another freq. or timer"));
    #endif
  }

 

  return 0;
}


byte valve_loop () {
  
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

  static int waittimer = 0;
  static int psuofftimer = 0;
  static int locktimer = 0;

  static enum ASTATE oldvalvestate;

  if(waittimer) waittimer--;

  oldvalvestate = valvestate;

  switch (valvestate) {
    case A_INIT:  
                  if ( motorcycle(0, CMD_M_NOTHING) == M_RES_IDLE ) {                    
                     #ifdef motDebug
                        COMM_DBG.println("A: init ready");
                     #endif
                     valvestate = A_IDLE;
                     
                     //valveindex = 0;                     
                     //myvalves[valveindex].scaler = 130;
                     //myvalves[valveindex].target_position = 0;    

                     valveindex = 255;
                    
                  }
                  break;

    case A_IDLE:  
                  valveindex = valvenr;
                  if (command == CMD_A_OPEN) {
                    #ifdef motDebug                    
                      COMM_DBG.print("A: cmd open for valve ");                  
                      COMM_DBG.println(valveindex, 10);                    
                    #endif
                    valvestate = A_OPEN1;
                    PSU_ON();
                    waittimer = 50;
                    psuofftimer = 0;
                    pos_change = poschangecmd;
                  }
                  else if (command == CMD_A_CLOSE) {
                    #ifdef motDebug
                      COMM_DBG.print("A: cmd close for valve ");                  
                      COMM_DBG.println(valveindex, 10);                                        
                    #endif
                    valvestate = A_CLOSE1;  
                    PSU_ON();
                    waittimer = 50;
                    psuofftimer = 0;
                    pos_change = poschangecmd;
                  }
                  else if (command == CMD_A_OPEN_END) {  
                    #ifdef motDebug                  
                      COMM_DBG.print("A: cmd open to endstop for valve ");                  
                      COMM_DBG.println(valveindex, 10);                    
                    #endif
                    valvestate = A_OPEN1;
                    PSU_ON();
                    waittimer = 50;
                    psuofftimer = 0;
                    pos_change = 255;
                  }
                  else if (command == CMD_A_CLOSE_END) {   
                    #ifdef motDebug                 
                      COMM_DBG.print("A: cmd close to endstop for valve ");                  
                      COMM_DBG.println(valveindex, 10);                    
                    #endif
                    valvestate = A_CLOSE1;
                    PSU_ON();
                    waittimer = 50;
                    psuofftimer = 0;
                    pos_change = 255;
                  }
                  else if (command == CMD_A_LEARN) {
                    #ifdef motDebug                    
                      COMM_DBG.print("A: cmd learn for valve ");                  
                      COMM_DBG.println(valveindex, 10);                    
                    #endif
                    valvestate = A_LEARN1;
                    PSU_ON(); 
                    psuofftimer = 0;
                    waittimer = 50; 
                  }
                  else if (command == CMD_A_TEST) { 
                    #ifdef motDebug                   
                      COMM_DBG.print("A: cmd test valve ");                  
                      COMM_DBG.println(valveindex, 10);  
                    #endif                  
                    valvestate = A_TEST;
                    PSU_ON();
                    waittimer = 100;
                    psuofftimer = 0;
                  }
                  else {
                    valvestate = A_IDLE;

                    // switch off PSU after some inactive (idle) time
                    psuofftimer++;
                    if(psuofftimer > 1500) {      // 15 s
                      psuofftimer = 0;
                      PSU_OFF();
                      MUX_OFF();
                    }  
                  }           

                  // decrement learning counter
                  if(valvestate == A_OPEN1 || valvestate == A_CLOSE1) {
                    if(myvalves[valveindex].learn_movements) myvalves[valveindex].learn_movements--;
                  }

                  // pause temperature measurement to avoid ADC interference
                  if(valvestate != A_IDLE) {
                    temp_command(TEMP_CMD_LOCK);
                    locktimer = 0;
                  }
                  else {
                    if (locktimer<50) locktimer++;
                    else temp_command(TEMP_CMD_UNLOCK);                      
                  }            
               
                  // // measure idle current for offset (drift) compensation
                  // if (idlecurrenttimer < 10) 
                  // {
                  //   idlecurrenttimer++;
                  // }
                  // else
                  // {                      
                  //   idlecurrenttimer = 0;
                  //   idlecurrent = (unsigned int) ((uint32_t) (analogRead(ANINREFHALF) * 200 + (uint32_t) idlecurrent_old * 800) / 1000);
                  //   idlecurrent_old = idlecurrent;
                  // }
                 

                  command = '\0';       // clear command for next loop call, prevents reevaluating
                  break;

    case A_OPEN1:  // start valve opening
                  if (!waittimer) {
                    if(pos_change==255) isr_target = 65535;
                    else isr_target = myvalvemots[valveindex].scaler * pos_change;
                    m_meancurrent = myvalvemots[valveindex].meancurrent;
                    
                    if (motorcycle (valveindex, CMD_M_OPEN) == M_RES_OPENS) {
                      #ifdef motDebug
                        COMM_DBG.print("A: begin opening by ");                  
                        COMM_DBG.println(pos_change);
                      #endif
                      myvalvemots[valveindex].status = VLV_STATE_OPENING;                                                            
                      valvestate = A_OPEN2;
                      isr_counter=0;
                    }
                    else {
                      #ifdef motDebug
                        COMM_DBG.print("A: cant open valve");                  
                      #endif
                      valvestate = A_IDLE;
                    }
                  }
                  break;
  
    case A_OPEN2:  // wait for finish opening
                  temp = motorcycle (valveindex, 0);
                  if (temp == M_RES_STOP) {
                    #ifdef motDebug
                      COMM_DBG.println("A: opened valve"); 
                    #endif                 
                    valvestate = A_IDLE;
                    myvalvemots[valveindex].actual_position += pos_change;
                    myvalvemots[valveindex].status = VLV_STATE_IDLE; 
                    #ifdef motDebug                   
                      COMM_DBG.print("A: new position "); COMM_DBG.println(myvalvemots[valveindex].actual_position);
                    #endif
                  }
                  else if (temp == M_RES_NOCURRENT) {
                    #ifdef motDebug
                      COMM_DBG.println("A: undercurrent");
                    #endif
                    myvalvemots[valveindex].status = VLV_STATE_OPENCIR;
                    myvalvemots[valveindex].actual_position = myvalvemots[valveindex].target_position;
                    valvestate = A_IDLE;
                    isr_counter=0;
                  } 
                  else if (temp == M_RES_ENDSTOP) {
                    #ifdef motDebug
                      COMM_DBG.println("A: opened valve to end stop");
                    #endif
                    valvestate = A_IDLE;
                    myvalvemots[valveindex].status = VLV_STATE_IDLE;
                    myvalvemots[valveindex].actual_position = 100;
                    #ifdef motDebug
                      COMM_DBG.print("A: new position "); COMM_DBG.println(myvalvemots[valveindex].actual_position);
                    #endif
                  }
                  else if (temp == M_RES_ERROR) {
                    #ifdef motDebug
                      COMM_DBG.println("A: opening valve failed, timeout");
                    #endif
                    valvestate = A_IDLE;
                    myvalvemots[valveindex].status = VLV_STATE_BLOCKS;
                    myvalvemots[valveindex].actual_position = myvalvemots[valveindex].target_position;
                  }                                     
                  break;

    case A_CLOSE1:  // start valve closing
                  if (!waittimer) {
                    if(pos_change==255) isr_target = 65535;
                    else isr_target = myvalvemots[valveindex].scaler * pos_change;
                    m_meancurrent = myvalvemots[valveindex].meancurrent;
                    
                    if (motorcycle (valveindex, CMD_M_CLOSE) == M_RES_CLOSES) {
                      #ifdef motDebug
                        COMM_DBG.print("A: begin closing by ");                  
                        COMM_DBG.println(pos_change);       
                      #endif
                      myvalvemots[valveindex].status = VLV_STATE_CLOSING;                                                     
                      valvestate = A_CLOSE2;
                      isr_counter=0;
                    }   
                    else {
                      #ifdef motDebug
                        COMM_DBG.print("A: cant close valve");                  
                      #endif
                      valvestate = A_IDLE;
                    }
                  }
                  break;

    case A_CLOSE2:  // wait for finish closing
                  temp = motorcycle (valveindex, 0);
                  if (temp == M_RES_STOP) {
                    #ifdef motDebug
                      COMM_DBG.println("A: closed valve");
                    #endif
                    valvestate = A_IDLE;
                    myvalvemots[valveindex].status = VLV_STATE_IDLE;
                    myvalvemots[valveindex].actual_position -= pos_change; 
                    #ifdef motDebug
                      COMM_DBG.print("A: new position "); COMM_DBG.println(myvalvemots[valveindex].actual_position);                   
                    #endif
                  }
                  else if (temp == M_RES_NOCURRENT) {
                    #ifdef motDebug
                      COMM_DBG.println("A: undercurrent");
                    #endif
                    myvalvemots[valveindex].status = VLV_STATE_OPENCIR;
                    myvalvemots[valveindex].actual_position = myvalvemots[valveindex].target_position;
                    valvestate = A_IDLE;
                    isr_counter=0;
                  } 
                  else if (temp == M_RES_ENDSTOP) {
                    #ifdef motDebug
                      COMM_DBG.println("A: closed valve to end stop");
                    #endif
                    valvestate = A_IDLE;
                    myvalvemots[valveindex].status = VLV_STATE_IDLE;
                    myvalvemots[valveindex].actual_position = 0;
                    #ifdef motDebug
                      COMM_DBG.print("A: new position "); COMM_DBG.println(myvalvemots[valveindex].actual_position);
                    #endif
                  }            
                  else if (temp == M_RES_ERROR) {
                    #ifdef motDebug
                      COMM_DBG.println("A: closing valve failed, timeout");
                    #endif
                    valvestate = A_IDLE;
                    myvalvemots[valveindex].status = VLV_STATE_BLOCKS;
                    myvalvemots[valveindex].actual_position = myvalvemots[valveindex].target_position;
                  }                   
                  break;                  
    
    case A_LEARN1:  // prepare learning  
                  #ifdef motDebug        
                    COMM_DBG.println("A: try learning valve");                  
                  #endif
                  // first: closing completely
                  motorcycle (valveindex, CMD_M_CLOSE);
                  myvalvemots[valveindex].status = VLV_STATE_CLOSING;
                  valvestate = A_LEARN2;
                  waittimer = 20;
                  m_meancurrent = myvalvemots[valveindex].meancurrent;
                  isr_target = 65535;       // max value to disable stopping
                
                  break;

    case A_LEARN2:  // goto start position
                  if (!waittimer) {
                    // waiting for closed valve as start position
                    temp = motorcycle (valveindex, 0);
                    if (temp == M_RES_ENDSTOP) {
                      #ifdef motDebug
                        COMM_DBG.println("A: closed valve before learning, now opening");                  
                      #endif
                      // second: opening completely and count rotations
                      isr_counter=0;
                      myvalvemots[valveindex].status = VLV_STATE_OPENING;
                      motorcycle (valveindex, CMD_M_OPEN);
                      isr_target = 65535;       // max value to disable stopping
                      valvestate = A_LEARN3;
                      m_meancurrent = myvalvemots[valveindex].meancurrent;
                      waittimer = 20;
                    }
                    else if (temp == M_RES_NOCURRENT) {
                      #ifdef motDebug
                        COMM_DBG.println("A: undercurrent");
                      #endif
                      myvalvemots[valveindex].status = VLV_STATE_OPENCIR;
                      myvalvemots[valveindex].target_position = myvalvemots[valveindex].actual_position;
                      valvestate = A_IDLE;
                      isr_counter=0;
                    }
                    else if (temp == M_RES_ERROR) {
                      #ifdef motDebug
                        COMM_DBG.println("A: closing valve failed, timeout");
                      #endif
                      valvestate = A_IDLE;
                      myvalvemots[valveindex].status = VLV_STATE_BLOCKS;
                      myvalvemots[valveindex].actual_position = myvalvemots[valveindex].target_position;
                    }           
                  }               
                  break;
                  
    case A_LEARN3:  // learning opening way
                  if (!waittimer) {
                    // waiting for opened valve
                    temp = motorcycle (valveindex, 0);
                    if (temp == M_RES_ENDSTOP) {
                      #ifdef motDebug
                        COMM_DBG.println("A: opened valve for learning, now closing again");                  
                      #endif
                      opening_count = isr_counter;   
                      if(m_meancurrent > 0) {
                        myvalvemots[valveindex].meancurrent = m_meancurrent;
                        #ifdef motDebug
                          COMM_DBG.print("A: learned mean current = "); 
                          COMM_DBG.println(myvalvemots[valveindex].meancurrent);
                        #endif
                      }                 
                      // third: closing completely and count rotations
                      isr_counter=0;
                      myvalvemots[valveindex].status = VLV_STATE_CLOSING;
                      motorcycle (valveindex, CMD_M_CLOSE);
                      isr_target = 65535;       // max value to disable stopping
                      m_meancurrent = 20;
                      valvestate = A_LEARN4;
                      waittimer = 20;
                    }          
                    else if (temp == M_RES_NOCURRENT) {
                      #ifdef motDebug
                        COMM_DBG.println("A: undercurrent");
                      #endif
                      myvalvemots[valveindex].status = VLV_STATE_OPENCIR;
                      myvalvemots[valveindex].target_position = myvalvemots[valveindex].actual_position;
                      valvestate = A_IDLE;
                      isr_counter=0;
                    }
                    else if (temp == M_RES_ERROR) {
                      #ifdef motDebug
                        COMM_DBG.println("A: opening valve failed, timeout");
                      #endif
                      valvestate = A_IDLE;
                      myvalvemots[valveindex].status = VLV_STATE_BLOCKS;
                      myvalvemots[valveindex].actual_position = myvalvemots[valveindex].target_position;
                    }                
                  }              
                  break;
                  
    case A_LEARN4:  // learning closing way
                  if (!waittimer) {
                    // waiting for closed valve again
                    temp = motorcycle (valveindex, 0);
                    if (temp == M_RES_ENDSTOP) { 
                      #ifdef motDebug 
                        COMM_DBG.println("A: closed valve for learning");                  
                      #endif
                      closing_count = isr_counter;  

                      if(m_meancurrent > 0) {
                        #ifdef motDebug
                          COMM_DBG.print("A: learned mean current = "); 
                          COMM_DBG.println(m_meancurrent);
                        #endif
                        myvalvemots[valveindex].meancurrent = (myvalvemots[valveindex].meancurrent + m_meancurrent) / 2;                      
                      }                      
                                          
                      deadzone_count = closing_count - opening_count;
                      scaler = opening_count / 100;
                      #ifdef motDebug
                        COMM_DBG.print("A: learned closing_count = "); COMM_DBG.println(closing_count);
                        COMM_DBG.print("A: learned opening_count = "); COMM_DBG.println(opening_count);
                        COMM_DBG.print("A: learned deadzone_count = "); COMM_DBG.println(deadzone_count);
                        COMM_DBG.print("A: learned scaler = "); COMM_DBG.println(scaler);
                        COMM_DBG.print("A: learned mean current = "); COMM_DBG.println(myvalvemots[valveindex].meancurrent);
                      #endif
                      myvalvemots[valveindex].closing_count = closing_count;
                      myvalvemots[valveindex].opening_count = opening_count;
                      myvalvemots[valveindex].deadzone_count = deadzone_count;
                      myvalvemots[valveindex].scaler = scaler;
                      myvalvemots[valveindex].actual_position = 0;    // because valve was closed completely                    
                      myvalvemots[valveindex].status = VLV_STATE_IDLE;

                      valveindex = 255;                                                                                  
                      valvestate = A_IDLE;
                    }    
                    else if (temp == M_RES_NOCURRENT) {
                      #ifdef motDebug
                        COMM_DBG.println("A: undercurrent");
                      #endif
                      myvalvemots[valveindex].status = VLV_STATE_OPENCIR;
                      myvalvemots[valveindex].target_position = myvalvemots[valveindex].actual_position;
                      
                      valveindex = 255;
                      valvestate = A_IDLE;
                      isr_counter=0;
                    }     
                    else if (temp == M_RES_ERROR) {
                      #ifdef motDebug
                        COMM_DBG.println("A: closing valve failed, timeout");
                      #endif
                      valvestate = A_IDLE;
                      myvalvemots[valveindex].status = VLV_STATE_BLOCKS;
                      myvalvemots[valveindex].actual_position = myvalvemots[valveindex].target_position;
                    }        
                  }           
                  break;

     case A_TEST:  // test if a valve is connected                  
                  if (!waittimer) {
                    
                    temp = motorcycle (valveindex, CMD_M_TEST);
                    if (temp != M_RES_TEST) {  
                      #ifdef motDebug
                        COMM_DBG.print("A: test valve ");
                        COMM_DBG.print(valveindex, DEC);                  
                      #endif
                      if(temp == M_RES_NOCURRENT) {
                        #ifdef motDebug
                          COMM_DBG.println(" --> undercurrent, no valve");
                        #endif
                        myvalvemots[valveindex].status = VLV_STATE_OPENCIR;
                      }
                      else {
                        #ifdef motDebug
                          COMM_DBG.println(" --> valve present");
                        #endif
                        myvalvemots[valveindex].status = VLV_STATE_PRESENT;
                      }

                      valvestate = A_IDLE;                    
                    }
                  }
                  break;

                  
    default:      valvestate = A_IDLE;
                  break;  
  }
  result = 0;

  return result;
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
  #define M_TESTPREP  9
  #define M_TEST      10

  static byte motorstate = M_INIT;

  static int cyclecnt = 0;
  static int debouncecnt = 0;
  static int testcnt = 0;
  static int undercurrcnt = 0;
  static int overcurrcnt = 0;
  static int normalcurrcnt = 0;

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
                    else if (cmd == CMD_M_TEST) {
                      motorstate = M_TESTPREP;
                      result = M_RES_TEST;  
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
                    overcurrcnt = 0;
                    
                    break;
      
      case M_OPEN:  
                    #ifdef motDebug
                      COMM_DBG.println("M: state open");                                  
                    #endif
                    set_motor(mvalvenr, DIR_OPEN);
                    //digitalWrite(POWER_ENA, 0);   // enable PSU for vlaves   
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
                    #ifdef motDebug
                      COMM_DBG.println("M: state close");                                 
                    #endif
                    set_motor(mvalvenr, DIR_CLOSE);
                    //digitalWrite(POWER_ENA, 0);   // enable PSU for vlaves
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

                    currentbound_low = (int) (m_meancurrent * -1 * currentbound_low_fac); //10 * -3;
                    currentbound_high = (int) (m_meancurrent * currentbound_high_fac); //10 * 3;

                    meancurrent_mem = 0;
                    meancurrent_cnt = 0;

                    analog_current_old = 0;
                    undercurrcnt = 0;
                    overcurrcnt = 0;
                    normalcurrcnt = 0;

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

                    if (debouncecnt > 10) {

                      // calc current in 1/10 mA
                      analog_current = (int) ((( (int32_t) analogRead(ANINCURRENT) - (int32_t) analogRead(ANINREFHALF)) * ANINCURRENTGAIN) / 100);
                      // COMM_DBG.print(analog_current,DEC);
                      // COMM_DBG.print(" ");
                      // COMM_DBG.print(idlecurrent,DEC);
                      // COMM_DBG.println(" analog_current");

                      // filter
                      analog_current = (int) (((int32_t) analog_current_old * 800 + (int32_t) analog_current * 200) / 1000);
                      analog_current_old = analog_current;

                      current_mA = analog_current;

                      // if (testcnt > 40) {
                      //   testcnt = 0;
                      //   COMM_DBG.print("A0 current ");
                      //   COMM_DBG.print(analog_current,DEC);
                      //   COMM_DBG.print(" idle ");
                      //   COMM_DBG.print(idlecurrent,DEC);
                      //   COMM_DBG.println(" digits");
                      // }

                      // under current detection
                      if(current_mA < THRESHOLD_UNDERCURRENT && current_mA > -THRESHOLD_UNDERCURRENT) 
                      {
                        undercurrcnt++;
                        if (undercurrcnt > TIMEOUT_UNDERCURRENT)
                        {
                          undercurrcnt = 0;
                          motorstate = M_UNDERCURR;
                        }                        
                      }

                      // overcurrent detection
                      if(current_mA > currentbound_high || current_mA < currentbound_low ||  
                         current_mA > 1000 || current_mA < -1000)         // safety mechanism, limit at +- 100 mA
                      {
                        overcurrcnt++;
                        if (overcurrcnt > TIMEOUT_OVERCURRENT)
                        {
                          //motorstate = M_STOP;                          
                          detachInterrupt(digitalPinToInterrupt(REVINPIN)); 
                          overcurrcnt = 0;
                          motorstate = M_IDLE;
                          result = M_RES_ENDSTOP;
                          ena_motor(0, 0);
                          isr_turning = 0;
                          //MUX_OFF();

                          if (meancurrent_cnt > 0) m_meancurrent = abs(meancurrent_mem) / meancurrent_cnt / 10;
                          else m_meancurrent = 0;
                          #ifdef motDebug
                            COMM_DBG.print("M: Current: "); COMM_DBG.print(current_mA/10,10); COMM_DBG.println(" mA");
                            //#warning fixme
                            //COMM_DBG.print("M: Current: "); COMM_DBG.print(current_mA,10); COMM_DBG.println(" 1/10 mA");
                            COMM_DBG.print("M: Cnt:     "); COMM_DBG.println(isr_counter, DEC);                                 
                            COMM_DBG.println("M: Autostop, reached end stop");
                          #endif
                        }
                      }

                      // normal turning
                      else  {
                        //COMM_DBG.print("M: Current: "); COMM_DBG.print(current_mA/10, 10); COMM_DBG.println(" mA");
                        normalcurrcnt++;
                        if(normalcurrcnt > TIMEOUT_NORMALCURRENT) {
                          #ifdef motDebug          
                            COMM_DBG.println("test: normal turning timeout");            
                          #endif
                          
                          detachInterrupt(digitalPinToInterrupt(REVINPIN)); 
                          normalcurrcnt = 0;
                          motorstate = M_IDLE;
                          result = M_RES_ERROR;
                          ena_motor(0, 0);
                          isr_turning = 0;    
                        }                
                      }
                      //cyclecnt=0;

                      

                      // testmode
                      // output position and current
                      if(testmode) {
                        #ifdef motDebug
                          COMM_DBG.print("tm;"); 
                          COMM_DBG.print(current_mA,10); 
                          COMM_DBG.print(";"); 
                          COMM_DBG.println(isr_counter);
                        #endif

                      }
                    }

                    if (debouncecnt > 50 && cyclecnt > 50) {
                      cyclecnt=0;
                      #ifdef motDebug
                        COMM_DBG.print("M: Current: "); COMM_DBG.print(current_mA/10, 10); COMM_DBG.println(" mA");
                        //#warning fixme
                        //COMM_DBG.print("M: Current: "); COMM_DBG.print(current_mA, 10); COMM_DBG.println(" 1/10 mA");
                      #endif
                      meancurrent_mem += current_mA;
                      meancurrent_cnt++;
                    }
                    break;
                      
      case M_STOP:         
                    //detachInterrupt(digitalPinToInterrupt(REVINPIN));
                    #ifdef motDebug
                      COMM_DBG.println("M: state stop");    
                    #endif                                  
                    ena_motor(0, 0);
                    //digitalWrite(POWER_ENA, 1);   // disable PSU for vlaves   
                    isr_turning = 0;
                    //MUX_OFF();
                    #ifdef motDebug
                      COMM_DBG.print("M: Cnt: ");  
                      COMM_DBG.println(isr_counter, DEC);                                 
                    #endif
                    motorstate = M_IDLE;
                    result = M_RES_STOP;                  
                    
                    break;

      case M_STOP_ISR: 
                    #ifdef motDebug
                      COMM_DBG.println("M: target stop");  
                    #endif
                    motorstate = M_STOP;
                    result = M_RES_STOP_ISR;  
                    break;

      case M_UNDERCURR:         
                    detachInterrupt(digitalPinToInterrupt(REVINPIN));
                    #ifdef motDebug
                      COMM_DBG.println("M: state undercurrent detected, stopped");    
                    #endif                                  
                    ena_motor(0, 0);
                    //digitalWrite(POWER_ENA, 1);   // disable PSU for vlaves 
                    isr_turning = 0;
                    //MUX_OFF();
                    
                    motorstate = M_IDLE;
                    result = M_RES_NOCURRENT;                  
                    
                    break;

      case M_TESTPREP:
                    #ifdef motDebug
                      COMM_DBG.println("M: test prep");                                  
                    #endif
                    set_motor(mvalvenr, DIR_OPEN); 
                    ena_motor(mvalvenr, 1);                 
                
                    cyclecnt = 0;
                    undercurrcnt = 0;
                    normalcurrcnt = 0;
                    overcurrcnt=0;
                    debouncecnt = 0;

                    analog_current_old = 0;
                    analog_current = 0;

                    motorstate = M_TEST;
                    result = M_RES_TEST;                    

                    break;

      case M_TEST:
                    #ifdef motDebug
                      COMM_DBG.print("M: testing"); 
                    #endif
                    result = M_RES_TEST;
    
                    // calc current in 1/10 mA
                    analog_current = (int) ((( (int32_t) analogRead(ANINCURRENT) - (int32_t) analogRead(ANINREFHALF)) * ANINCURRENTGAIN) / 100);

                    analog_current = (int) (((int32_t) analog_current_old * 800 + (int32_t) analog_current * 200) / 1000);
                    analog_current_old = analog_current;

                    current_mA = analog_current;
                    #ifdef motDebug
                      COMM_DBG.print(" - current: ");
                      COMM_DBG.println (analog_current,DEC);
                    #endif
                    if(debouncecnt<255) debouncecnt++;

                    if(debouncecnt>7) {

                      // under current detection
                      if(current_mA < THRESHOLD_UNDERCURRENT && current_mA > -THRESHOLD_UNDERCURRENT) 
                      {
                        undercurrcnt++;
                        if (undercurrcnt > TIMEOUT_UNDERCURRENTTEST)
                        {
                          #ifdef motDebug
                            COMM_DBG.println("test: undercurrent!");
                          #endif
                          undercurrcnt = 0;
                          motorstate = M_IDLE;
                          result = M_RES_NOCURRENT;
                          ena_motor(0, 0);
                        }                        
                      }

                      // overcurrent detection
                      else if(current_mA > currentbound_high || current_mA < currentbound_low ||  
                        current_mA > 1000 || current_mA < -1000)         // safety mechanism, limit at +- 100 mA
                      {
                        overcurrcnt++;
                        if (overcurrcnt > TIMEOUT_OVERCURRENT)
                        {
                          #ifdef motDebug
                            COMM_DBG.println("test: overcurrent!");
                          #endif
                          overcurrcnt = 0;
                          motorstate = M_IDLE;
                          result = M_RES_ENDSTOP;
                          ena_motor(0, 0);
                        
                        }
                      }

                      // normal turning
                      else  {
                        normalcurrcnt++;
                        if(normalcurrcnt > 5) {
                          #ifdef motDebug          
                            COMM_DBG.println("test: normal turning!");            
                          #endif
                          motorstate = M_IDLE;
                          result = M_RES_OPENS;
                          ena_motor(0, 0);      
                        }                
                      }
                    }

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
  delay(50); // wait for relay settling
  
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
enum ASTATE valve_getstate () {
  return valvestate;
}


int16_t appsetaction(char cmd, unsigned int valveindex, byte posdelta) {

  if(valvestate == A_IDLE && valveindex < ACTUATOR_COUNT) {
    valvenr = (int) valveindex;
    command = cmd;
    poschangecmd = posdelta;
    return 0;
  }
  else {
    #ifdef motDebug          
      COMM_DBG.print("command rejected, state: ");
      COMM_DBG.println(valvestate, DEC);            
    #endif
    return -1;
  }
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