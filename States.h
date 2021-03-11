#ifndef STATES_H
#define STATES_H

/*
   State IDs:
   0 = Unlocked (on lock/set combo screen)
   1 = Locked
   2 = Set combination
   3 = Time-Locked
   4 = Set duration
   5 = Sleep
   */
#define UNLOCKED_STATE_ID 0
#define LOCKED_STATE_ID 1
#define SETCOMBO_STATE_ID 2
#define TIME_LOCKED_STATE_ID 3
#define SET_DURATION_STATE_ID 4
#define SLEEP_STATE_ID 5

#define DEBOUNCE_TIME 100 //Button debounce

static unsigned long lastPress = 0; //For interrupt debouncing
static unsigned long pressTime = 0; //A variable for storing the time a button was pressed for comparison with lastPress

//Reserve space for permanent stored data in Flash memory:
FlashStorage(stored_state_id, uint8_t);
FlashStorage(stored_combination, uint32_t);
FlashStorage(locked_at_time, uint32_t);
FlashStorage(locked_until_time, uint32_t);


//Update the state stored in the flash memory:
void storeState(uint8_t state){
  if(stored_state_id.read() != state){
    noInterrupts();
    stored_state_id.write(state);
    interrupts();
  }
}

//Update the combination stored in the flash memory:
void storeCombination(uint32_t combo){
  if(stored_combination.read() != combo){
    noInterrupts();
    stored_combination.write(combo);
    interrupts();
  }
}

bool hasElapsed(const unsigned long& startTime, int duration){ //This function accounts for at most one millis() rollover
  //return millis() >= startTime ? millis()-startTime > duration : (static_cast<unsigned long>(-1)-startTime+millis()) > duration;
  if(millis() >= startTime){
    return millis()-startTime > duration;
  }else{
    uint64_t elapsedTime = static_cast<unsigned long>(-1)-startTime+millis();
    return elapsedTime >= duration;
  }
}

struct State{
  uint8_t state_id;

  virtual void initialize() = 0; //Executes when the box transfers into this state
  virtual void finalize() = 0; //Executes right before the box transfers out of this state

  virtual void upButton() = 0; //Executes when the up button is pressed
  virtual void downButton() = 0;
  virtual void leftButton() = 0;
  virtual void rightButton() = 0;

  virtual void tick() = 0; //Executes once per second while this state is active
};

static State* curr_state = nullptr;
static State* last_state = nullptr;


//Forward declarations of mutually referenced states:
//NAMING CONVENTION:
// No leading underscore = pointer to a state
// One leading underscore = the name of a state
// Two leading underscores = an instance of a state
static State* UnlockedScreen = nullptr;
static State* SetCombo = nullptr;
static State* LockedScreen = nullptr;
static State* SetDuration = nullptr;
static State* TimeLockedScreen = nullptr;
static State* SleepState = nullptr;

//Function to transfer out of the current state and into next_state:
void transferTo(State* next_state){
  curr_state->finalize();
  if(curr_state != SleepState){
    last_state = curr_state;
  }
  curr_state = next_state;
  curr_state->initialize();
}

//Define this device's states:

struct _UnlockedScreen: public State{
  uint8_t state_id = UNLOCKED_STATE_ID;
  uint8_t substate_id = 0;

  /*
   * Substates:
   * 0 -> Lock button selected
   * 1 - > Set password button selected
   * 2 - > Duration lock
   */

  void initialize(){
    //Turn the display on:
    ssd1306_displayOn();
    //Select the default menu option:
    substate_id = 0;
    //Draw the menu for the first time:
    draw_menu();
  }

  void finalize(){
    ssd1306_clearScreen();
  }

  void upButton(){ //Moves up on the menu
    if(substate_id > 0){
      substate_id--;
      draw_menu();
    }
  }

  void downButton(){ //Moves down on the menu
    if(substate_id < 2){
      substate_id++;
      draw_menu();
    }
  }

