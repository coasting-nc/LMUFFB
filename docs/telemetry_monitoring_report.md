# Investigation Report: Telemetry Sample Rate Monitoring

## Current State of Telemetry Monitoring

The LMUFFB application currently monitors telemetry update rates in the `FFBThread` located in `src/main.cpp`. The following `RateMonitor` instances are used:

- `loopMonitor`: Tracks the frequency of the FFB loop itself (target 400Hz).
- `telemMonitor`: Tracks updates to `pPlayerTelemetry->mElapsedTime`.
- `hwMonitor`: Tracks successful hardware force updates via DirectInput.
- `torqueMonitor`: Tracks updates to `pPlayerTelemetry->mSteeringShaftTorque`.
- `genTorqueMonitor`: Tracks updates to `g_localData.generic.FFBTorque`.

Telemetry is updated by comparing the current value of a field with its last seen value. If they differ, an event is recorded in the corresponding `RateMonitor`.

## Data Structures

Telemetry data is stored in `SharedMemoryObjectOut` (defined in `src/lmu_sm_interface/SharedMemoryInterface.hpp`), which contains:
- `SharedMemoryTelemtryData telemetry`: Contains an array of `TelemInfoV01` structures.
- `SharedMemoryGeneric generic`: Contains `FFBTorque`.

`TelemInfoV01` (defined in `src/lmu_sm_interface/InternalsPlugin.hpp`) contains many fields that could potentially be updated at different rates (e.g., some at 100Hz, some at 400Hz).

## Logging Mechanism

The `Logger` class (defined in `src/Logger.h`) provides a synchronous logging mechanism that:
1. Prepends a timestamp.
2. Writes to a debug log file (`lmuffb_debug.log`).
3. Flushes the file immediately.
4. Prints the message to `std::cout` (prefixed with `[Log]`).

## Identified Channels for Monitoring

To gain a better understanding of LMU's telemetry output, the following additional channels from `TelemInfoV01` should be monitored:

- `mLocalAccel` (X, Y, Z)
- `mLocalVel` (X, Y, Z)
- `mLocalRot` (X, Y, Z)
- `mUnfilteredSteering`
- `mFilteredSteering`
- `mEngineRPM`
- `mWheel[0..3].mTireLoad`
- `mWheel[0..3].mLateralForce`
- `mPos` (to check if world position updates faster)

Additionally, `g_localData.generic.events[SME_UPDATE_TELEMETRY]` itself could be monitored to see how often the game signals a telemetry update event.

## Planned Implementation Strategy

1. Extend `FFBThread` in `main.cpp` with additional `RateMonitor`s and state variables to track the identified channels.
2. Implement a periodic logging block (e.g., every 5-10 seconds) that outputs the rates of all monitored channels using `Logger::Get().Log`.
3. Ensure the existing "Low Sample Rate" warning remains but potentially incorporates more data if useful.
4. Add a way to see these rates in the GUI if possible, though the primary requirement is logging. (The user only asked for logging for now).
