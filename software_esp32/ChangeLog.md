# Changelog
All notable changes to this project will be documented in this file.

### Breaking Changed

### Changed

### Fixed

### Removed

## [Unreleased] - Development

## [1.3.0]
### fixed
- Issue #110 : MQTT sending uptime
### Added
- window icon
- Issue 111 : Automatic wifi reconnect if lost 
- MQTT : send calibration time/date of valve when calibration is in progress
- MQTT : send link # of valve if exists
- Website : display calibration info in status page when calibration is in progress 

## [1.2.6]
### Added
- window handling : Icon inserted
- MQTT  : sepererate topics handling of read/write 
        : topics values plaintext instead of number possible 
### Changed
- if control is set to off then the valve go to zero position                    

## [1.2.5]
### Added
- window handling : activate window handling in control
                    if window is open the background color is red in Status window
## [1.2.4]
### Added
- window handling : mqtt flag   window/state 0 = closed, 1 = open
                                window/target target position if window is open. no control

## [1.2.3]
### Added
- mqtt : retained flag

## [1.2.2]
### Added
- status page : display valve movements, open counts, close counts, dead counts
### Changed
- web site : more modern style
- status page is alwas displayed even a login user exists, the activate-set slider is disabled in this case.
### Fixed
- hour time was not set correctly when only changed hour time

## [1.2.1]
### Added
- status page : display heat control state and last calibration 

## [1.2.0]
### Breaking Changed
- use LittleFS instead SPIFFS, all files will be removed with update.
### Added
- messenger : pushover and email support in case of errors
### Changed
- CWT32AsyncOTA::restart() : direct call of ESP.restart() instead Services.restartSystem(false);

## [1.1.1]
### Breaking Changed
- protocol flags redefined, please check settings 
### Changed
- MQTT : loop interval and publish interval separated. Default : interval = 1000 (ms) , publish 10 (s)  

## [1.1.0]
### Changed
- webui : control configuration does not reset the system
- PI Controller : Add cooling function
- MQTT : publish esp32 reset reason
- MQTT : publish esp32 uptime (in config activate/deactivate)
- MQTT : don't set the actuators.target_position if valve is deactivated in config
- Statuspage : hide column temp1 and temp2 if no temp sensor is linked to valve
- Config : after saving config to stm, wait until eeprom-write is finished (timeout = 60s)
- add delay after reset to give stm more time to initialise
- platformio.ini : forcehardreset enabled
- web : show tTarget and tValue on status page (issue #76)
- web : show alert if manual valve setting on status page is enabled and on control page manual heat control is not set (issue #76)
- STM : always hard-reset stm with start of esp except power-on
- Env : suppress all debug outputs in release environment, except IP and  sytem message 
### Fixed
- PIControl : set linked valves to park position also (issue #73)
- small fixes in webui

## [1.0.9]
### Fixed
- MQTT : possible buffer overflow  

## [1.0.8]
### Added
- forceHardReset option added, but disabled by default
### Changed
- Softreset timing
### Fixed
- web : bugfix calibration settings
- stm32: fixed findings with uninitialized vaiables, unclean global and local declarations, removed 2 doubled commands


## [1.0.7] 
### Added
- new STM32 commands: soft reset, match 1-wire sensors

### Fixed
- fixed problems with configuration of 1-wire sensors (matching to valves) 


## [1.0.6] 
### Added
- Introducing start valve position after power on

### Changed
- start pi-control tasks delayed

### Fixed
- web : fixed problems with configuration savings 
- missing prefs.end in vdmconfig added
- don't start pi-control task, if ts = 0
- mqtt compare topics with non leading slash