  void leftButton(){ //Does nothing
    //transferTo(SleepState);
  }

  void rightButton(){ //Transfers to the selected state
    if(substate_id == 0){ //Lock and send to sleep state
      //DISPLAY LOCKED IMAGE
      storeState(LOCKED_STATE_ID); //The box is now locked
      move_servo(LOCKED_POSITION);
      transferTo(LockedScreen); //Transfer to the sleep state
    }else if(substate_id == 1){ //Set combination.
      transferTo(SetCombo);
    }else if(substate_id == 2){ //Time lock.
      transferTo(SetDuration);
    }
  }

  void tick(){}

  //Helper functions for this state:
  void draw_menu(){
    
    ssd1306_setFixedFont(ssd1306xled_font6x8);
    ssd1306_clearScreen();
    printCenter(("Box unlocked."), 0);
    ssd1306_setFixedFont(ssd1306xled_font8x16);
    //ssd1306_clearBlock(0, 30, 128, 40);
    
    ssd1306_printFixed(8, 16, (String(F("Lock"))+String(substate_id == 0 ? F(" >") : F(""))).c_str(), substate_id == 0 ? STYLE_BOLD : STYLE_NORMAL);
    ssd1306_printFixed(8, 32, (String(F("Set code"))+String(substate_id == 1 ? F(" >") : F(""))).c_str(), substate_id == 1 ? STYLE_BOLD : STYLE_NORMAL);
    ssd1306_printFixed(8, 48, (String(F("Time lock"))+String(substate_id == 2 ? F(" >") : F(""))).c_str(), substate_id == 2 ? STYLE_BOLD : STYLE_NORMAL);
  }
} __UnlockedScreen;

struct _SetCombo: public State{
  uint8_t state_id = SETCOMBO_STATE_ID;
  int8_t substate_id = 0;

  /*
   Substates:
   -1 -> Cancel
   0 through COMBO_LENGTH - 1 -> Specify digit
   COMBO_LENGTH -> Confirm
   */

  uint8_t curr_combo[COMBO_LENGTH] = {0}; //The combination currently displayed on the screen

  void initialize(){
    substate_id = 0;
    //Read the combo from flash:
    String combo_string = String(stored_combination.read());
    int num_preceding_zeros = COMBO_LENGTH - combo_string.length(); //The number of zeros preceding the number
    for(uint8_t i = 0; i < COMBO_LENGTH; i++){ //Take the numbers in the string and save them as integers in the combo array
      if(i < num_preceding_zeros){
        curr_combo[i] = 0;
      }else{
        curr_combo[i] = String(combo_string[i-num_preceding_zeros]).toInt();
      }
    }
    //Print the combination to the screen:
    printCombo();
  }

  void finalize(){
    ssd1306_clearScreen();
  }

  void upButton(){ //Increase the current digit.
    if(substate_id > -1 && substate_id < COMBO_LENGTH){
      if(curr_combo[substate_id] < 9){
        curr_combo[substate_id]++;
      }else{
        curr_combo[substate_id] = 0;
      }
      printCombo();
    }
  }

  void downButton(){ //Decrease the current digit.
    if(substate_id > -1 && substate_id < COMBO_LENGTH){
      if(curr_combo[substate_id] > 0){
        curr_combo[substate_id]--;
      }else{
        curr_combo[substate_id] = 9;
      }
      printCombo();
    }
  }

  void leftButton(){ //Decrease the substate ID
    if(substate_id > -1){
      substate_id--;
      printCombo();
    }else if(substate_id == -1){
      //Cancel.
      transferTo(UnlockedScreen);
    }
  }

  void rightButton(){
    if(substate_id < COMBO_LENGTH){ //Move right on the menu
      substate_id++;
      printCombo();
    }else if(substate_id == COMBO_LENGTH){ //Save the set password to flash
      String combo_string = "";
      for(int i = 0; i < COMBO_LENGTH; i++){
        combo_string += String(curr_combo[i]);
      }
      storeCombination(combo_string.toInt());
      transferTo(UnlockedScreen);
    }
  }

