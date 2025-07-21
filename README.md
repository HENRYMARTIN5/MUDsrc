# Microgravity Ullage Detection (MUD)

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

- [WiringPi](https://github.com/mccdaq/daqhats.git)

- [daqhats](https://github.com/WiringPi/WiringPi.git)

Please refer to the above URLs for more information about each dependency.

## Flow of experiment

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
| Mission Elapsed Time (MET) (seconds) | Expected Log output  |   Reason   |
|-----------------|-----------------|-----------------|
|   0~30   |   <ul><li>`"mcc118 #%i is open"`</li><li>`"mcc118 #%i scan stopped"`</li><li>`"mcc118 #%i scan buffer cleaned"`</li><li>`"mcc118 #%i closed"`</li></ul>   |
|   "   |   <ul><li>`"Log Started"`</li><li>`"GPIO initialized"`</li><li>`"Kernel Log Started"`</li><li>`"camera turned on"`</li><li>`"act turned on"`</li><li>`"Data Collection Started"`</li></ul>   |
|   163~165 |   <ul><li>`"CC"`</li><li>`"Experiment on"`</li></ul>   |
|   193~195    |   `"CC + 30"`   |
|   207~212    |   <ul><li>`"Equalization Start"`</li><li>`"A to B on"`</li></ul>  |
|   327~332 |   <ul><li>`"A to B off"`</li><li>`"Equalization End"`</li></ul>   |
|   357~380    |   <ul><li>`"Experiment Off"`</li><li>`"Quitting Program"`</li><li>`"Program ended safely"`</li></ul>   |

<ul><li>C</li></ul>

## How to use

### TUI

### command