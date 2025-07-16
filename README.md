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

### New Shepard flight
The following table is a flight profile of events, and Mission Elapsed Time colored in <span style="color: red;">RED</span> are events that MUD monitors or actions linked to the event.

| Mission Elapsed Time (MET) (seconds) | EVENT  |
|-----------------|-----------------|
| <span style="color: red;">-300</span>    | Payload Power and Data On    |
| 0     | Main Engine Ignition Command  |
| 8     | Liftoff  |
| 128   | Max g on Ascent  |
| 141   | MECO (Main Engine Cut Off)   |
| <span style="color: red;">163</span>   | Separate Crew Capsule, fire RCS to stabilize Crew Capsule   |
| <span style="color: red;">181</span>   | Coast Start Detected RCS stabilization firings complete, Sensed Acceleration < 0.01 g   |
| 245   | Apogee  |
| <span style="color: red;">332</span>   | Sensed Acceleration  > 0.01 g  |
| 346   | Coast End Detected Sensed Acceleration  > 0.1 g  |
| 364   | Sensed Acceleration  > 1.0 g  |
| 381   | Max g on Reentry  |
| 496   | Deploy Drogues  |
| 508   | Deploy Mains   |
| 622   | Initiate Terminal Decelerator  |
| 922   | Payload Power Off, Mission End   |


For questions regarding the New Shepard flight profile, please refer to [Blue Origin's payload information](https://www.blueorigin.com/new-shepard/payloads).

## How to use

### TUI

### command