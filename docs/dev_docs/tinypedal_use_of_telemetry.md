
# Question

Look at how the TinyPedal code accesses the telemetry data, in particular mTireLoad (information about the load on each of the four tires of the car). Can it access this value directly from the shared memory map plugin for the game? Does it need to perform some additional calculations or workarounds to access these values?

# Answer


Based on the analysis of the codebase, here is the answer regarding how TinyPedal accesses `mTireLoad`:

### 1. Direct Access
**Yes, the code accesses `mTireLoad` directly** from the shared memory map plugin. It does not perform complex physics calculations to derive this value from other telemetry data (like suspension travel or spring rates) under normal circumstances.

This is defined in **`tinypedal/adapter/rf2_data.py`** within the `Tyre` class:

```python
class Tyre(DataAdapter):
    # ...
    def load(self, index: int | None = None) -> tuple[float, ...]:
        """Tyre load (Newtons)"""
        wheel_data = self.shmm.rf2TeleVeh(index).mWheels
        return (
            rmnan(wheel_data[0].mTireLoad),
            rmnan(wheel_data[1].mTireLoad),
            rmnan(wheel_data[2].mTireLoad),
            rmnan(wheel_data[3].mTireLoad),
        )
```

It reads the `mTireLoad` attribute from the `mWheels` array provided by the shared memory interface (`self.shmm`). The only processing done here is passing it through `rmnan`, which converts `NaN` (Not a Number) or Infinite values to `0.0` to prevent crashes.

### 2. Workarounds and Limitations
While the code *attempts* to read the value directly, there is a specific workaround implemented for cases where the game API fails to provide this data (specifically noted in the changelog for LMGT3 vehicles in Le Mans Ultimate).

In **`tinypedal/widget/weight_distribution.py`**, the code checks if the total tire load is zero. If it is, it falls back to using **Suspension Force** (`mSuspForce`) as a proxy to calculate weight distribution:

```python
    def timerEvent(self, event):
        """Update when vehicle on track"""
        load_fl, load_fr, load_rl, load_rr = api.read.tyre.load()
        total_load = load_fl + load_fr + load_rl + load_rr

        # Fallback to suspension load if tyre load data not avaiable
        if total_load <= 0:
            load_fl, load_fr, load_rl, load_rr = api.read.wheel.suspension_force()
            total_load = load_fl + load_fr + load_rl + load_rr
```

### Summary
1.  **Primary Method:** Direct read of `mTireLoad` from the shared memory struct.
2.  **Processing:** Minimal (sanitization of invalid numbers).
3.  **Fallback:** If `mTireLoad` is missing (0), the **Weight Distribution** widget substitutes it with `mSuspForce` (Suspension Force). However, the **Tyre Load** widget (`tinypedal/widget/tyre_load.py`) does not appear to have this fallback logic and will likely show 0 if the API data is missing.