  void tick(){}

  //Helper functions for this state:
  void printCombo(){
    ssd1306_clearScreen();
    ssd1306_setFixedFont(ssd1306xled_font6x8);
    printCenter(("Set combination:"), 0);
    ssd1306_setFixedFont(courier_new_font11x16_digits);
    for(int i = 0; i < COMBO_LENGTH; i++){
      if(i == substate_id){ //If the current digit is selected, draw the arrows and highlight the number
        ssd1306_drawBitmap((113 - COMBO_LENGTH*11)/2 + 11*i, 2, 24, 8, UpArrow);
        ssd1306_drawBitmap((113 - COMBO_LENGTH*11)/2 + 11*i, 7, 24, 8, DownArrow);
        ssd1306_negativeMode();
      }else{
        ssd1306_positiveMode();
      }
      ssd1306_printFixed((128 - COMBO_LENGTH*11)/2 + 11*i, 32, String(curr_combo[i]).c_str(), STYLE_NORMAL);
      
    }
    ssd1306_positiveMode();
    if(substate_id == -1){
      ssd1306_setFixedFont(ssd1306xled_font6x8);
      printCenter(("< Cancel?"), 56);
    }else if(substate_id == COMBO_LENGTH){
      ssd1306_setFixedFont(ssd1306xled_font6x8);
      printCenter(("Confirm? >"), 56);
    }  
  }
} __SetCombo;

//The screen the user sees while the device is locked. This is where the password is entered.
struct _LockedScreen: public State{
  uint8_t state_id = LOCKED_STATE_ID;
  int8_t substate_id = 0;

  uint8_t curr_combo[COMBO_LENGTH] = {0}; //The combination currently displayed on the screen

  void initialize(){
    substate_id = 0; //Set the default selection on the menu
    //Print the combination to the screen:
    printCombo();
  }

  void finalize(){
    memset(curr_combo, 0, COMBO_LENGTH); //Clear the entered password field
    ssd1306_clearScreen();
  }

  void upButton(){ //Increase the current digit.
    if(substate_id > -1 && substate_id < COMBO_LENGTH){
      if(curr_combo[substate_id] < 9){
        curr_combo[substate_id]++;
      }else{
        curr_combo[substate_id] = 0;
      }
      printCombo();
    }
  }

  void downButton(){ //Decrease the current digit.
    if(substate_id > -1 && substate_id < COMBO_LENGTH){  
      if(curr_combo[substate_id] > 0){
        curr_combo[substate_id]--;
      }else{
        curr_combo[substate_id] = 9;
      }
      printCombo();
    }
  }

  void leftButton(){ //Decrease the substate ID
    if(substate_id > -1){
      substate_id--;
      printCombo();
    }else if(substate_id == -1){
      //Cancel.
      //Transfer to the sleep state:
      //transferTo(SleepState);
    }
  }

  void rightButton(){
    if(substate_id < COMBO_LENGTH){ //Move right on the menu
      substate_id++;
      printCombo();
    }else if(substate_id == COMBO_LENGTH){ //Check if the password is correct.
      String combo_string = "";
      for(int i = 0; i < COMBO_LENGTH; i++){ //Take the individual digits in the curr_combo array and convert them into a single integer
        combo_string += String(curr_combo[i]);
      }
      if(stored_combination.read() == combo_string.toInt()){ //Unlock the device. Display an unlocked symbol while the servo actuates, then transfer to the unlocked screen
        ssd1306_setFixedFont(ssd1306xled_font6x8);
        ssd1306_clearScreen();
        printCenter("Unlocked!", 32); //REPLACE THIS WITH AN IMAGE
        storeState(UNLOCKED_STATE_ID);
        move_servo(UNLOCKED_POSITION);
        transferTo(UnlockedScreen);
      }else{ //The password is incorrect. Inform the user and delay.
        //We remain in the current state.
        memset(curr_combo, 0, COMBO_LENGTH); //Clear the entered password field
        substate_id = 0;
        printCombo();
        ssd1306_clearBlock(0, 0, 128, 8);
        ssd1306_setFixedFont(ssd1306xled_font6x8);
        printCenter("Incorrect password.", 0);
      }
    }
  }

