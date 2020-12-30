// defines used hardware

// status LED 
#define LED PC13

// valve MUX relay
#define CTRL_MUX PB1

// L293 enable pins
#define CTRL_ENA0 PA1
#define CTRL_ENA1 PA5
#define CTRL_ENA2 PA6
#define CTRL_ENA3 PA7
#define CTRL_ENA4 PB3
#define CTRL_ENA5 PA15

// L293 direction pin
#define CTRL_DIRECTION PB0 


// macros  
#define ENA0_ON()   { digitalWrite(CTRL_ENA0, HIGH); }
#define ENA0_OFF()   { digitalWrite(CTRL_ENA0, LOW); }
#define ENA1_ON()   { digitalWrite(CTRL_ENA0, HIGH); }
#define ENA1_OFF()   { digitalWrite(CTRL_ENA0, LOW); }
#define ENA2_ON()   { digitalWrite(CTRL_ENA0, HIGH); }
#define ENA2_OFF()   { digitalWrite(CTRL_ENA0, LOW); }

#define DIR_ON()   { digitalWrite(CTRL_DIRECTION, HIGH); }
#define DIR_OFF()   { digitalWrite(CTRL_DIRECTION, LOW); }

#define MUX_ON()   { digitalWrite(CTRL_MUX, HIGH); }
#define MUX_OFF()   { digitalWrite(CTRL_MUX, LOW); }
