/**
  EasyVR Tester

  Dump contents of attached EasyVR module
  and exercise it with playback and recognition.

  Serial monitor can be used to send a few basic commands:
  '?' - display the module setup
  'l' - cycles through available languages
  'c' - cycles through available command groups
  'b' - cycles through built-in word sets
  'g' - cycles through custom grammars
  's123' - play back sound 123 if available (or beep)
  'd0123456789ABCD*#' - dials the specified number ('_' is dial tone)
  'k' - starts detection of tokens
  '4' or '8' - sets token length to 4 or 8 bits
  'n123' - play back token 123 (not checked for validity)
  'm1' - set mic distance to HEADSET
  'm2' - set mic distance to ARMS_LENGTH (default)
  'm3' - set mic distance to FAR_MIC
  'w' - enter sleep mode without audio wakeup (any command interrupts)
  'ww' - enter sleep mode with "whistle" wakeup
  'w2' - enter sleep mode with "double-clap" wakeup
  'w3' - enter sleep mode with "triple-clap" wakeup
  'wl' - enter sleep mode with "loud-sound" wakeup
  'r12' - record message 12 if empty
  'p12' - play back message 12 if recorded
  'e12' - erase message 12

  With EasyVR Shield, the green LED is ON while the module
  is listening (using pin IO1 of EasyVR).
  Successful recognition is acknowledged with a beep.
  Details are displayed on the serial monitor window.

**
  Example code for the EasyVR library v1.10.1
  Written in 2017 by RoboTech srl for VeeaR <http:://www.veear.eu>

  To the extent possible under law, the author(s) have dedicated all
  copyright and related and neighboring rights to this software to the
  public domain worldwide. This software is distributed without any warranty.

  You should have received a copy of the CC0 Public Domain Dedication
  along with this software.
  If not, see <http://creativecommons.org/publicdomain/zero/1.0/>.
*/

#include "Arduino.h"
#if !defined(SERIAL_PORT_MONITOR)
  #error "Arduino version not supported. Please update your IDE to the latest version."
#endif

#if defined(__SAMD21G18A__)
  // Shield Jumper on HW (for Zero, use Programming Port)
  #define port SERIAL_PORT_HARDWARE
  #define pcSerial SERIAL_PORT_MONITOR
#elif defined(SERIAL_PORT_USBVIRTUAL)
  // Shield Jumper on HW (for Leonardo and Due, use Native Port)
  #define port SERIAL_PORT_HARDWARE
  #define pcSerial SERIAL_PORT_USBVIRTUAL
#else
  // Shield Jumper on SW (using pins 12/13 or 8/9 as RX/TX)
  #include "SoftwareSerial.h"
  SoftwareSerial port(12, 13);
  #define pcSerial SERIAL_PORT_MONITOR
#endif

#include "EasyVR.h"

EasyVR easyvr(port);

int8_t bits = 4;
int8_t set = 0;
int8_t group = 0;
uint32_t mask = 0;
uint8_t train = 0;
int8_t grammars = 0;
int8_t lang = 0;
char name[33];
bool useCommands = false;
bool useGrammars = true;
bool useTokens = false;
bool isSleeping = false;
bool isBusy = false;

