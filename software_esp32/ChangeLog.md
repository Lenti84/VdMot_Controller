# Changelog
All notable changes to this project will be documented in this file.

### Breaking Changed

### Changed

### Fixed

### Removed

## [Unreleased] - Development


## [1.3.7]
### Changed
- System :      increase DYNAMIC_JSON_DOCUMENT_SIZE to 4096
- WebUI  :      Config Assembly / Calibration select valve added

## [1.3.6]
### Fixed
- System :      fix bug if no network time exists
- WebUI :       show information on status bar
### Changed
-platform.ini   Board Info is now wt32-eth01
 
## [1.3.4]
### Added
- Website :     min. couts
                max. calibration retries
                publish diag information
- MQTT :        counts in topic diag
### Breaking Change
- MQTT :        meancur is now in topic diag/meancurrent

## [1.3.3]
### Added
- Website :     Show chip and program size information
                More info during flashing on website
- Syslog :      added with ota STM32
### Changed
- OTA STM32 :   more relaxed timing, replace infinite loop with timeout.

## [1.3.2]
### Breaking Change
- Configuration MQTT :  Strict separate topics - publish values separated for read and write.
                        if config/protocol/publish 'strict separate topics' is activated then values are published with '/value' added.
                        E.g. (Station name).common.heatControl/value.
                        if config/protocol/publish 'strict separate topics' is activated then only write topics are accepted with '/set' added.
                        E.g. (Station name).common.heatControl/set. These topics are not published by VDMot

                        For IOBroker : config/protocol/publish 'strict separate topics' is not needed to activate. You can activate in the mqtt iobroker adapter 'different topic names for read and write'. IOBroker added a '/set' to the write topic. VDMot accept this automatically.
                        
                        For Mosquito and other mqtt server  : It is mandotary to activate config/protocol/publish 'strict separate topics'. Otherwise the system will fail. (Loop malfunction)
                        Please see also : https://github.com/Lenti84/VdMot_Controller/wiki/MQTT

                        actual topic removed, the value is published with target.

### Fixed
- pi controller : sometimes the valves starts with 50% after power up / restart -> pi controller starts after 60sec
- pi controller : problem with window handling
- mqtt : problem with force publish
### Added
- Config : seperate checkbox for timeout tsValue and DS18 failed
### Changed
- Heat Control, Park Position : via mqtt is only temporary now. Control page shows the permanent values, status page shows the temporary values. 
                                After reboot the permanent values are copied into the temporary values.
- syslog configuration : set syslog without reboot.
- website : rename motor characteristic low/high in open/close

## [1.3.1]
### fixed
- website : reload page remains on the actual page
### Added
- website and mqtt : visualization of failed DS18 sensors
- valve : timeout of mqtt received tValue : set valve to a defined position
- valve : timeout of DS18sensor failed (fixed 5 min): set valve to a defined position in Config/Protocol/Timeout
- messenger : send a message if tValue timeout 
- messenger : send a message if DS18 sensor failed (fixed timeout 5 min) 
### Changed
- website : color for valve number changed : gray : disabled in config, green : all ok, yellow : mode=off, red : error (tsValue or DS18 failed)

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

