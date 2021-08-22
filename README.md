## Table of Contents

* [Overview](#overview)
* [Using the Box](#using-the-box)
   - [Setting the PIN](#setting-the-pin)
   - [Lock/Unlock with PIN](#lock-and-unlock-with-pin)
   - [Lock for Duration](#lock-for-duration)
* [Hardware Design](#hardware-design)
* [Software Design](#software-design)


## Overview
This is my electronic lockbox. It can be locked with a six-digit PIN or for a duration of time up to thirty days. I created it from scratch, which involved designing the box in CAD, 3D-printing it, selecting and ordering the electronics, creating a schematic, wiring everything together, and writing the software to control it.

![ClosedBox](https://user-images.githubusercontent.com/78624384/130335036-70e5ead1-01f4-411a-b8d2-1a44e0405642.jpg)

I designed the box to consume as little power as possible. The box runs on three AAA batteries and has a battery life of about one month in sleep mode. To conserve energy, the box can be switched off while it is locked: all state information is stored in non-volatile memory and the internal clock runs on a separate battery. This means that the box could theoretically last years on a single battery charge. 

## Using the Box

Using the box is fairly simple. All input is handled through four push buttons on the lid of the box. A small OLED screen displays the user interface:

![ClosedTopView](https://user-images.githubusercontent.com/78624384/130335012-82422aaf-362e-4152-a55d-eac48bf4cc3b.jpg)

To power the box on, flip the switch near the battery cover. The main menu will display on the screen:

![Lock](https://user-images.githubusercontent.com/78624384/130335018-c6594899-b65f-4ad4-99f3-3a45ca9ca7d8.jpg)

The up and down buttons can be used to scroll through the menu. The right button will proceed to the next menu based on the current selection. The "Lock" option locks the box with the current PIN, the "Set code" option allows the user to change the PIN, and the "Time lock" option allows the user to lock the box for a set duration.

Note that the box will enter sleep mode if no input is provided for ten consecutive seconds. Pressing the up button will wake the box from sleep mode.

### Setting the PIN

The "Set code" menu is where the PIN can be set. The left and right buttons are used to select the digit to change, while the up and down buttons increase or decrease the value of the selected digit:

![SettingCode](https://user-images.githubusercontent.com/78624384/130335023-a905079a-68f1-424b-8d43-7c3a4ea1e08b.jpg)

To confirm the new PIN, press the right button while the rightmost digit is selected. The menu will ask for confirmation:

![ConfirmSetCode](https://user-images.githubusercontent.com/78624384/130335025-570d0831-2f9b-4d3d-9369-8a991d7d6f15.jpg)

Pressing the right button once more will confirm the PIN selection. The confirmation can be canceled by pressing the left button. The menu can be exited altogether by pressing the left button while the leftmost digit is selected.

### Lock and Unlock with PIN

To lock the box, put on the lid and twist it as far clockwise as it will go. Selecting the "Lock" option in the main menu will lock the box shut. A menu will appear to ask for the combination:

![EnterCombo](https://user-images.githubusercontent.com/78624384/130336484-8d648e99-45c5-4575-a8a8-a682f4db3f5a.png)

### Lock for Duration

Be sure to turn the lid as far clockwise as it will go before locking. 

Selecting the "Time lock" menu option opens a screen where the lock duration can be set. The controls work the same way as they do when setting the PIN:

![SettingDuration](https://user-images.githubusercontent.com/78624384/130335027-55a37b77-cbac-4e32-a3e8-c0f3890a7314.jpg)

The format for entering times is DD:HH:MM. In the above picture, the duration is set to 29 days, 23 hours, and 59 minutes. 

Once the duration is confirmed, the box will lock. The confirmation dialog will display the current time and date. If the time and date shown are not reasonable values, then the internal clock may be malfunctioning. If this occurs, replace the clock battery and set the correct time before using the timed lock feature.

![image](https://user-images.githubusercontent.com/78624384/130336511-4cf2949d-153d-4177-8b79-dfe0b78f0ffe.png)

Once the box is locked, it cannot be opened until the set duration elapses. Because the internal clock runs on its own battery, the box can be switched off in the meantime. Switching the box on will display the time remaining:

![WaitingForUnlock](https://user-images.githubusercontent.com/78624384/130335031-e44edff8-4b3a-4a72-b40a-da81d46a55dc.jpg)

Once the duration has elapsed, any button can be used to unlock the box:

![DurationElapsed](https://user-images.githubusercontent.com/78624384/130335033-2ad136a8-4e8c-43bf-bc7a-3cdf2c5090c4.jpg)


## Hardware Design

The electronics in the lid are shown below:

![LabeledElectronicsII](https://user-images.githubusercontent.com/78624384/130338196-d95e3e80-d9b9-45c9-835d-82e32e04e476.jpg)


The main board is a Seeeduino XIAO, which uses an ATSAMD21 microcontroller. It is quite small and consumes very little power in sleep mode. A DS1302 real-time clock is connected to the microcontroller via SPI. Its dedicated battery allows it to continue keeping time while the rest of the system is switched off. The OLED display, connected via I2C, is .96 inches in size and has a resolution of 128 x 64. It has its own sleep mode in which it consumes negligible power. The transistor assembly regulates power to the servo to prevent it from consuming power when not in use.

The locking mechanism is a rack and pinion system. When the box is locked, the servo turns a pinion to move a rack into a slot. Once the rack is in place, the lid cannot be twisted until the rack is retracted. Below shows the rack in the locked position, protruding from the locking assembly:

![ProtrudingPinion](https://user-images.githubusercontent.com/78624384/130338164-9254b43c-3460-47ff-8e51-d84cf69dfcba.png)

## Software Design

The code for the box is written in C++. It is divided into five files: 

[LockBoxCode.ino](LockBoxCode.ino) initializes the box when it is turned on. It configures the inputs and outputs, reads the state data from memory, and navigates to the proper screen on startup. It also contains the main loop, which handles sleep mode, controls the servo transistors, and periodically executes functions relevant to the current state. 

[ScreenCommands.h](ScreenCommands.h) contains functions for writing text to the screen.

[Images.h](Images.h) contains bitmap data for the images used in the menu, namely the up and down arrows used when entering a PIN or time duration.

[ClockCommands.h](ClockCommands.h) contains functions for manipulating time data. This includes converting times to strings, UNIX timestamps, and other objects for storing time data.

[States.h](States.h) contains the majority of the box's logic. It handles the menu system, locking and unlocking, and storing state data to memory. It does this with an abstract class called "State". Each of State's subclasses represents a menu screen and the box's behavior when that screen is active. For instance, the class "UnlockedScreen" represents the main menu. It contains the logic to draw the menu onto the screen, and it defines what each button does while that menu is being displayed.

A pointer keeps track of the current state by pointing to a State object. Whenever the box receives an input, the pointer is dereferenced to find the current state's function corresponding to the button pushed. This framework makes it easy to add new functions to the box without having to modify logic elsewhere in the code.
