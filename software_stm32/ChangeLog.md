# Changelog
All notable changes to this project will be documented in this file.

## [Unreleased] - Development

### Breaking Changed

### Changed

### Fixed

### Removed

## [1.3.1]
### fixed
- next calibration : movements set to min. 50

## [1.3.0]
### Fixed
- calibration move numbers was not set from eeprom
- calibration function was not correct
### Added
- send calibration state to WTH32

## [1.2.2]
### Add
- send valve movements, open counts, close counts, dead counts with cmd APP_PRE_GETVLVDATA

## [1.1.0]
### Changed
- new command : APP_PRE_EEPSTATE : gets the state of eeprom (1 = ready, 0 = write pending)
- disable motDebug also in DEV to prevent handling UART in interrupt routines
- set default motor low / high to 17 (1.7) 

## [1.0.9]
### Fixed
- UART : max read length for UART buffer is limited to 990 now, not unlimited anymore
- valves : valve_loop() is now called via timer interrupt, this hopefully ensures correct motor drive and stopping during high system loads

## [1.0.8]
### Changed
- Softreset timing changed

## [1.0.7] 
### Added
- new STM32 commands

### Fixed
- fixed problems with configuration of 1-wire sensors (matching to valves) 
