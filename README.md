# Microgravity Ullage Detection (MUD)

## About

## Content

- `MUD.c` can be used to control MUD experiment through command. It has TUI.
- `MUD_BlueOrigin_Code.c` can be used to automate MUD experiment for rocket launch. It is tailor made for Blue Origin New Shepard flight profile.
- `MUD_BlueOrigin_Code_test.c` is used to test half baked code, and it is not guaranteed to be stable.
- `MUD_BlueOrigin_Code.bak.c` is fully functional and stable backup of MUD_BlueOrigin_Code`.c

## Prerequisites
- Raspberry Pi OS
- Raspberry Pi with 40-pin GPIO header

## Dependencies
This code utilizes the following dependencies:

- [WiringPi](https://github.com/WiringPi/WiringPi.git)

- [daqhats](https://github.com/mccdaq/daqhats.git)

Please refer to the above URLs for more information about each dependency.

The experiment hardware is managed by a Raspberry Pi 3B, which interfaces with all components of MUD. You can operate the experiment using either `MUD.c` or `MUD_BlueOrigin_Code.c`:

- `MUD.c` offers a text-based user interface (TUI) for manual control and monitoring of the experiment.
- `MUD_BlueOrigin_Code.c` provides automated control specifically designed for the Blue Origin New Shepard flight profile, enabling hands-off operation during launch and flight.

Choose the appropriate program based on your experimental scenario and required level of automation.

## How to Operate
Both programs (`MUD.c` and `MUD_BlueOrigin_Code.c`) should be placed within a same folder for the experiment, and both code can be compiled and build by attached CMake file. The operation procedures differ between `MUD.c` and `MUD_BlueOrigin_Code.c`:

- **`MUD.c`**:  
    Designed for manual control via a text-based user interface (TUI).  
    Launch the program in a terminal to interactively monitor and control the experiment.  
    Follow on-screen prompts to execute commands and view experiment status.

- **`MUD_BlueOrigin_Code.c`**:  
    Intended for automated operation during the Blue Origin New Shepard flight.  
    Run the program to initiate automated data collection and experiment control according to the flight profile.  
    Minimal user interaction is required; the program will log events and manage hardware based on mission timing.

Refer to the sections below for details on TUI usage, command-line options, and experiment scenarios.

### TUI
The image below shows the TUI of `MUD.c`, which provides interactive control and monitoring of the experiment.
![TUI screenshot](images\TUI.png)
| Row | Explanation  |
|-----------------|-----------------|
|   2   |The second row displays a selection of commands that can be entered in the third row; note that this is not an exhaustive list of all available commands.|
|   3   |Command input field|
|   4   ||
|   5   ||
|   6   ||
|   7   ||
|   8   ||
|   9   ||
|   10  ||
|   11  ||

### Command
| Command | Explanation  |
|-----------------|-----------------|
|   atob   ||
|   btoa   ||
|   act   ||
|   data   ||
|   flight   ||
|   s1   ||
|   s2   ||
|   s3   ||
|   file  ||
|   cam  ||
|   speed  ||
|   count  ||
speed"enter the pump speed you want 0-100:"
channels
count"Enter Device Count\n"

### Manual data collection
need diagram.

### New Shepard flight
The following table is a flight profile of events, and Mission Elapsed Time colored in <span style="color: red;">RED</span> are events that MUD monitors or actions linked to the event.

| Mission Elapsed Time (MET) (seconds) | New Shepard EVENT  | MUD EVENT  |
|-----------------|-----------------|-----------------|
| <span style="color: red;">-300</span>    | Payload Power and Data On    |   MUD turns ON and starts recording data(Controller, pressure transducer, flow sensor, LED, white noise generator/amplifier, and actuator turn on). Solenoid valve 3 equilizes the pressure of Tanks A and B as needed. When controller booted, `crontab` run `mud_blue.202x`       |
| 0     | Main Engine Ignition Command  |
| 8     | Liftoff  |
| 128   | Max g on Ascent  |
| 141   | MECO (Main Engine Cut Off)   |
| <span style="color: red;">163</span>   | Separate Crew Capsule, fire RCS to stabilize Crew Capsule   |   Controller expect digital signal indicate Crew Capsule separation. When it recieve signal, contrller count 30 seconds and log `"CC + 30"`, which is about MET+193. 30 seconds wait time exists to let liquid in the tank settle down for clean and high-resolution data from an equilibrated liquid.    |
| <span style="color: red;">181</span>   | Coast Start Detected RCS stabilization firings complete, Sensed Acceleration < 0.01 g   |    Count an additional 15 seconds from `"CC+30"` and start periodical equilization. 1.5 seconds later, contrller log `"Equalization Start"`, and pump turns on to move liquid from Tank A to Tank B for 120 seconds. Count 1.5 second, stop periodical equilization, and log `"Equalization End" `. 1.5 seconds gap between the beginning and end of periodic equalization and pump is to avoid an increase in start-up impedance happening at the same time.   |
| 245   | Apogee  |
| <span style="color: red;">332</span>   | Sensed Acceleration  > 0.01 g  |   Experiment expected to end at MET+328, and log `"Experiment Off"` and begin shutdown process. During the shutdown process, cameras will repeat turning on and off a few times, and log `"Quitting Program"`. This is part of the video saving process. Once the shutdown process ends, log `"Program ended safely"` and controller shutdown. Flow sensor and LED remain turned on.   |
| 346   | Coast End Detected Sensed Acceleration  > 0.1 g  |
| 364   | Sensed Acceleration  > 1.0 g  |
| 381   | Max g on Reentry  |
| 496   | Deploy Drogues  |
| 508   | Deploy Mains   |
| 622   | Initiate Terminal Decelerator  |
| <span style="color: red;">922</span>   | Payload Power Off, Mission End   |   Flow sensor and LED turn off as Crew Capsule power off.   |


For questions regarding the New Shepard flight profile, please refer to [Blue Origin's payload information](https://www.blueorigin.com/new-shepard/payloads).

## Expected output in log
| Expected output timing in MET | Expected Log output  |   Reason and Usage   |
|-----------------|-----------------|-----------------|
|   0~30   |   <ul><li>`"mcc118 #%i is open"`</li><li>`"mcc118 #%i scan stopped"`</li><li>`"mcc118 #%i scan buffer cleaned"`</li><li>`"mcc118 #%i closed"`</li></ul>   |    These messages appear when MCC DAQHATs are active when `mud_blue.202x` launch. It likely happen when MUD experience sudden power loss or could not perform prescribed shutdown process. These message can be used as indicator of unstable power from vehicle or extenal power supply.  `mud_blue.202x` perform MCC initilization when it launch, so these messages do not effect MUD data collection process.    |
|   "   |   <ul><li>`"Log Started"`</li><li>`"GPIO initialized"`</li><li>`"Kernel Log Started"`</li><li>`"camera turned on"`</li><li>`"act turned on"`</li><li>`"Data Collection Started"`</li></ul>   |    These message indicates which and when individual component of MUD turned on.   |
|   163~165 |   <ul><li>`"CC"`</li><li>`"Experiment on"`</li></ul>   |  These message apear when WiringPi Pin 24/BCM GPIO 5/Physical Pin 18 detect `HIGH`, and these can be used to debug payload integration. |
|   193~195    |   <ul>`"CC + 30"`   |  This message indicates that liquid in Tank A is expected to be in an equilibrium state.   |
|   207~212    |   <ul><li>`"Equalization Start"`</li><li>`"A to B on"`</li></ul>  |    These messages should apears 15 and 16.5 seconds after `"CC + 30"` message.   |
|   327~332 |   <ul><li>`"A to B off"`</li><li>`"Equalization End"`</li></ul>   |    These messages should apears 120 and 120.5 seconds after `"A to B on"` message.   |
|   357~360    |   <ul>`"Experiment Off"`  |   This message indicates 30 seconds passed from the last message.   |
|   375~380    |   <ul>`"Quitting Program"` |   This message appears when `quitting_program` flag is set. Data collection will begin to wrap up buffered data, clean up heap-allocated memories, and close MCC DAQHATs.   |
|   "   |   <ul>`"Program ended safely"`    |   This is and should be **the last message** in the log file. This message indicates all buffers, heaps, and resources are cleaned up, leaving no remains.    |