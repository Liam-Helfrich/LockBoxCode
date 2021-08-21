## Overview
This is my electronic lockbox. It can be locked with a six-digit PIN or for a duration of time up to thirty days. I created it from scratch, which involved designing the box in CAD, 3D-printing it, selecting and ordering the electronics, creating a schematic, wiring everything together, and writing the software to control it.

![ClosedBox](https://user-images.githubusercontent.com/78624384/130335036-70e5ead1-01f4-411a-b8d2-1a44e0405642.jpg)

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

### Lock/Unlock with PIN

To lock the box, put on the lid and twist it as far clockwise as it will go. Selecting the "Lock" option in the main menu will lock the box shut. A menu will appear to ask for the combination:

![EnterCombo](https://user-images.githubusercontent.com/78624384/130336484-8d648e99-45c5-4575-a8a8-a682f4db3f5a.png)

### Lock for Duration

Selecting the "Time lock" menu option opens a screen where the lock duration can be set. The controls work the same way as they do when setting the PIN:

![SettingDuration](https://user-images.githubusercontent.com/78624384/130335027-55a37b77-cbac-4e32-a3e8-c0f3890a7314.jpg)

The format for entering times is DD:HH:MM. In the above picture, the duration is set to 29 days, 23 hours, and 59 minutes. 

The confirmation dialog will display the current time and date. If the time and date shown are not reasonable values, then the internal clock may be malfunctioning. If this occurs, replace the clock battery and set the correct time before using the timed lock feature.

![image](https://user-images.githubusercontent.com/78624384/130336511-4cf2949d-153d-4177-8b79-dfe0b78f0ffe.png)

Once the box is locked, it cannot be opened until the set duration elapses. Because the internal clock runs on its own battery, the box can be switched off in the meantime. Switching the box on will display the time remaining:

![WaitingForUnlock](https://user-images.githubusercontent.com/78624384/130335031-e44edff8-4b3a-4a72-b40a-da81d46a55dc.jpg)

Once the duration has elapsed, any button can be used to unlock the box:

![DurationElapsed](https://user-images.githubusercontent.com/78624384/130335033-2ad136a8-4e8c-43bf-bc7a-3cdf2c5090c4.jpg)





## Hardware Design

![Electronics](https://user-images.githubusercontent.com/78624384/130335014-18b24654-9e1d-4452-9a5e-bfba166d5159.jpg)

## Software Design

![SetCodeSelected](https://user-images.githubusercontent.com/78624384/130335020-a2a1fc08-120e-4073-b3c1-1ce83627e544.jpg)

![CancelDuration](https://user-images.githubusercontent.com/78624384/130335029-305bcce7-a275-47a3-9a3e-57cfd631121d.jpg)
![LockBarView](https://user-images.githubusercontent.com/78624384/130335034-6e1d0bf2-7da5-499a-82f4-35ea5f4eed74.jpg)






