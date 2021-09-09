#include "Arduino.h"

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
int8_t group = 1;
uint32_t mask = 0;
uint8_t train = 0;
int8_t grammars = 0;
int8_t lang = 0;
char name[33];
bool useCommands = true;
bool useGrammars = false;
bool useTokens = false;
bool isSleeping = false;
bool isBusy = false;

const char* ws0[] =
{
  "ROBOT",};
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

const char** ws[] = {ws0, ws1};

//Check for user saying "action". If true then turn on LED. 
void setup() {
  // put your setup code here, to run once:
  pcSerial.begin(9600);
  //initialize easyvr
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
  pcSerial.println("Starting to run");
  pcSerial.print("EasyVR detected, version ");
  pcSerial.print(easyvr.getID());
  
  easyvr.setDelay(0);
  //sets language to english (0)
  easyvr.setTimeout(5);
  pcSerial.println("Timeout set");
  lang = EasyVR::ENGLISH;
  easyvr.setLanguage(lang);
  pcSerial.println("Language set");
  // use fast recognition
  easyvr.setTrailingSilence(EasyVR::TRAILING_MIN);
  easyvr.setCommandLatency(EasyVR::MODE_FAST);
  pcSerial.println("Latencies set");
  int16_t count = 0;
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
  group = 0;
  set = 0;
  mask |= 1; // force to use trigger (mixed SI/SD)
  useCommands = (mask != 0);
  isSleeping = false;
  pcSerial.println(F("---"));
  //sets timeout to 10 seconds
  //easyvr.setTimeout(10);
  
}

void loop() {
  //pcSerial.println("Main loop starting");
  // put your main code here, to run repeatedly:
  easyvr.setPinOutput(EasyVR::IO1, HIGH);

  while (!easyvr.hasFinished());
  isSleeping = false;
  isBusy = false;
  pcSerial.println("-");
  int16_t idx;
  idx = easyvr.getWord();
  if (idx >= 0)
  {
    pcSerial.print(F("Word: "));
    pcSerial.print(easyvr.getWord());
    pcSerial.print(F(" = "));
    if (useCommands)
      pcSerial.println(ws[group][idx]);
    // --- optional: builtin words can be retrieved from the module
    else if (set < 4)
      pcSerial.println(ws[set][idx]);
    // ---
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
    if (set < 4)
    {
      set++;
      if (set > 3)
        set = 0;
    }
    else
    {
      set++;
      if (set >= grammars)
        set = 4;
    }
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
  /*
  pcSerial.print("Command in group: ");
  pcSerial.print(group);
  //starts listening to command index 1
  easyvr.recognizeCommand(group);

  int idx = easyvr.getWord();
  //if (idx >= 0) {
   pcSerial.print(F("Word: "));
    pcSerial.print(easyvr.getWord());
    pcSerial.print(F(" = "));
    pcSerial.println(ws[group][idx]);
    //} //should print whatever word
    // in wordset 1 (ws1[]) was said. 
  
 
  //returns the word recognized, 1-31, if operation successful.
  //if error or no word recognized returns -1
  
  delay(5000);
  */
}
