# Build Fixes Applied

**Date:** 2025-12-16T00:42:37+01:00

---

## Issues Fixed

### 1. ✅ Test Compilation Error - Non-existent Field

**Error:**
```
tests\test_ffb_engine.cpp(2584): error C2039: 'mSpeed': is not a member of 'TelemInfoV01'
tests\test_ffb_engine.cpp(2656): error C2039: 'mSpeed': is not a member of 'TelemInfoV01'
```

**Problem:** The test code incorrectly used `data.mSpeed` which doesn't exist in `TelemInfoV01`.

**Fix:** Removed the non-existent `data.mSpeed` assignments. The struct only has `mLocalVel.z` for car speed.

**Files Modified:**
- `tests/test_ffb_engine.cpp` (Lines 2584, 2656)

**Changes:**
```cpp
// BEFORE (WRONG):
data.mSpeed = 50.0f; // Fast (50 m/s)
data.mLocalVel.z = 50.0; // Car speed

// AFTER (CORRECT):
data.mLocalVel.z = 50.0; // Car speed (50 m/s)
```

---

### 2. ✅ Type Truncation Warning

**Warning:**
```
C:\dev\personal\LMUFFB_public\LMUFFB\FFBEngine.h(293,58): warning C4305: '=': truncation from 'const double' to 'float'
```

**Problem:** Assigning `double` constant `DEFAULT_STEERING_RANGE_RAD` to `float` variable `range` without explicit cast.

**Fix:** Added explicit `(float)` cast to eliminate warning.

**Files Modified:**
- `FFBEngine.h` (Line 695)

**Changes:**
```cpp
// BEFORE (WARNING):
if (range <= 0.0f) range = DEFAULT_STEERING_RANGE_RAD;

// AFTER (NO WARNING):
if (range <= 0.0f) range = (float)DEFAULT_STEERING_RANGE_RAD;
```

---

## Build Status

### Tests
- ✅ Should now compile without errors
- ✅ All type issues resolved

### Main Application
- ✅ Should now build without warnings
- ✅ Type truncation warning eliminated

---

## Next Steps

You can now rebuild:

```powershell
# Rebuild tests
cl /EHsc /std:c++17 /I.. tests\test_ffb_engine.cpp src\Config.cpp /Fe:tests\test_ffb_engine.exe

# Rebuild main app
cmake --build build --config Release --clean-first
```

Both should now complete successfully without errors or warnings.

---

**Fixes Applied:** 2025-12-16T00:42:37+01:00  
**Status:** ✅ Ready to rebuild
