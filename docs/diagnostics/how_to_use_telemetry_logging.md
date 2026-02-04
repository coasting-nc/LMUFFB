# How to Use Telemetry Logging

**Version:** 0.7.9+  
**Purpose:** Record FFB data for diagnostics, tuning, and analysis

---

## Overview

The Telemetry Logger captures high-frequency FFB data to CSV files for offline analysis. It runs asynchronously at 100Hz (decimated from the 400Hz FFB loop) and records 40+ physics channels without impacting your FFB feel.

**Use Cases:**
- Debugging slope detection behavior
- Analyzing grip loss patterns
- Tuning FFB settings for specific cars/tracks
- Investigating clipping or oscillation issues
- Sharing diagnostic data with developers

---

## Quick Start

### Manual Logging

1. **Start a Session:** Load into a car and track in LMU
2. **Open lmuFFB:** The main window should be visible
3. **Start Recording:** Click the **"START LOGGING"** button in the Advanced Settings section
4. **Drive:** The logger will record all FFB calculations in real-time
5. **Mark Events:** Press the **"MARKER"** button during interesting moments (e.g., during a slide)
6. **Stop Recording:** Click **"STOP LOG"** when done

**Recording Indicator:** When logging is active, you'll see a blinking red **"REC"** indicator and a frame count.

### Auto-Start Mode

For hands-free logging:

1. Open **Advanced Settings** → **Telemetry Logger**
2. Enable **"Auto-Start on Session"**
3. Set your **Log Path** (default: `logs/`)
4. Click **Save**

Now the logger will automatically:
- **Start** when you leave the pits/menu
- **Stop** when you return to the menu

---

## Log File Format

### Filename Convention

```
lmuffb_log_<DATE>_<TIME>_<CAR>_<TRACK>.csv
```

**Example:**
```
lmuffb_log_2026-02-04_14-30-15_Porsche_911_GT3_R_Spa-Francorchamps.csv
```

### File Structure

#### Header Section (Metadata)

The first ~20 lines contain session information and FFB settings:

```csv
# LMUFFB Telemetry Log v1.0
# App Version: 0.7.9
# ========================
# Session Info
# ========================
# Driver: PlayerName
# Vehicle: Porsche 911 GT3 R
# Track: Spa-Francorchamps
# ========================
# FFB Settings
# ========================
# Gain: 1.0
# Understeer Effect: 1.0
# SoP Effect: 1.666
# Slope Detection: Enabled
# Slope Sensitivity: 0.5
# ...
```

#### Data Section (CSV)

After the header, you'll find comma-separated values with these columns:

| Column | Unit | Description |
|--------|------|-------------|
| `Time` | seconds | Session elapsed time |
| `DeltaTime` | seconds | Frame delta time |
| `Speed` | m/s | Vehicle speed |
| `LatAccel` | m/s² | Lateral acceleration |
| `LongAccel` | m/s² | Longitudinal acceleration |
| `YawRate` | rad/s | Yaw rate |
| `Steering` | -1 to 1 | Steering input |
| `Throttle` | 0 to 1 | Throttle input |
| `Brake` | 0 to 1 | Brake input |
| `SlipAngleFL/FR` | radians | Front left/right slip angle |
| `SlipRatioFL/FR` | ratio | Front left/right slip ratio |
| `GripFL/FR` | 0 to 1 | Front left/right grip fraction |
| `LoadFL/FR` | Newtons | Front left/right tire load |
| `CalcSlipAngle` | radians | Calculated front slip angle |
| `CalcGripFront` | 0 to 1 | Calculated front grip |
| `CalcGripRear` | 0 to 1 | Calculated rear grip |
| `GripDelta` | -1 to 1 | Front - Rear grip |
| `dG_dt` | G/s | Rate of change of lateral G |
| `dAlpha_dt` | rad/s² | Rate of change of slip angle |
| `SlopeCurrent` | G/rad | dG/dAlpha ratio |
| `SlopeSmoothed` | 0 to 1 | Smoothed grip output |
| `Confidence` | 0 to 1 | Slope confidence factor |
| `FFBTotal` | -1 to 1 | Final normalized FFB output |
| `FFBBase` | -1 to 1 | Base steering shaft force |
| `FFBSoP` | -1 to 1 | Seat of Pants force |
| `GripFactor` | 0 to 1 | Applied grip modulation |
| `SpeedGate` | 0 to 1 | Speed gate factor |
| `Clipping` | 0/1 | Output clipping flag |
| `Marker` | 0/1 | User-triggered marker |

