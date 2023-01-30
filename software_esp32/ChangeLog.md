# Changelog
All notable changes to this project will be documented in this file.

### Breaking Changed

### Changed

### Fixed

### Removed

## [Unreleased] - Development

## [1.0.8]
### Added
- forceHardReset added
### Fixed
- web : bugfix calibration settings


## [1.0.7] 
### Added
- new STM32 commands

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
