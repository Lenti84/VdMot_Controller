/*
 
*/

//unsigned char motorcycle (unsigned char valvenr, unsigned char position);

byte appcycle (byte cmd, byte valvenr, byte position);
byte appsetup ();

#define CMD_A_OPEN      'o'
#define CMD_A_CLOSE     'c'
#define CMD_A_LEARN     'l'
#define CMD_A_TARGET    't'