void setup()
{
  pinMode(7,OUTPUT);
  // setup PC serial port
  pcSerial.begin(9600);
bridge:
  // bridge mode?
  int mode = easyvr.bridgeRequested(pcSerial);
  switch (mode)
  {
  case EasyVR::BRIDGE_NONE:
    // setup EasyVR serial port
    port.begin(9600);
    // run normally
    pcSerial.println(F("Bridge not requested, run normally"));
    pcSerial.println(F("---"));
    break;
    
  case EasyVR::BRIDGE_NORMAL:
    // setup EasyVR serial port (low speed)
    port.begin(9600);
    // soft-connect the two serial ports (PC and EasyVR)
    easyvr.bridgeLoop(pcSerial);
    // resume normally if aborted
    pcSerial.println(F("Bridge connection aborted"));
    pcSerial.println(F("---"));
    break;
    
  case EasyVR::BRIDGE_BOOT:
    // setup EasyVR serial port (high speed)
    port.begin(115200);
    pcSerial.end();
    pcSerial.begin(115200);
    // soft-connect the two serial ports (PC and EasyVR)
    easyvr.bridgeLoop(pcSerial);
    // resume normally if aborted
    pcSerial.println(F("Bridge connection aborted"));
    pcSerial.println(F("---"));
    break;
  }

  // initialize EasyVR  
  while (!easyvr.detect())
  {
    pcSerial.println(F("EasyVR not detected!"));
    for (int i = 0; i < 10; ++i)
    {
      if (pcSerial.read() == EasyVR::BRIDGE_ESCAPE_CHAR)
        goto bridge;
      delay(100);
    }
  }

  pcSerial.print(F("EasyVR detected, version "));
  pcSerial.print(easyvr.getID());

  if (easyvr.getID() < EasyVR::EASYVR3)
    easyvr.setPinOutput(EasyVR::IO1, LOW); // Shield 2.0 LED off

  if (easyvr.getID() < EasyVR::EASYVR)
    pcSerial.print(F(" = VRbot module"));
  else if (easyvr.getID() < EasyVR::EASYVR2)
    pcSerial.print(F(" = EasyVR module"));
  else if (easyvr.getID() < EasyVR::EASYVR3)
    pcSerial.print(F(" = EasyVR 2 module"));
  else
    pcSerial.print(F(" = EasyVR 3 module"));
  pcSerial.print(F(", FW Rev."));
  pcSerial.println(easyvr.getID() & 7);

  easyvr.setDelay(0); // speed-up replies

  if (easyvr.getID() >= EasyVR::EASYVR3_1)
  {
    if (!easyvr.checkMessages() && easyvr.getError() == EasyVR::ERR_CUSTOM_INVALID)
    {
      pcSerial.print(F("Message recovery needed, please wait..."));
      if (easyvr.fixMessages())
        pcSerial.println(F(" done!"));
      else
        pcSerial.println(F(" failed!"));
    }
  }

  pcSerial.print(F("Recorded messages:"));
  if (easyvr.getID() >= EasyVR::EASYVR3_1)
  {
    pcSerial.println();
    for (int8_t idx = 0; idx < 32; ++idx)
    {
      int8_t bits = -1; int32_t len = -1;
    easyvr.dumpMessage(idx, bits, len);
      if ((bits == 0) && (len == 0))
        continue; // skip empty
      pcSerial.print(idx);
      pcSerial.print(F(" = "));
      if (bits < 0 || len < 0)
        pcSerial.println(F(" has errors"));
      else
      {
        pcSerial.print(bits);
        pcSerial.print(F(" bits, size "));
        pcSerial.println(len);
      }
    }
  }
  else
    pcSerial.println(F("n/a"));

  easyvr.setTimeout(0);
  lang = EasyVR::ENGLISH;
  easyvr.setLanguage(lang);
  // use fast recognition
  easyvr.setTrailingSilence(EasyVR::TRAILING_MIN);
  easyvr.setCommandLatency(EasyVR::MODE_FAST);

  int16_t count = 0;

  pcSerial.print(F("Sound table: "));
  if (easyvr.dumpSoundTable(name, count))
  {
    pcSerial.println(name);
    pcSerial.print(F("Sound entries: "));
    pcSerial.println(count);
  }
  else
    pcSerial.println(F("n/a"));

  pcSerial.print(F("Custom Grammars: "));
  grammars = easyvr.getGrammarsCount();
  if (grammars > 4)
  {
    pcSerial.println(grammars - 4);
    for (set = 4; set < grammars; ++set)
    {
      pcSerial.print(F("Grammar "));
      pcSerial.print(set);

      uint8_t flags, num;
      if (easyvr.dumpGrammar(set, flags, num))
      {
        pcSerial.print(F(" has "));
        pcSerial.print(num);
        if (flags & EasyVR::GF_TRIGGER)
          pcSerial.println(F(" trigger"));
        else
          pcSerial.println(F(" command(s)"));
      }
      else
      {
        pcSerial.println(F(" error"));
        continue;
      }

      for (int8_t idx = 0; idx < num; ++idx)
      {
        pcSerial.print(idx);
        pcSerial.print(F(" = "));
        if (!easyvr.getNextWordLabel(name))
          break;
        pcSerial.println(name);
      }
    }
  }
  else
    pcSerial.println(F("n/a"));

  if (easyvr.getGroupMask(mask))
  {
    uint32_t msk = mask;
    for (group = 0; group <= EasyVR::PASSWORD; ++group, msk >>= 1)
    {
      if (!(msk & 1)) continue;
      if (group == EasyVR::TRIGGER)
        pcSerial.print(F("Trigger: "));
      else if (group == EasyVR::PASSWORD)
        pcSerial.print(F("Password: "));
      else
      {
        pcSerial.print(F("Group "));
        pcSerial.print(group);
        pcSerial.print(F(" has "));
      }
      count = easyvr.getCommandCount(group);
      pcSerial.print(count);
      if (group == 0)
        pcSerial.println(F(" trigger(s)"));
      else
        pcSerial.println(F(" command(s)"));
      for (int8_t idx = 0; idx < count; ++idx)
      {
        if (easyvr.dumpCommand(group, idx, name, train))
        {
          pcSerial.print(idx);
          pcSerial.print(F(" = "));
          pcSerial.print(name);
          pcSerial.print(F(", Trained "));
          pcSerial.print(train, DEC);
          if (!easyvr.isConflict())
            pcSerial.println(F(" times, OK"));
          else
          {
            int8_t confl = easyvr.getWord();
            if (confl >= 0)
              pcSerial.print(F(" times, Similar to Word "));
            else
            {
              confl = easyvr.getCommand();
              pcSerial.print(F(" times, Similar to Command "));
            }
            pcSerial.println(confl);
          }
        }
      }
    }
  }
  easyvr.setMicDistance(3);
  easyvr.setKnob(0);
  easyvr.setLevel(0);
  group = 0;
  set = 0;
  mask |= 1; // force to use trigger (mixed SI/SD)
  useCommands = (mask != 0);
  isSleeping = false;
  pcSerial.println(F("---"));
 
  
}
const char* ws0[] =
{
  "ROBOT",
};

