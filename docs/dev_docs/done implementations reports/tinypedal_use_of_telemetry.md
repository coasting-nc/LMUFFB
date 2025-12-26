
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

# Question 2


User
Look if there are any checks about the presence and access to the rFactor2 shared memory plugin. Specifically, if the user has the dll of that plugin in the game folder, and if the configuration json of the game plugins has it enabled and with the other necessary settings enabled.
Refer also to the instructions about this in the readme, and then look at if the code performs checks about this, and if warnings are generated.

# Answer 2


Based on the analysis of the codebase, here is the assessment regarding checks for the rFactor 2 Shared Memory Map Plugin:

### 1. No File System or Configuration Checks
**There are no checks** in the code that verify the physical presence of `rFactor2SharedMemoryMapPlugin64.dll` in the game directory, nor are there any checks that parse the game's `CustomPluginVariables.JSON` file to see if the plugin is enabled.

*   **Code Evidence:** The codebase does not contain any logic to search for the rFactor 2 or Le Mans Ultimate installation paths, nor does it attempt to read files from the game's `UserData` or `Plugins` folders.
*   **Design Approach:** The application operates entirely decoupled from the game's file system. It relies solely on the Windows Shared Memory interface (RAM) to communicate.

### 2. Connection Logic vs. Explicit Warnings
Instead of checking for the plugin files, the application attempts to connect directly to the Shared Memory Map. If the plugin is missing or disabled, the shared memory map will simply not exist in the system's memory.

*   **Behavior:** In `tinypedal/adapter/rf2_connector.py`, the `SyncData` class attempts to create the memory map connection (`self.dataset.create_mmap`).
*   **Failure State:** If the plugin is not running (and thus the memory map is not created by the game), the application does not generate a specific "Plugin Missing" error popup.
*   **Logging:** The application will log the status of the connection.
    *   If the data is not updating (which happens if the plugin is missing, disabled, or the game is paused), the logger in `rf2_connector.py` will output:
        ```text
        sharedmemory: UPDATING: paused, data version [number]
        ```
    *   The main window status bar (handled in `tinypedal/ui/app.py`) displays the API status (e.g., "API: RF2 (unknown)" or similar) based on `api.read.state.version()`, but it does not explicitly diagnose *why* the connection failed.

### 3. Reliance on User Manual (README)
The application relies entirely on the user following the instructions provided in the `README.md` file.

*   **Readme Instructions:** The README explicitly states:
    > "TinyPedal requires The Iron Wolf’s rF2 Shared Memory Map Plugin... The plugin file... should be placed in... Plugins folder."
    > "Enable shared memory map plugin... in game Settings... or manually enable plugin by editing CustomPluginVariables.JSON"
*   **Runtime Assumption:** The code assumes the user has performed these steps. If the user has not, the application simply waits for the shared memory data to appear/update without providing specific feedback that the plugin installation is the root cause.

### Summary
The code **does not perform any validation** of the plugin DLL presence or the JSON configuration. It attempts to read the data stream, and if that stream is absent (due to missing/disabled plugin), the application sits in a "paused" or "waiting" state rather than alerting the user to the specific configuration error.

# Question 3

Tell me more about when  the API fails to provide this data (specifically noted in the changelog for LMGT3 vehicles in Le Mans Ultimate). Is the use of another plugin suggested as a source of that data?

# Answer 3


Based on the analysis of the codebase and documentation, here is the answer regarding the missing data for LMGT3 vehicles:

### No Additional Plugin is Suggested
**No, the code and documentation do not suggest using a different or additional plugin** to retrieve the missing `mTireLoad` data.

Instead of relying on an external plugin, the application implements an **internal software workaround** to calculate the necessary values using different telemetry data that *is* available.

### The Internal Workaround
As noted in the changelog for version **2.34.0**, the specific issue is that the game API returns `0` for tire load on LMGT3 vehicles. TinyPedal handles this within its own code logic:

1.  **Detection:** The code checks if the total tire load returned by the API is less than or equal to zero.
2.  **Fallback:** If zero, it switches to reading **Suspension Force** (`mSuspForce`) instead of **Tire Load** (`mTireLoad`).

This logic is found in **`tinypedal/widget/weight_distribution.py`**:

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

### Technical Distinction
The changelog notes that suspension load is "not entirely the same" as tire load.
*   **Tire Load:** The total force pressing the tire into the ground (includes the weight of the car + aerodynamic downforce + the weight of the wheel/brakes/suspension itself, known as unsprung mass).
*   **Suspension Force:** The force exerted by the springs/dampers holding up the car body (excludes the unsprung mass of the wheels).

While slightly less accurate for absolute physics calculations, Suspension Force is a close enough proxy for calculating **Weight Distribution** percentages when the primary data source is broken.