  void tick(){}

  //Helper functions for this state:
  void printCombo(){ //The screen that allows the user to enter the combination and unlock the box
    ssd1306_clearScreen();
    ssd1306_setFixedFont(ssd1306xled_font6x8);
    printCenter(("Enter combination:"), 0);
    ssd1306_setFixedFont(courier_new_font11x16_digits);
    for(int i = 0; i < COMBO_LENGTH; i++){
      if(i == substate_id){ //If the current digit is selected, draw the arrows and highlight the number
        ssd1306_drawBitmap((113 - COMBO_LENGTH*11)/2 + 11*i, 2, 24, 8, UpArrow);
        ssd1306_drawBitmap((113 - COMBO_LENGTH*11)/2 + 11*i, 7, 24, 8, DownArrow);
        ssd1306_negativeMode();
      }else{
        ssd1306_positiveMode();
      }
      ssd1306_printFixed((128 - COMBO_LENGTH*11)/2 + 11*i, 32, String(curr_combo[i]).c_str(), STYLE_NORMAL);
      
    }
    ssd1306_positiveMode();
    if(substate_id == -1){
      //ssd1306_setFixedFont(ssd1306xled_font6x8);
      //printCenter(("< Cancel?"), 56);
    }else if(substate_id == COMBO_LENGTH){
      ssd1306_setFixedFont(ssd1306xled_font6x8);
      printCenter(("Confirm? >"), 56);
    }  
  }
} __LockedScreen; 

struct _SetDuration: public State{
  uint8_t state_id = SET_DURATION_STATE_ID;
  int8_t substate_id = 0;

  /*
   * Substates:
   * -1 -> Cancel
   * 0 - 2 -> Days/Hours/Minutes 
   * 3: Lock
  */

  uint8_t curr_combo[3] = {0}; //The combination (duration) currently displayed on the screen

  void initialize(){
    substate_id = 0;
    //Print the combination to the screen:
    printCombo();
  }

  void finalize(){
    memset(curr_combo, 0, 3); //Clear the entered password field
    ssd1306_clearScreen();
  }

  void upButton(){ //Increase the current digit.
    if(substate_id == 0){ //Days
      if(curr_combo[substate_id] < MAX_DAYS){
        curr_combo[substate_id]++;
      }else{
        curr_combo[substate_id] = 0;
      }
      printCombo();
    }else if(substate_id == 1){ //Hours
      if(curr_combo[substate_id] < MAX_HOURS){
        curr_combo[substate_id]++;
      }else{
        curr_combo[substate_id] = 0;
      }
      printCombo();
    }else if(substate_id == 2){ //Minutes
      if(curr_combo[substate_id] < MAX_MINUTES){
        curr_combo[substate_id]++;
      }else{
        curr_combo[substate_id] = 0;
      }
      printCombo();
    }
  }

  void downButton(){ //Decrease the current digit.
    if(substate_id == 0){ //Days
      if(curr_combo[substate_id] > 0){
        curr_combo[substate_id]--;
      }else{
        curr_combo[substate_id] = MAX_DAYS;
      }
      printCombo();
    }else if(substate_id == 1){ //Hours
      if(curr_combo[substate_id] > 0){
        curr_combo[substate_id]--;
      }else{
        curr_combo[substate_id] = MAX_HOURS;
      }
      printCombo();
    }else if(substate_id == 2){ //Minutes
      if(curr_combo[substate_id] > 0){
        curr_combo[substate_id]--;
      }else{
        curr_combo[substate_id] = MAX_MINUTES;
      }
      printCombo();
    }
  }