const char* ws2[] =
{
  "LEFT",
  "RIGHT",
  "UP",
  "DOWN",
  "FORWARD",
  "BACKWARD",
};
const char* ws3[] =
{
  "ZERO",
  "ONE",
  "TWO",
  "THREE",
  "FOUR",
  "FIVE",
  "SIX",
  "SEVEN",
  "EIGHT",
  "NINE",
  "TEN",
};
const char** ws[] = { ws0};
/*
const char* ws0[] =
{
  "ROBOT",
};
const char* ws1[] =
{
  "ACTION",
  "MOVE",
  "TURN",
  "RUN",
  "LOOK",
  "ATTACK",
  "STOP",
  "HELLO",
};
const char* ws2[] =
{
  "LEFT",
  "RIGHT",
  "UP",
  "DOWN",
  "FORWARD",
  "BACKWARD",
};
const char* ws3[] =
{
  "ZERO",
  "ONE",
  "TWO",
  "THREE",
  "FOUR",
  "FIVE",
  "SIX",
  "SEVEN",
  "EIGHT",
  "NINE",
  "TEN",
};
const char** ws[] = { ws0, ws1, ws2, ws3 };
*/
void unMuteMicrophone() {
  //
  digitalWrite(7,LOW);
  delay(20000);
  }

