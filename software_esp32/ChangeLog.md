# Changelog
All notable changes to this project will be documented in this file.

### Breaking Changed

### Changed

### Fixed

### Removed

## [Unreleased] - Development

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