  void leftButton(){ //Decrease the substate ID
    if(substate_id > -1){
      substate_id--;
      printCombo();
    }else if(substate_id == -1){
      //Cancel.
      transferTo(UnlockedScreen);
    }
  }

  void rightButton(){
    if(substate_id < 3){ //Move right on the menu
      substate_id++;
      printCombo();
    }else if(substate_id == 3){ //Save the set password to flash
      //Convert the duration array into seconds:
      uint32_t duration = curr_combo[0]*86400 + curr_combo[1]*3600 + curr_combo[2]*60;
      //Add that duration to the current unix timestamp and save to flash
      Time t = rtc.time(); //Get the current time
      uint32_t current_timestamp = time_to_timestamp(t);
      locked_at_time.write(current_timestamp); //Record the time at which the box was locked. This will help for clock function verification later
      locked_until_time.write(current_timestamp + duration); //This is the timestamp at which the box will unlock
      stored_state_id.write(TIME_LOCKED_STATE_ID); //The box now will now remember that it is locked
      move_servo(LOCKED_POSITION); //Lock the box.
      transferTo(TimeLockedScreen);
    }
  }

 void tick(){}
  
 void printCombo(){
    ssd1306_clearScreen();
    ssd1306_setFixedFont(ssd1306xled_font6x8);
    printCenter("Set duration:", 0);
    ssd1306_setFixedFont(courier_new_font11x16_digits);
    for(int i = 0; i < 3; i++){
      if(i == substate_id){
        ssd1306_negativeMode();
      }else{
        ssd1306_positiveMode();
      }
      ssd1306_printFixed(20+33*i, 32, (String(curr_combo[i] < 10 ? "0" : "") + String(curr_combo[i])).c_str(), STYLE_NORMAL);
      ssd1306_positiveMode();
      ssd1306_printFixed(42, 32, ":", STYLE_NORMAL);
      ssd1306_printFixed(75, 32, ":", STYLE_NORMAL);
    }
    if(substate_id > -1 && substate_id < 3){
      ssd1306_drawBitmap(19+33*substate_id, 2, 24, 8, UpArrow);
      ssd1306_drawBitmap(19+33*substate_id, 7, 24, 8, DownArrow);
    } else if(substate_id == -1){
      ssd1306_setFixedFont(ssd1306xled_font6x8);
      printCenter(("< Cancel?"), 56);
    }else if(substate_id == 3){
      ssd1306_setFixedFont(ssd1306xled_font6x8);
      Time t = rtc.time();
      printCenter(timeAsString(t).c_str(), 16);
      printCenter(("Confirm? >"), 56);
    }  
  }

} __SetDuration; 

struct _TimeLockedScreen: public State{
  uint8_t state_id = TIME_LOCKED_STATE_ID;
  uint8_t substate_id = 0;

  void initialize(){
    printScreen();
  }

  void finalize(){
    ssd1306_clearScreen();
  }

  void upButton(){
    if(!isLocked()){
      unlock();
    }
  }

  void downButton(){
     if(!isLocked()){
      unlock();
    }
  }

  void leftButton(){
     if(!isLocked()){
      unlock();
    }
  }

  void rightButton(){
     if(!isLocked()){
      unlock();
    }
  }

  void tick(){
    printScreen();
  }

  bool isLocked(){
    static uint32_t last_recorded_time = 0;
    static uint32_t lastCheck = 0;

    Time t = rtc.time();
    uint32_t curr_time = time_to_timestamp(t);

    //Check that the clock is still ticking
    if(hasElapsed(lastCheck, 2000)){ //Check at most every 2 seconds. Otherwise there is a risk we will check the clock in the same second.
      lastCheck = millis();
      if(curr_time <= last_recorded_time){ //The clock is not ticking properly. Unlock the box.
        return false;
      }
      last_recorded_time = curr_time;
    }
    //Check the current time to see if the box should be locked:
    //If the first check returns false, the timer is malfunctioning. If the second check returns false, then either the duration has elapsed or the timer is malfunctioning.
    return (locked_at_time.read() <= curr_time && curr_time < locked_until_time.read());
  }

