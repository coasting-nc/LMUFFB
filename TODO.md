# LMUFFB - Development Roadmap

## High Priority

### Core Functionality
- [ ] **Configuration Persistence**
  - Add "Save Configuration" button to export current slider settings to JSON
  - Add "Reset to Defaults" button to restore initial app settings
  - Implement auto-load of last used configuration on startup
  - Store configurations in `%APPDATA%/LMUFFB/` directory

- [ ] **User Feedback & Error Handling**
  - Display game connection errors as GUI notifications (when GUI available/installed) in addition to console output 
  - Show modal dialog when game is not running (if GUI enabled)
  - Display connection status indicator in the GUI window
  - Implement error retry mechanism with visual feedback
  - At startup, still show the main GUI window with the configuration sliders even if the game (LMU or rF2) is not running. Just Display the error popup (game not running) on top.

- [ ] **Change Log**
  - Create `CHANGELOG.md` file tracking all releases and major commits
  - Follow semantic versioning (v1.0.0 format)
  - Document new features, bug fixes, and breaking changes per release

## Medium Priority

### UI Enhancements
- [ ] Add tooltips to all GUI sliders explaining their purpose
- [ ] Implement presets system (e.g., "Street", "Racing", "Smooth" profiles)
- [ ] Add real-time telemetry display window
- [ ] Status bar showing game connection state and FFB output level

### Performance & Stability
- [ ] Profile and optimize FFB calculation loop
- [ ] Add configurable FFB update rate (currently ~400Hz)
- [ ] Implement graceful shutdown when game disconnects
- [ ] Add logging system for debugging (optional file output)

## Lower Priority

### Advanced Features
- [ ] DirectInput wheel support (alternative to vJoy)
- [ ] Custom effect sequencing
- [ ] Telemetry recording and playback
- [ ] Network multiplayer FFB sharing
- [ ] Integration with other racing simulators (iRacing, Assetto Corsa, etc.)

## Completed

- [x] C++ port from Python prototype
- [x] Multi-threaded FFB and GUI loops
- [x] ImGui integration with real-time sliders
- [x] Cross-platform vJoy library detection (32-bit / 64-bit)
- [x] Automated DLL deployment in build process
- [x] Installation documentation for end users