void muteMicrophone(){
  //sends power to the relay, which sends signal through the microphone
  //mute circuit and mutes the microphone
   digitalWrite(7,HIGH); 
  
  }




int readNum()
{
  int rx;
  int num = 0;
  delay(5);
  while ((rx = pcSerial.read()) >= 0)
  {
    delay(5);
    if (isdigit(rx))
      num = num * 10 + (rx - '0');
    else
      break;
  }
  return num;
}


void loop()
{
  muteMicrophone();
  
  //checkMonitorInput();

  if (!isSleeping && !isBusy)
  {
   
    easyvr.setPinOutput(EasyVR::IO1, HIGH); // Shield 2.0 LED on (listening)

    if (useTokens)
    {
      pcSerial.print(F("Detect a "));
      pcSerial.print(bits);
      pcSerial.println(F(" bit token ..."));
      easyvr.detectToken(bits, EasyVR::REJECTION_AVG, 0);
    }
    else if (useCommands)
    {
      pcSerial.print(F("Say a command in Group "));
      pcSerial.println(group);
      easyvr.recognizeCommand(group);
    }
    else if (useGrammars)
    {
      pcSerial.print(F("Say a word in Wordset "));
      pcSerial.println(set);
      easyvr.recognizeWord(set);
    }
  }
  
  while (!easyvr.hasFinished());
  isSleeping = false;
  isBusy = false;

  if (easyvr.isAwakened())
  {
    pcSerial.println(F("Audio wake-up!"));
    return;
  }

  int16_t idx;
  if (useTokens)
  {
    idx = easyvr.getToken();
    if (idx >= 0)
    {
      pcSerial.print(F("Token: "));
      pcSerial.println(idx);
      easyvr.playSound(0, EasyVR::VOL_FULL);
    }
  }
  // handle voice recognition
  idx = easyvr.getWord();
  if (idx >= 0)
  {
    pcSerial.print(F("Word: "));
    pcSerial.println(easyvr.getWord());
    if (ws[group][idx] == "ROBOT") {
      pcSerial.println("Success");
      unMuteMicrophone();}
    else 
      pcSerial.println("Failure");
    pcSerial.print(F(" = "));
    if (useCommands) {
      pcSerial.println("Command Used:");
      pcSerial.println(ws[set][idx]);
    }
    // --- optional: builtin words can be retrieved from the module
    else if (set < 4) {
      pcSerial.println("WS Used");
      pcSerial.println(ws[set][idx]);
    // ---
    }
    else
    {
      uint8_t flags, num;
      if (easyvr.dumpGrammar(set, flags, num))
        while (idx-- >= 0)
        {
          if (!easyvr.getNextWordLabel(name))
            break;
        }
      if (idx < 0)
        pcSerial.println(name);
      else
        pcSerial.println();
    }
    // ok, let's try another set
    
    easyvr.playSound(0, EasyVR::VOL_FULL);
  }
  else
  {
    idx = easyvr.getCommand();
    if (idx >= 0)
    {
      pcSerial.print(F("Command: "));
      pcSerial.print(easyvr.getCommand());
      
      if (easyvr.dumpCommand(group, idx, name, train))
      {
        pcSerial.print(F(" = "));
        pcSerial.println(name);
      }
      else
        pcSerial.println();
      // ok, let's try another group
      do
      {
        group++;
        if (group > EasyVR::PASSWORD)
          group = 0;
      }
      while (!((mask >> group) & 1));
      easyvr.playSound(0, EasyVR::VOL_FULL);
    }
    else // errors or timeout
    {
      int16_t err = easyvr.getError();
      if (err >= 0)
      {
        pcSerial.print(F("Error 0x"));
        pcSerial.println(err, HEX);
      }
      else if (easyvr.isTimeout())
        pcSerial.println(F("Timed out."));
      else
        pcSerial.println(F("Done."));
    }
  }
}