  void unlock(){
    move_servo(UNLOCKED_POSITION); //Unlock the box
    stored_state_id.write(UNLOCKED_STATE_ID); //The box will now remember that it is unlocked.
    transferTo(UnlockedScreen);
  }

  void printScreen(){
    ssd1306_clearScreen();
    ssd1306_setFixedFont(ssd1306xled_font6x8);
    if(isLocked()){
      Time t = rtc.time();
      TimeSpan ts(locked_until_time.read() - time_to_timestamp(t));
      printCenter("Time to unlock: ", 0);
      printCenter(spanAsString(ts).c_str(), 24);
      printCenter("Current time: ", 40);
      printCenter(timeAsString(t).c_str(), 48);
    }else{
      printCenter("Duration elapsed.", 0);
      printCenter("Press any key", 24);
      printCenter("to unlock.", 32);
    }
  }
  
} __TimeLockedScreen; 
       
//Interrupt functions:

void upButtonInterrupt(){
  pressTime = millis();
  if(hasElapsed(lastPress, DEBOUNCE_TIME)){ //abs(pressTime-lastPress) > DEBOUNCE_TIME)
    lastPress = pressTime;

    curr_state->upButton();
    
  }
}


void downButtonInterrupt(){
  pressTime = millis();
  if(hasElapsed(lastPress, DEBOUNCE_TIME)){
    lastPress = pressTime;

    curr_state->downButton();
    
  }
}

void leftButtonInterrupt(){
  pressTime = millis();
  if(hasElapsed(lastPress, DEBOUNCE_TIME)){
    lastPress = pressTime;

    curr_state->leftButton();
    
  }
}

void rightButtonInterrupt(){
  pressTime = millis();
  if(hasElapsed(lastPress, DEBOUNCE_TIME)){
    lastPress = pressTime;

    curr_state->rightButton();
    
  }
}

void wakeUpInterrupt(){
  pressTime = millis();
  if(curr_state == SleepState){
    transferTo(last_state);
  }
}

struct _SleepState: public State{
  uint8_t state_id = SLEEP_STATE_ID;
  uint8_t substate_id = 0;

  void initialize(){
    ssd1306_displayOff();
    detachInterrupt(digitalPinToInterrupt(UP_BUTTON_PIN)); //Detach the current interrupt on the up button
    LowPower.attachInterruptWakeup(UP_BUTTON_PIN, wakeUpInterrupt, RISING); //Attach the new wakeup interrupt
    LowPower.deepSleep(); //Go to sleep
  }

  void finalize(){
    detachInterrupt(digitalPinToInterrupt(UP_BUTTON_PIN)); //Detach the current interrupt on the up button
    attachInterrupt(digitalPinToInterrupt(UP_BUTTON_PIN), upButtonInterrupt, RISING);  //Attach the normal interrupt
    ssd1306_displayOn();
    ssd1306_clearScreen();
  }

  void upButton(){}

  void downButton(){}

  void leftButton(){}

  void rightButton(){}

  void tick(){};
  
} __SleepState; 

#endif

//A template for easily making new states in the future:
/*
struct _UnlockedScreen: public State{
  uint8_t state_id = 0;
  uint8_t substate_id = 0;

  void initialize(){
    printCenter("Initialized!");
  }

  void finalize(){
    printCenter("Finalized!");
  }

  void upButton(){
    printCenter("Up");
  }

  void downButton(){
    printCenter("Down");
  }

  void leftButton(){
    printCenter("Left");
  }

  void rightButton(){
    printCenter("Right");
  }

  void tick(){};
  
} UnlockedScreen; 
 */
