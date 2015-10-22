//*****************************************************************************
/// @file
/// @brief
///   Arduino SmartThings Shield Blind Controller
///   (c) 2015 Michael Barnathan
/// @note
///              ______________
///             |              |
///             |         SW[] |
///             |[]RST         |
///             |         AREF |--
///             |          GND |--
///             |           13 |--X LED
///             |           12 |--
///             |           11 |-- HOLD BUTTON
///           --| 3.3V      10 |-- DOWN BUTTON
///           --| 5V         9 |-- UP BUTTON
///           --| GND        8 |--
///           --| GND          |
///           --| Vin        7 |--
///             |            6 |-- (RESERVED BY SHIELD)
///           --| A0         5 |--
///           --| A1    ( )  4 |--
///           --| A2         3 |--X THING_RX
///           --| A3  ____   2 |--X THING_TX
///           --| A4 |    |  1 |--
///           --| A5 |    |  0 |--
///             |____|    |____|
///                  |____|
///
//*****************************************************************************
#include <SoftwareSerial.h>
#include <SmartThings.h>

template<class T> inline Print &operator <<(Print &obj, T arg) { obj.print(arg); return obj; }

//*****************************************************************************
// Pin Definitions    | | | | | | | | | | | | | | | | | | | | | | | | | | | | |
//                    V V V V V V V V V V V V V V V V V V V V V V V V V V V V V
//*****************************************************************************

#define PIN_THING_RX    3
#define PIN_THING_TX    2

#define PIN_UP          9
#define PIN_DOWN        10
#define PIN_HOLD        11

#define PIN_LED         13

#define DRAW_MS         27500 // Time from 100% up to 100% down / vice versa (empirically the same in both directions)

//*****************************************************************************
// Global Variables   | | | | | | | | | | | | | | | | | | | | | | | | | | | | |
//                    V V V V V V V V V V V V V V V V V V V V V V V V V V V V V
//*****************************************************************************
SmartThingsCallout_t messageCallout;    // call out function forward decalaration
SmartThings smartthing(PIN_THING_RX, PIN_THING_TX, messageCallout);  // constructor

int currentLevel;

//*****************************************************************************
// Local Functions  | | | | | | | | | | | | | | | | | | | | | | | | | | | | | |
//                  V V V V V V V V V V V V V V V V V V V V V V V V V V V V V V
//*****************************************************************************

void level(int percent) {
  if (percent == currentLevel) {
    // No-op.
    return;
  }

  Serial << "Setting level from " << currentLevel << " to " << percent << ".\n";

  int pinToEngage;

  if (percent < currentLevel) {
    Serial.println("Going up!");
    smartthing.shieldSetLED(0, 1, 1);
    pinToEngage = PIN_UP;
    smartthing.send("turningOff");
  } else {
    Serial.println("Going down!");
    smartthing.shieldSetLED(1, 1, 0);
    pinToEngage = PIN_DOWN;
    smartthing.send("turningOn");
  }

  // How long to get to the new level?
  int duration = abs(percent - currentLevel) * DRAW_MS / 100;

  // If all the way up or down, add an extra 1s
  // Since going all the way, it improves accuracy of this common use case and is "free" - the blinds can't go to 101%.
  if (percent == 0 || percent == 100) {
    duration += 1000;
  }

  Serial << "This will take " << duration << " ms.\n";

  engagePin(pinToEngage);
  delay(duration);
  engagePin(PIN_HOLD);

  smartthing.shieldSetLED(0, 0, 0);
  currentLevel = percent;
  sendCurrentLevel();
  Serial.println("Done setting level.");
}

// To open, we don't write HIGH - that would destroy the remote.
// Rather, we switch to input pinmode.
void resetPins() {
  pinMode(PIN_UP, INPUT);
  pinMode(PIN_DOWN, INPUT);
  pinMode(PIN_HOLD, INPUT);
}

// Switches are closed when pulled down to ground.
// Pins never need to be held on, just "flicked".
void engagePin(int pin) {
  analogWrite(pin, 0);
  delay(100);
  pinMode(pin, INPUT);
}

void sendCurrentLevel() {
  smartthing.send("level " + String(currentLevel));        // send message to cloud
  smartthing.send(currentLevel == 0 ? "off" : "on");
}

//*****************************************************************************
// API Functions    | | | | | | | | | | | | | | | | | | | | | | | | | | | | | |
//                  V V V V V V V V V V V V V V V V V V V V V V V V V V V V V V
//*****************************************************************************
void setup()
{
  // Don't burn out pins, which are expecting 3.3v max.
  resetPins();

  pinMode(PIN_LED, OUTPUT);     // define PIN_LED as an output
  digitalWrite(PIN_LED, LOW);   // turn LED off

  Serial.begin(9600);         // setup serial with a baud rate of 9600

  // Move blinds all the way up to start in known position.
  currentLevel = 100;
  level(0);
}

//*****************************************************************************
void loop()
{
  // run smartthing logic
  smartthing.run();
}

//*****************************************************************************
void messageCallout(String message)
{
  // Will be used to remember state across on/off.
  static int savedLevel = 100;

  if (message == "") {
    return;
  }

  Serial.print("Received message: '");
  Serial.print(message);
  Serial.println("' ");

  // if message contents equals to 'on' then draw shades down
  // else if message contents equals to 'off' then draw shades up
  // else if message is "refresh" or "ping", respond with current level
  // else if message is a number, draw shades partially down
  // otherwise, I have no idea what the device type wants to do.
  if (message == "on") {
    // Restore the saved level.
    level(savedLevel);
  } else if (message == "off") {
    // Save the last level for the next on.
    savedLevel = currentLevel;
    level(0);
  } else if (message == "refresh" || message == "ping") {
    sendCurrentLevel();
  } else {
    int newLevel = message.toInt();
    if (newLevel == 0 && message != "0") {
      Serial << "Unknown message: " << message << "\n";
    } else if (newLevel >= 0 && newLevel <= 100) {
      level(newLevel);
    }
  }
}

