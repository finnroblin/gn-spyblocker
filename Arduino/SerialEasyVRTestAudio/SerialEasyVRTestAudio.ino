#include "Arduino.h"
#if !defined(pcSerial_PORT_MONITOR)
  #error "Arduino version not supported. Please update your IDE to the latest version."
#endif

#if defined(__SAMD21G18A__)
  // Shield Jumper on HW (for Zero, use Programming Port)
  #define port pcSerial_PORT_HARDWARE
  #define pcpcSerial pcSerial_PORT_MONITOR
#elif defined(pcSerial_PORT_USBVIRTUAL)
  // Shield Jumper on HW (for Leonardo and Due, use Native Port)
  #define port pcSerial_PORT_HARDWARE
  #define pcpcSerial pcSerial_PORT_USBVIRTUAL
#else
  // Shield Jumper on SW (using pins 12/13 or 8/9 as RX/TX)
  #include "SoftwarepcSerial.h"
  SoftwarepcSerial port(12, 13);
  #define pcpcSerial pcSerial_PORT_MONITOR
#endif

#include "EasyVR.h"
EasyVR easyvr(port);

//Groups and Commands
enum Groups
{
//GROUP_0  = 0,
GROUP_1  = 1,
};

enum Group0
{
G0_ARDUINO = 0,
};

enum Group1
{
G1_FORWARD = 0,
G1_REVERSE = 1,
G1_LEFT = 2,
G1_RIGHT = 3,
G1_STOP = 4,

};

EasyVRBridge bridge;

int8_t group, idx;

void setup()
{
// bridge mode?
if (bridge.check())
{
cli();
bridge.loop(0, 1, 12, 13);
}
// run normally
pcpcSerial.begin(9600);
port.begin(9600);

if (!easyvr.detect())
{
pcpcSerial.println(“EasyVR not detected!”);
for (;;);
}

easyvr.setPinOutput(EasyVR::IO1, LOW);
pcpcSerial.println(“EasyVR detected!”);
easyvr.setTimeout(5);
easyvr.setLanguage(EasyVR::ENGLISH);

group = EasyVR::TRIGGER; //&amp;lt;– start group (customize)

pinMode(2, OUTPUT);
digitalWrite(2, LOW);    // set the LED off
pinMode(3, OUTPUT);
digitalWrite(3, LOW);
pinMode(4, OUTPUT);
digitalWrite(4, LOW);
pinMode(5, OUTPUT);
digitalWrite(5, LOW);
pinMode(6, OUTPUT);
digitalWrite(6, LOW);

}

void action();

void loop()
{
easyvr.setPinOutput(EasyVR::IO1, HIGH); // LED on (listening)

pcSerial.print(“Say a command in Group”);
pcSerial.println(group);
easyvr.recognizeCommand(group);

do
{
// can do some processing while waiting for a spoken command
}
while (!easyvr.hasFinished());

easyvr.setPinOutput(EasyVR::IO1, LOW); // LED off

idx = easyvr.getWord();
if (idx >= 0)
{
// built-in trigger (ROBOT)
// group = GROUP_X; &amp;lt;– jump to another group X
return;
}
idx = easyvr.getCommand();
if (idx >= 0)
{
// print debug message
uint8_t train = 0;
char name[32];
pcSerial.print(“Command: “);
pcSerial.print(idx);
if (easyvr.dumpCommand(group, idx, name, train))
{
pcSerial.print(” = “);
pcSerial.println(name);
}
else
pcSerial.println();
easyvr.playSound(0, EasyVR::VOL_FULL);
// perform some action
action();
}
else // errors or timeout
{
if (easyvr.isTimeout())
pcSerial.println(“Timed out, try again…”);
int16_t err = easyvr.getError();
if (err >= 0)
{
pcSerial.print(“Error “);
pcSerial.println(err, HEX);
}

group = GROUP_1;
}
}

void action()
{
switch (group)
{
// case GROUP_0:
// switch (idx)
//  {
//  case G0_ARDUINO:
// write your action code here
//      group = GROUP_1; //&amp;lt;– or jump to another group X for composite commands
//    break;
//  }
//  break;
case GROUP_1:
switch (idx)
{
case G1_FORWARD:
pcSerial.print(“9”);
digitalWrite(2, HIGH);
break;
case G1_REVERSE:
pcSerial.print(“Q”);
digitalWrite(3,HIGH);
break;
case G1_LEFT:
pcSerial.print(“X”);
digitalWrite(4,HIGH);
break;
case G1_RIGHT:
pcSerial.print(“Y”);
digitalWrite(5,HIGH);
break;
case G1_STOP:
pcSerial.print(“Z”);
digitalWrite(6,HIGH);
break;
}
break;
}
}