---

## Analysis Tips

### Using Excel/Google Sheets

1. Open the CSV file
2. **Filter by Marker:** Set `Marker` column to show only `1` to find tagged events
3. **Plot Time Series:** Create line charts with `Time` on X-axis
4. **Compare Channels:** Plot multiple columns (e.g., `CalcGripFront` vs `SlopeSmoothed`)

### Using Python/Pandas

```python
import pandas as pd
import matplotlib.pyplot as plt

# Load log file (skip comment lines starting with #)
df = pd.read_csv('lmuffb_log_2026-02-04_14-30-15_Porsche_911_GT3_R_Spa.csv', comment='#')

# Plot grip vs time
plt.figure(figsize=(12, 6))
plt.plot(df['Time'], df['CalcGripFront'], label='Front Grip')
plt.plot(df['Time'], df['CalcGripRear'], label='Rear Grip')
plt.xlabel('Time (s)')
plt.ylabel('Grip (0-1)')
plt.legend()
plt.grid(True)
plt.show()

# Find clipping events
clipping_frames = df[df['Clipping'] == 1]
print(f"Clipping occurred {len(clipping_frames)} times")

# Analyze markers
markers = df[df['Marker'] == 1]
print(f"User marked {len(markers)} events at times: {markers['Time'].tolist()}")
```

### Common Analysis Tasks

#### 1. **Diagnose Slope Detection Issues**

Plot these channels together:
- `dG_dt` (lateral G derivative)
- `dAlpha_dt` (slip angle derivative)
- `SlopeCurrent` (dG/dAlpha ratio)
- `SlopeSmoothed` (final grip output)
- `Confidence` (confidence factor)

**What to look for:**
- `SlopeCurrent` should be positive during normal cornering
- Negative `SlopeCurrent` indicates sliding
- Low `Confidence` means the algorithm is uncertain (low signal strength)

#### 2. **Find Clipping**

```python
# Percentage of frames with clipping
clipping_pct = (df['Clipping'].sum() / len(df)) * 100
print(f"Clipping: {clipping_pct:.2f}% of frames")
```

If clipping is > 5%, reduce your **Gain** setting.

#### 3. **Analyze Understeer vs Oversteer**

```python
# Grip delta: positive = understeer, negative = oversteer
understeer_frames = df[df['GripDelta'] > 0.1]
oversteer_frames = df[df['GripDelta'] < -0.1]

print(f"Understeer: {len(understeer_frames)} frames")
print(f"Oversteer: {len(oversteer_frames)} frames")
```

#### 4. **Speed Gate Analysis**

Plot `SpeedGate` vs `Speed` to see how the speed gate ramps up:

```python
plt.scatter(df['Speed'], df['SpeedGate'], alpha=0.3)
plt.xlabel('Speed (m/s)')
plt.ylabel('Speed Gate (0-1)')
plt.title('Speed Gate Curve')
plt.grid(True)
plt.show()
```

---

## Performance & File Size

### Recording Rate

- **FFB Loop:** 400Hz (2.5ms per frame)
- **Logging Rate:** 100Hz (10ms per frame, 4x decimation)
- **Overhead:** < 5 microseconds per frame (negligible)

### File Size Estimates

