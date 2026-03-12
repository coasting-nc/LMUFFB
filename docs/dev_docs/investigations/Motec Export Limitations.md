# MoTeC Export Limitations

The current implementation of the MoTeC exporter has several limitations that should be addressed in future versions of the LMUFFB logger and analyzer.

## 1. Missing Lap Marker Data
The current `.bin` telemetry format (v0.7.129+) does not include a `LapNumber` or `CompletedLaps` field.
- **Impact**: The exported `.ldx` file cannot automatically place lap beacons at the exact start/finish line crossings.
- **Current Workaround**: The exporter adds a single beacon at `Time=0.0`. Users must manually split laps in MoTeC i2 Pro using "Track Map" or manual beacon placement.
- **Recommended Fix**: Add `mCompletedLaps` from the `VehicleScoringInfoV01` struct to the binary log frame.

## 2. Fixed Resampling
MoTeC requires a constant sampling frequency. The exporter uses linear interpolation to resample the variable-rate log data to 100Hz (configurable).
- **Impact**: Extremely high-frequency transients (above 50Hz) might be slightly smoothed by the interpolation.
- **Current Workaround**: Users can increase the export frequency with the `-f` flag (e.g., `-f 400`), but this significantly increases file size.

## 3. 16-bit Integer Compression
To maintain compatibility and reduce file size, most channels are compressed into 16-bit signed integers.
- **Impact**: Some loss of precision may occur if the scaling factors are not perfectly tuned for the data range.
- **Current Workaround**: Default scaling factors are tuned based on typical racing telemetry ranges (e.g., Speed 0.1, G-force 0.001).

## 4. Limited Channel Set
Only a subset of the available 120+ telemetry fields are currently mapped to MoTeC channels.
- **Impact**: Some advanced debugging fields (e.g., specific FFB algorithm internal states) are not yet exported.
- **Current Workaround**: Users can manually add channel mappings to `MotecExporter.channel_configs` in `tools/lmuffb_log_analyzer/exporters/motec_exporter.py`.
