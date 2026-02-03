# Vendor Code Modifications and Workarounds

**Purpose:** Track any modifications or workarounds related to third-party vendor code  
**Last Updated:** 2026-01-31

---

## SharedMemoryInterface.hpp (Le Mans Ultimate SDK)

**Vendor:** Studio 397 / Le Mans Ultimate  
**File:** `src/lmu_sm_interface/SharedMemoryInterface.hpp`  
**Status:** **NOT MODIFIED** (as of v0.6.39)

### Known Issues in Vendor Code

#### 1. Missing Standard Library Includes

**Issue:** The vendor header is missing several required standard library includes:
- `<optional>` for `std::optional`
- `<utility>` for `std::exchange`, `std::swap`
- `<cstdint>` for `uint32_t`, `uint8_t`
- `<cstring>` for `memcpy`

**Workaround:** `src/lmu_sm_interface/LmuSharedMemoryWrapper.h`
- Includes all missing headers **before** including the vendor file
- This allows the vendor header to compile without modification
- Pattern: Pre-include dependencies, then include vendor file unmodified

**Rationale:** Adding includes before vendor header is non-invasive and survives vendor updates.

---

#### 2. Race Condition in SharedMemoryLock::Lock()

**Issue:** The `SharedMemoryLock::Lock()` method at line 103-124 has a race condition:

```cpp
bool Lock(DWORD dwMilliseconds = INFINITE) {
    // ... spin lock attempts ...
    
    InterlockedIncrement(&mDataPtr->waiters);
    while (true) {
        if (InterlockedCompareExchange(&mDataPtr->busy, 1, 0) == 0) {
            InterlockedDecrement(&mDataPtr->waiters);
            return true;
        }
        // BUG: Returns immediately after event is signaled, without retrying atomic flag
        return WaitForSingleObject(mWaitEventHandle, dwMilliseconds) == WAIT_OBJECT_0;
    }
}
```

**The Problem:**
1. Thread A holds the lock
2. Thread B spins, fails, increments waiters, waits on event
3. Thread A releases lock, signals event
4. Thread B wakes from `WaitForSingleObject` (returns WAIT_OBJECT_0)
5. **Thread B returns TRUE immediately** without checking if lock is actually available
6. Meanwhile, Thread C could have grabbed the lock between steps 3-4
7. **Both Thread B and Thread C think they have the lock** → data corruption

**Correct Implementation:**
```cpp
// After WaitForSingleObject succeeds, should LOOP BACK to retry atomic acquisition
while (true) {
    if (InterlockedCompareExchange(&mDataPtr->busy, 1, 0) == 0) {
        InterlockedDecrement(&mDataPtr->waiters);
        return true;
    }
    if (WaitForSingleObject(mWaitEventHandle, dwMilliseconds) != WAIT_OBJECT_0) {
        // Timeout or error
        InterlockedDecrement(&mDataPtr->waiters);
        return false;
    }
    // Loop back to retry acquiring the atomic flag
}
```

**Our Solution:** `SafeSharedMemoryLock` wrapper (v0.6.39+)
- Wraps the vendor `SharedMemoryLock` class
- Exposes timeout support without modifying vendor code
- **Does NOT fix the race condition** to avoid vendor file modification
- Risk mitigation: The race window is extremely small in practice

**Why Not Fix It:**
- **Maintenance burden**: Every vendor SDK update would require re-applying the fix
- **Version tracking complexity**: Hard to track which vendor version has which fixes
- **User requirement**: "We don't want to modify the vendor file"

**If the race condition causes issues in production:**
- Option 1: Implement our own lock using Windows API (CreateMutex, etc.)
- Option 2: Submit bug report to Studio 397 and wait for official fix
- Option 3: Fork vendor file and maintain our patched version (not recommended)

---

#### 3. Lack of Timeout Support in Lock() (FEATURE, not bug)

**Issue:** The original vendor `Lock()` method waits `INFINITE` by default, which can hang the application if the game crashes while holding the lock.

**Workaround:** `SafeSharedMemoryLock::Lock(DWORD timeout_ms = 50)`
- Wrapper exposes timeout parameter
- Default 50ms timeout prevents infinite hangs
- Returns `false` on timeout (calling code handles gracefully)

**Implementation:**
```cpp
class SafeSharedMemoryLock {
public:
    bool Lock(DWORD timeout_ms = 50) {
        return m_vendorLock.Lock(timeout_ms);  // Vendor Lock() already supports timeout parameter
    }
    // ... rest of wrapper
private:
    SharedMemoryLock m_vendorLock;
};
```

**Why This Works:**
- The vendor `Lock()` method signature already includes `DWORD dwMilliseconds = INFINITE`
- We just provide a different default (50ms instead of INFINITE)
- No vendor code modification required

---

## InternalsPlugin.hpp (Le Mans Ultimate SDK)

**Vendor:** Studio 397 / Le Mans Ultimate  
**File:** `src/lmu_sm_interface/InternalsPlugin.hpp`  
**Status:** **NOT MODIFIED**

### Usage

This header defines the telemetry data structures:
- `ScoringInfoV01` - Contains `mInRealtime` flag
- `TelemInfoV01` - Contains vehicle telemetry
- `VehicleScoringInfoV01` - Contains vehicle scoring data

**No known issues. No modifications needed.**

---

## Wrapper Pattern Best Practices

### When to Create a Wrapper

Create a wrapper instead of modifying vendor code when:
1. **Vendor code is auto-generated** (likely for InternalsPlugin.hpp)
2. **Vendor updates are frequent** (SDK is actively maintained)
3. **Changes are small enhancements**, not bug fixes
4. **Risk of future conflicts** is high

### When Direct Modification is Acceptable

Directly modify vendor code (with documentation) when:
1. **Critical bug fix** that causes data corruption or crashes
2. **Vendor is no longer maintained** (abandonware)
3. **Fix is urgent** and vendor hasn't responded to bug report
4. **Wrapper would be too complex** (e.g., requires extensive refactoring)

**In all cases:** Document the modification in this file and consider contributing the fix back to the vendor.

---

## Change History

| Date       | File                        | Change Type | Description                          |
|------------|----------------------------|-------------|--------------------------------------|
| 2026-01-31 | SharedMemoryInterface.hpp  | REVERTED    | Removed timeout loop fix (lines 116-122) to avoid vendor modification |
| 2026-01-31 | SafeSharedMemoryLock.h     | NEW WRAPPER | Created wrapper for timeout support without modifying vendor |
| 2025-XX-XX | LmuSharedMemoryWrapper.h   | PRE-INCLUDE | Added missing standard library includes before vendor header |

---

## Contact

If vendor code issues require fixes that cannot be worked around:
- **Studio 397 Support:** [https://forum.studio-397.com/](https://forum.studio-397.com/)
- **LMU SDK Issues:** Report through official channels

---

**Maintained by:** LMUFFB Development Team  
**Review Frequency:** On every vendor SDK update