| Duration | Approximate Size |
|----------|------------------|
| 5 minutes | ~600 KB |
| 30 minutes | ~3.6 MB |
| 1 hour | ~7.2 MB |
| 2 hours | ~14.4 MB |

**Note:** Files are flushed to disk every 5 seconds to minimize data loss in case of a crash.

---

## Troubleshooting

### "Log file not created"

**Possible causes:**
1. **Invalid log path:** Check that the directory exists and you have write permissions
2. **Disk full:** Ensure you have sufficient disk space
3. **File already open:** Close any programs that might have the log file open

**Solution:** Try changing the **Log Path** to a different location (e.g., `C:\Temp\lmuffb_logs\`)

### "Recording stopped unexpectedly"

**Possible causes:**
1. **Disk full:** The logger stops if it can't write to disk
2. **Application crash:** Unflushed data may be lost (up to 5 seconds)

**Solution:** Check disk space and ensure the log path is writable

### "File size is very large"

**Cause:** Long recording sessions generate large files

**Solutions:**
1. Record shorter sessions
2. Use the **Marker** feature to tag specific events, then filter the CSV later
3. Consider using Auto-Start mode to automatically stop when you return to the menu

### "Can't open CSV in Excel"

**Cause:** Excel may struggle with files > 50MB or files with many rows

**Solutions:**
1. Use a text editor (e.g., Notepad++) to view the file
2. Use Python/Pandas for analysis
3. Split the file into smaller chunks

---

## Advanced Usage

### Custom Log Path

You can specify a custom directory for log files:

1. Open **Advanced Settings** → **Telemetry Logger**
2. Set **Log Path** to your desired directory (e.g., `D:\Racing\Logs\`)
3. Click **Save**

**Note:** The directory will be created automatically if it doesn't exist.

### Batch Processing

If you have multiple log files, you can process them in batch using Python:

```python
import glob
import pandas as pd

# Find all log files
log_files = glob.glob('logs/*.csv')

for log_file in log_files:
    df = pd.read_csv(log_file, comment='#')
    
    # Extract session info from filename
    parts = log_file.split('_')
    car = parts[5]
    track = parts[6].replace('.csv', '')
    
    # Analyze...
    clipping_pct = (df['Clipping'].sum() / len(df)) * 100
    print(f"{car} @ {track}: {clipping_pct:.2f}% clipping")
```

### Sharing Logs with Developers

If you encounter an issue and want to share diagnostic data:

1. Record a session with the issue
2. Use **Marker** to tag the exact moment the issue occurs
3. Stop recording
4. Locate the log file in your **Log Path** directory
5. Share the file (e.g., via Discord, GitHub issue, or email)

**Privacy Note:** Log files contain car/track names and timestamps, but no personal information.

---

## FAQ

**Q: Does logging affect FFB performance?**  
A: No. The logger uses a separate thread and adds < 5 microseconds of overhead per frame, which is negligible.

**Q: Can I log while racing online?**  
A: Yes, but be aware that large log files may accumulate over time. Use Auto-Start mode to automatically stop logging when you return to the menu.

**Q: How do I delete old log files?**  
A: Navigate to your **Log Path** directory and delete the `.csv` files manually. The logger does not auto-delete old files.

**Q: Can I change the logging rate?**  
A: Currently, the logging rate is fixed at 100Hz (4x decimation). Future versions may add a configurable decimation factor.

**Q: What's the difference between `CalcGripFront` and `SlopeSmoothed`?**  
A: `CalcGripFront` is the raw grip estimation from telemetry. `SlopeSmoothed` is the grip output from the slope detection algorithm (if enabled), which uses derivatives to detect sliding.

---

## Version History

- **v0.7.9:** Initial release of telemetry logger
  - 100Hz CSV logging
  - 40+ physics channels
  - Auto-start mode
  - User markers
  - File size monitoring

---

**Need Help?**  
Visit the [lmuFFB GitHub repository](https://github.com/coasting-nc/LMUFFB) or ask in the Discord community.
