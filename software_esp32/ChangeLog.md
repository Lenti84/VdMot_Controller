# Changelog
All notable changes to this project will be documented in this file.

### Breaking Changed

### Changed

### Fixed

### Removed

## [Unreleased] - Development

## [1.1.0]
### Changed
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
