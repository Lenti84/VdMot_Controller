/*
 
*/

//unsigned char motorcycle (unsigned char valvenr, unsigned char position);

#define ACTUATOR_COUNT  12

#define CMD_A_OPEN      'o'
#define CMD_A_CLOSE     'c'
#define CMD_A_LEARN     'l'
#define CMD_A_TARGET    't'

#define VLV_STATE_IDLE      (byte) 0x01
#define VLV_STATE_OPENING   (byte) 0x02
#define VLV_STATE_CLOSING   (byte) 0x03
#define VLV_STATE_BLOCKS    (byte) 0x04
#define VLV_STATE_UNKNOWN   (byte) 0x05
#define VLV_STATE_OPENCIR   (byte) 0x06


typedef struct {
//typedef struct valves {
  unsigned int closing_count;  
  unsigned int opening_count;
  unsigned int deadzone_count;  
  unsigned int scaler;
  unsigned int meancurrent;
  byte target_position;
  byte actual_position;
  byte status;
} valves;

extern valves myvalves[ACTUATOR_COUNT];

enum ASTATE {
A_INIT, A_IDLE, A_CLOSE, A_OPEN1, A_OPEN2, A_LEARN1, 
A_LEARN2, A_LEARN3, A_LEARN4, A_CLOSE1, A_CLOSE2 };

extern enum ASTATE appstate;




byte appcycle ();
byte appsetup ();
enum ASTATE appgetstate ();
int16_t appsetaction(char cmd, byte valveindex, byte pos);