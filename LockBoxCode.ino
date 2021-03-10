#include <ArduinoLowPower.h>
#include <nano_gfx.h>
#include <ssd1306.h>
#include <Servo.h>
#include <FlashStorage.h>
#include <DS1302.h>
#include <timestamp32bits.h>
#include <RTClib.h>

#define UP_BUTTON_PIN 1
#define RIGHT_BUTTON_PIN 2
#define DOWN_BUTTON_PIN 3
#define LEFT_BUTTON_PIN 6

#define SERVO_PIN 7
#define SERVO_TRANSISTOR_PIN 0

#define CLOCK_ENABLE_PIN 10  // Chip Enable
#define CLOCK_IO_PIN  9  // Input/Output
#define CLOCK_CLOCK_PIN 8  // Serial Clock

#define COMBO_LENGTH 6 //The current implementation supports combinations of up to 10 digits

#define UNLOCKED_POSITION 180 //The servo angle for the unlocked position
#define LOCKED_POSITION 100

#define SERVO_WAIT_TIME 2000 //How long the servos are enabled before they are disabled again with the transistor
#define SLEEP_TIMEOUT 10000 //How long the device stays awake since last input
#define TICK_PERIOD 1000 //The amount of time between ticks

#define MAX_DAYS 29 //Maximum number of days that can be entered into the set duration menu's days field
#define MAX_HOURS 23
#define MAX_MINUTES 59

DS1302 rtc(CLOCK_ENABLE_PIN, CLOCK_IO_PIN, CLOCK_CLOCK_PIN);

Servo servo;
static unsigned long lastServoActuation = 0;
static unsigned long lastTick = 0;

void move_servo(const int servo_pos){
  lastServoActuation = millis();
  digitalWrite(SERVO_TRANSISTOR_PIN, HIGH); //Enable the servo transistor
  servo.attach(SERVO_PIN);
  servo.write(servo_pos);
  //The deactivation of the servo is handled in the main loop
}

#include "Images.h"
#include "ScreenCommands.h"
#include "ClockCommands.h"
#include "States.h"

void setup() {
  pinMode(UP_BUTTON_PIN, INPUT_PULLDOWN);
  pinMode(RIGHT_BUTTON_PIN, INPUT_PULLDOWN);
  pinMode(DOWN_BUTTON_PIN, INPUT_PULLDOWN);
  pinMode(LEFT_BUTTON_PIN, INPUT_PULLDOWN);
  pinMode(SERVO_TRANSISTOR_PIN, OUTPUT);

  //Initialize the forward declarations (This is agonizing)
  UnlockedScreen = &__UnlockedScreen;
  SetCombo = &__SetCombo;
  LockedScreen = &__LockedScreen;
  SetDuration = &__SetDuration;
  TimeLockedScreen = &__TimeLockedScreen;
  SleepState = &__SleepState;

  //Initialize communications with the display
  ssd1306_128x64_i2c_init();

  //Read the stored data and transfer to the proper state
  switch(stored_state_id.read()){
    case UNLOCKED_STATE_ID: //The box is unlocked.
      curr_state = UnlockedScreen;
      //move_servo(UNLOCKED_POSITION);
      break;
    case LOCKED_STATE_ID:
      curr_state = LockedScreen;
      //move_servo(LOCKED_POSITION);
      break;
    case TIME_LOCKED_STATE_ID:
      curr_state = TimeLockedScreen;
      break;
  }
  curr_state->initialize();

  //Finally, attach the interrupts. (Don't do this before setting the curr_state variable.)
  attachInterrupt(digitalPinToInterrupt(UP_BUTTON_PIN), upButtonInterrupt, RISING);
  attachInterrupt(digitalPinToInterrupt(DOWN_BUTTON_PIN), downButtonInterrupt, RISING);
  attachInterrupt(digitalPinToInterrupt(LEFT_BUTTON_PIN), leftButtonInterrupt, RISING);
  attachInterrupt(digitalPinToInterrupt(RIGHT_BUTTON_PIN), rightButtonInterrupt, RISING);

  
}

void loop() {
  if(servo.attached() && hasElapsed(lastServoActuation, SERVO_WAIT_TIME)){ //Turn the servo off.
    servo.detach();
    digitalWrite(SERVO_TRANSISTOR_PIN, LOW); //Disable the servo transistor
  }
  if(hasElapsed(lastTick, TICK_PERIOD)){ //Execute the current state's tick function (For continuously updating screens, etc.)
    noInterrupts();
    curr_state->tick();
    lastTick = millis();
    interrupts();
  }
  if(curr_state != SleepState && hasElapsed(pressTime, SLEEP_TIMEOUT)){ //Put the device to sleep.
    transferTo(SleepState);
  }
}
