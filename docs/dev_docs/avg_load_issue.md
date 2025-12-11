
This applies for version 0.4.6

### 🟡 Minor Issue: avg_load Dependency

**Location:** `FFBEngine.h` - `calculate_grip()` function

**Issue:** The fallback logic depends on `avg_load` which is calculated from front wheels only. This means:
- Rear grip fallback won't trigger if front wheels are unloaded (even if rear wheels have load)
- This is documented in `AGENTS_MEMORY.md` but not in the code

**Impact:** Low - Most driving scenarios have all wheels loaded

**Recommendation:**
```cpp
// Add comment near calculate_grip() declaration:
// NOTE: avg_load is calculated from front wheels only. 
// Rear fallback requires front wheels to have load (see AGENTS_MEMORY.md §6)
```


## Grip Calculation Logic (v0.4.6)

### Fallback Mechanism
*   **Behavior**: When telemetry grip (`mGripFract`) is 0.0 but load is present, the engine approximates grip from slip angle.
*   **Front vs Rear**: As of v0.4.6, this logic applies to BOTH front and rear wheels.
*   **Constraint**: The fallback triggers if `avg_grip < 0.0001` AND `avg_load > 100.0`.
    *   *Gotcha*: `avg_load` is currently calculated from **Front Wheels Only**. This means rear fallback depends on front loading. This works for most cases (grounded car) but requires care in synthetic tests (must set front load even when testing rear behavior).

### Diagnostics
*   **Struct**: `GripDiagnostics m_grip_diag` tracks whether approximation was used and the original values.
*   **Why**: Original telemetry values are overwritten by the fallback logic. To debug or display "raw" data, use `m_grip_diag.original` instead of the modified variables.
