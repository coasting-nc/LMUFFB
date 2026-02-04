# Preventing Tests from Being Defined but Not Called

In standard C++, there is **no built-in reflection mechanism** that allows a program to inspect itself and say "give me a list of all functions defined in this file."

However, there are three standard ways to solve this problem, ranging from "Compiler Checks" to "Architectural Patterns."

---

## Summary of Options

| Option | Complexity | Prevention | Integration Effort | Best For |
|--------|------------|------------|-------------------|----------|
| **1. Static Keyword** | Zero | Detection only | None | Quick debugging |
| **2. Auto-Registration** | Low-Medium | Full prevention | 2-4 hours | Permanent solution |
| **3. External Script** | Low | Detection (CI/CD) | 30 min | Pipeline validation |

---

## Option 1: The Compiler Warning Method (Easiest)

If your tests are all in the same file as `main()` (which `tests/test_ffb_engine.cpp` is), you can rely on the compiler to tell you if a function is defined but not used.

**The Trick:** You must declare the test functions as `static`.
In C++, `static` on a global function means "this function is only visible in this file." If the compiler sees a `static` function that is never called, it knows it's dead code and will issue a warning.

**How to apply it:**

1.  Change your test definitions:
    ```cpp
    // Old
    void test_my_feature() { ... }

    // New
    static void test_my_feature() { ... }
    ```
2.  Compile.
    *   **MSVC (Windows):** Warning **C4505** ("unreferenced local function has been removed").
    *   **GCC/Clang (Linux):** Warning **-Wunused-function**.

**Pros:** Zero infrastructure code.
**Cons:** You have to manually add `static` to every test.

### Expert Review

**Verdict: Suitable only for debugging, not as a long-term solution.**

This approach is already in place in the current codebase (all test functions are `static`), but it has a fundamental limitation: **it only works within a single translation unit**. Once the test suite was split into 12+ files (v0.7.0), this method became less effective because:

1.  Each `Run_Category()` function is non-static and explicitly called in `Run()`.
2.  The compiler cannot warn about a `static` test function that IS called within its own file but is called from a `Run_Category()` function that itself is never called from `Run()`.

**Conclusion:** Keep using `static` on test functions, but do not rely on it as the primary safeguard.

---

## Option 2: The Auto-Registration Pattern (Recommended)

This is how frameworks like **Google Test** and **Catch2** work. Instead of manually calling functions in `Run()`, you create a system where defining a test *automatically* adds it to a global registry.

Given the new **multi-file structure** of the test suite (v0.7.6+), this pattern requires a **Cross-Unit Singleton** to ensure all tests register to the same list regardless of which `.cpp` file they are in.

### Expert Review

**Verdict: Best long-term solution, with the improvements below.**

The proposed pattern is sound, but the original code sample has several gaps when considering integration with the *existing* LMUFFB test infrastructure:

#### Issues with the Original Proposal

1.  **Missing Tag Support:** The current test system uses `ShouldRunTest(tags, category)` with a `std::vector<std::string>` for tags. The proposed `TestEntry` only stores `name` and `category`.
2.  **Category Derivation:** Using `__FILE__` for category is clever, but the current system uses human-readable category names like `"CorePhysics"`, `"SlopeDetection"`. A raw file path like `c:\dev\...\test_ffb_core_physics.cpp` is not user-friendly for `--category=` filtering.
3.  **No Test Ordering:** The current `Run()` function calls categories in a specific order for readability in output. Auto-registration order is **not guaranteed** (depends on static initialization order across translation units).
4.  **Assertion Count Tracking:** The proposed `Run()` loop only increments `g_tests_failed` on exception. The current system uses `g_tests_passed` and `g_tests_failed` which are incremented by `ASSERT_*` macros *within* the test function, not per-test-function.

#### Improved Architecture

**In `tests/test_ffb_common.h` (The Interface):**
```cpp
#include <vector>
#include <functional>
#include <string>
#include <algorithm>

namespace FFBEngineTests {

struct TestEntry {
    std::string name;
    std::string category;
    std::vector<std::string> tags;
    std::function<void()> func;
    int order_hint; // For sorting within a category
};

class TestRegistry {
public:
    static TestRegistry& Instance();
    void Register(const std::string& name, 
                  const std::string& category, 
                  const std::vector<std::string>& tags,
                  std::function<void()> func,
                  int order = 0);
    const std::vector<TestEntry>& GetTests() const;
    std::vector<TestEntry> GetTestsForCategory(const std::string& category) const;
    void SortByCategory(); // Call once in Run() before iteration

private:
    std::vector<TestEntry> m_tests;
    bool m_sorted = false;
};

// Helper class for static registration
struct AutoRegister {
    AutoRegister(const std::string& name, 
                 const std::string& category, 
                 const std::vector<std::string>& tags,
                 std::function<void()> func,
                 int order = 0) {
        TestRegistry::Instance().Register(name, category, tags, func, order);
    }
};

} // namespace FFBEngineTests

// ============================================================
// Macros to define tests
// ============================================================

// Usage: TEST_CASE(test_my_feature, "CorePhysics", {"Physics", "Regression"})
#define TEST_CASE_TAGGED(test_name, category, tags) \
    static void test_name(); \
    static FFBEngineTests::AutoRegister reg_##test_name( \
        #test_name, category, tags, test_name); \
    static void test_name()

// Simple version without tags (defaults to {"Functional"})
#define TEST_CASE(test_name, category) \
    TEST_CASE_TAGGED(test_name, category, {"Functional"})
```

**In `tests/test_ffb_common.cpp` (The Implementation):**
```cpp
namespace FFBEngineTests {

// Category ordering for consistent output
static const std::vector<std::string> CATEGORY_ORDER = {
    "CorePhysics", "SlopeDetection", "Understeer", "SpeedGate",
    "YawGyro", "Coordinates", "RoadTexture", "Texture",
    "LockupBraking", "Config", "SlipGrip", "Internal"
};

static int GetCategoryOrder(const std::string& cat) {
    auto it = std::find(CATEGORY_ORDER.begin(), CATEGORY_ORDER.end(), cat);
    if (it != CATEGORY_ORDER.end()) {
        return static_cast<int>(std::distance(CATEGORY_ORDER.begin(), it));
    }
    return 999; // Unknown categories go last
}

TestRegistry& TestRegistry::Instance() {
    static TestRegistry instance;
    return instance;
}

void TestRegistry::Register(const std::string& name, 
                            const std::string& category,
                            const std::vector<std::string>& tags,
                            std::function<void()> func,
                            int order) {
    m_tests.push_back({name, category, tags, func, order});
}

void TestRegistry::SortByCategory() {
    if (m_sorted) return;
    std::stable_sort(m_tests.begin(), m_tests.end(), 
        [](const TestEntry& a, const TestEntry& b) {
            int orderA = GetCategoryOrder(a.category);
            int orderB = GetCategoryOrder(b.category);
            if (orderA != orderB) return orderA < orderB;
            return a.order_hint < b.order_hint;
        });
    m_sorted = true;
}

const std::vector<TestEntry>& TestRegistry::GetTests() const {
    return m_tests;
}

// Updated Run() Loop - replaces manual Run_Category() calls
void Run() {
    std::cout << "\n--- FFTEngine Regression Suite ---" << std::endl;
    
    auto& registry = TestRegistry::Instance();
    registry.SortByCategory();
    auto& tests = registry.GetTests();
    
    std::cout << "Running " << tests.size() << " registered tests..." << std::endl;

    std::string current_category;
    for (const auto& test : tests) {
        // Print category header on change
        if (test.category != current_category) {
            current_category = test.category;
            std::cout << "\n=== " << current_category << " Tests ===" << std::endl;
        }
        
        // Integrate with existing Tag Filtering
        if (!ShouldRunTest(test.tags, test.category)) continue;

        try {
            test.func();
        } catch (const std::exception& e) {
            std::cout << "[FAIL] " << test.name << " threw exception: " << e.what() << std::endl;
            g_tests_failed++;
        } catch (...) {
            std::cout << "[FAIL] " << test.name << " threw unknown exception" << std::endl;
            g_tests_failed++;
        }
    }

    std::cout << "\n--- Physics Engine Test Summary ---" << std::endl;
    std::cout << "Tests Passed: " << g_tests_passed << std::endl;
    std::cout << "Tests Failed: " << g_tests_failed << std::endl;
}

} // namespace FFBEngineTests
```

### Migration Example

**Before (current code in `test_ffb_core_physics.cpp`):**
```cpp
static void test_base_force_modes() {
    std::cout << "\nTest: Base Force Modes & Gain (v0.4.13)" << std::endl;
    // ... test body ...
}

void Run_CorePhysics() {
    std::cout << "\n=== Core Physics Tests ===" << std::endl;
    test_base_force_modes();
    test_grip_modulation();
    // ... more tests ...
}
```

**After (with auto-registration):**
```cpp
TEST_CASE_TAGGED(test_base_force_modes, "CorePhysics", {"Physics", "Regression"}) {
    std::cout << "\nTest: Base Force Modes & Gain (v0.4.13)" << std::endl;
    // ... test body unchanged ...
}

TEST_CASE(test_grip_modulation, "CorePhysics") {
    // ... test body unchanged ...
}

// Run_CorePhysics() is DELETED - no longer needed
```

### Incremental Migration Strategy

**Yes, this can be implemented incrementally!** During the transition period, both the old manual `Run_Category()` calls and the new auto-registered tests can coexist. This is the safest approach.

#### Key Insight: Hybrid Run() Function

During migration, modify `Run()` to execute **both** systems:

```cpp
void Run() {
    std::cout << "\n--- FFTEngine Regression Suite ---" << std::endl;
    
    // ========================================
    // PHASE 1: Run legacy (not yet migrated) tests
    // Comment out each line as you migrate that file
    // ========================================
    Run_CorePhysics();       // TODO: Migrate
    Run_SlopeDetection();    // TODO: Migrate
    Run_Understeer();        // TODO: Migrate
    Run_SpeedGate();         // TODO: Migrate
    Run_YawGyro();           // TODO: Migrate
    Run_Coordinates();       // TODO: Migrate
    Run_RoadTexture();       // TODO: Migrate
    Run_Texture();           // TODO: Migrate
    Run_LockupBraking();     // TODO: Migrate
    Run_Config();            // TODO: Migrate
    Run_SlipGrip();          // TODO: Migrate
    Run_Internal();          // TODO: Migrate

    // ========================================
    // PHASE 2: Run auto-registered tests
    // These are tests that have been migrated to TEST_CASE()
    // ========================================
    auto& registry = TestRegistry::Instance();
    if (!registry.GetTests().empty()) {
        registry.SortByCategory();
        auto& tests = registry.GetTests();
        
        std::cout << "\n--- Auto-Registered Tests (" << tests.size() << ") ---" << std::endl;
        
        std::string current_category;
        for (const auto& test : tests) {
            if (test.category != current_category) {
                current_category = test.category;
                std::cout << "\n=== " << current_category << " Tests ===" << std::endl;
            }
            
            if (!ShouldRunTest(test.tags, test.category)) continue;

            try {
                test.func();
            } catch (const std::exception& e) {
                std::cout << "[FAIL] " << test.name << " threw exception: " << e.what() << std::endl;
                g_tests_failed++;
            } catch (...) {
                std::cout << "[FAIL] " << test.name << " threw unknown exception" << std::endl;
                g_tests_failed++;
            }
        }
    }

    std::cout << "\n--- Physics Engine Test Summary ---" << std::endl;
    std::cout << "Tests Passed: " << g_tests_passed << std::endl;
    std::cout << "Tests Failed: " << g_tests_failed << std::endl;
}
```

#### Phase-by-Phase Migration Plan

| Phase | Action | Verification |
|-------|--------|--------------|
| **0. Infrastructure** | Add `TestRegistry`, `TestEntry`, `AutoRegister`, and macros to `common.h/cpp`. Update `Run()` to hybrid mode. | Build succeeds. Test count unchanged (591). |
| **1. Pilot File** | Migrate `test_ffb_internal.cpp` (smallest file, ~10 tests). | Build succeeds. Test count unchanged. |
| **2-12. Remaining Files** | Migrate one file per commit. | After each: build + verify count. |
| **13. Cleanup** | Remove all `Run_Category()` declarations and the hybrid `Run()` code. | Final architecture is clean. |

---

### File-by-File Migration Checklist

Use this checklist to track progress. Migrate in order of increasing complexity:

| Order | File | Tests | Category | Status |
|-------|------|-------|----------|--------|
| 1 | `test_ffb_internal.cpp` | ~10 | Internal | ✅ Migrated & Verified |
| 2 | `test_ffb_coordinates.cpp` | ~15 | Coordinates | ✅ Migrated & Verified |
| 3 | `test_ffb_road_texture.cpp` | ~20 | RoadTexture | ✅ Migrated & Verified |
| 4 | `test_ffb_features.cpp` | ~25 | Texture | ✅ Migrated & Verified |
| 5 | `test_ffb_lockup_braking.cpp` | ~30 | LockupBraking | ✅ Migrated & Verified |
| 6 | `test_ffb_yaw_gyro.cpp` | ~25 | YawGyro | ✅ Migrated & Verified |
| 7 | `test_ffb_smoothstep.cpp` | ~30 | SpeedGate | ✅ Migrated & Verified |
| 8 | `test_ffb_config.cpp` | ~40 | Config | ✅ Migrated & Verified |
| 9 | `test_ffb_understeer.cpp` | ~50 | Understeer | ✅ Migrated & Verified |
| 10 | `test_ffb_slip_grip.cpp` | ~60 | SlipGrip | ✅ Migrated & Verified |
| 11 | `test_ffb_core_physics.cpp` | ~100 | CorePhysics | ✅ Migrated & Verified |
| 12 | `test_ffb_slope_detection.cpp` | ~80 | SlopeDetection | ✅ Migrated & Verified |

**Legend:** ⬜ Not started | 🔄 In progress | ✅ Complete

---

### Step-by-Step Instructions for Each File

For each file in the checklist, follow these steps:

#### Step 1: Record Baseline
```powershell
.\build\tests\Release\run_combined_tests.exe 2>&1 | Select-String "Tests Passed"
# Example output: Tests Passed: 591
```

#### Step 2: Convert Test Functions

**Find (Regex):**
```
static void (test_\w+)\(\) \{
```

**Replace with:**
```
TEST_CASE($1, "CATEGORY_NAME") {
```

> ⚠️ **Important:** Replace `CATEGORY_NAME` with the actual category (e.g., `"CorePhysics"`).

#### Step 3: Delete the `Run_Category()` Function

Remove the entire `Run_Category()` function from the file. For example, delete:
```cpp
void Run_CorePhysics() {
    std::cout << "\n=== Core Physics Tests ===" << std::endl;
    test_base_force_modes();
    test_grip_modulation();
    // ... all calls ...
}
```

#### Step 4: Comment Out the Call in `Run()`

In `test_ffb_common.cpp`, comment out the corresponding line:
```cpp
// Run_CorePhysics();  // MIGRATED to auto-registration
```

#### Step 5: Build and Verify
```powershell
# Build
cmake --build build --config Release

# Run tests
.\build\tests\Release\run_combined_tests.exe 2>&1 | Select-String "Tests Passed"

# Verify count matches baseline (591)
```

#### Step 6: Commit (if using version control)
```powershell
# DO NOT use these commands directly - per GEMINI.md, user handles git
# Just verify the build and tests pass, then inform the user
```

---

### Rollback Strategy

If a migration introduces issues:

1.  **Revert the file** to its previous state (restore `static void test_xxx()` and `Run_Category()`).
2.  **Uncomment** the `Run_Category()` call in `Run()`.
3.  **Rebuild and verify** the test count is restored.

The hybrid `Run()` function makes this safe because un-migrated files continue to work normally.

---

### Final Cleanup (After All Files Migrated)

Once all 12 files are migrated:

1.  **Remove legacy code from `Run()`:** Delete the entire "PHASE 1" block.
2.  **Remove `Run_Category()` declarations from `common.h`:**
    ```cpp
    // DELETE ALL OF THESE:
    void Run_CorePhysics();
    void Run_SlipGrip();
    void Run_Understeer();
    // ... etc.
    ```
3.  **Simplify `Run()`:** The final version should only have the auto-registration loop.
4.  **Bump version:** This is a significant refactor—increment to next minor version (e.g., `0.8.0`).

---

## Implementation Analysis (v0.7.x Split Suite)

Implementing this solution for the current codebase (14 files, ~591 tests) involves the following considerations:

**Complexity: Low-Medium**
*   **Logic:** The registry code itself is minimal (~80 lines with improvements).
*   **Integration:** Requires ensuring correct linking. Since all test files are explicitly listed in `CMakeLists.txt`, the linker will include them, and the auto-registration macros will execute automatically at startup.

**Time Consumption: 3-5 Hours**
*   **Infrastructure:** ~45 minutes to update `common` files (with improved sorting and tag support).
*   **Migration:** ~2-3 hours to convert all 591 tests across 12 files.
*   **Verification:** ~1 hour to ensure all 591 tests still run and tags work.

**Risks: Low**
*   **Static Initialization Order:** The order of test registration is undefined across translation units, but the `SortByCategory()` function ensures consistent ordering in the output.
*   **Build Issues:** Minor risk of linker optimizations stripping "unused" objects. **Mitigation:** Already mitigated by the current CMake configuration, which explicitly includes all test source files.
*   **Dependency:** Creates a harder dependency on `common.h` macros, but simplifies test writing significantly.

---

## Option 3: External Script (The "Linter" Way)

If you don't want to change your C++ code structure, you can use a simple Python script to scan the file. This is often used in CI/CD pipelines.

**`scripts/check_tests.py`**:
```python
import re
import sys
from pathlib import Path

def check_tests():
    tests_dir = Path("tests")
    all_defined = set()
    all_called = set()
    
    # Scan all test files
    for test_file in tests_dir.glob("test_ffb_*.cpp"):
        content = test_file.read_text(encoding="utf-8")
        
        # Find all static void test_xxx() definitions
        defined = set(re.findall(r'static void (test_\w+)\(\)', content))
        all_defined.update(defined)
        
        # Find all test_xxx(); calls within Run_* functions
        run_funcs = re.findall(r'void Run_\w+\(\)[^}]+\{([^}]+)\}', content, re.DOTALL)
        for run_body in run_funcs:
            called = set(re.findall(r'(test_\w+)\(\);', run_body))
            all_called.update(called)
    
    # Also check common.cpp for the Run() master list
    common = (tests_dir / "test_ffb_common.cpp").read_text(encoding="utf-8")
    run_categories = set(re.findall(r'Run_(\w+)\(\);', common))
    
    missing = all_defined - all_called
    
    if missing:
        print("ERROR: The following tests are defined but NOT called:")
        for t in sorted(missing):
            print(f"  - {t}")
        return 1
    else:
        print(f"All {len(all_defined)} tests are called. Categories: {len(run_categories)}")
        return 0

if __name__ == "__main__":
    sys.exit(check_tests())
```

### Expert Review

**Verdict: Good for CI/CD, but does not prevent the issue.**

This approach is excellent for catching issues in a pull request pipeline. However, it has limitations:

1.  **Detection, Not Prevention:** The bug can still be introduced; the script just catches it later.
2.  **Regex Fragility:** The script assumes a specific code pattern (`static void test_xxx()`). Non-conforming code will be missed.
3.  **No Runtime Awareness:** If a test is conditionally compiled out (e.g., `#ifdef WIN32`), the script may produce false positives or negatives.

**Recommendation:** Use this as a *complement* to Option 2, not a replacement.

---

## Final Recommendation for LMUFFB

| Timeframe | Action |
|-----------|--------|
| **Immediate (v0.7.x)** | Continue using `static` on all test functions. This is already in place and provides baseline detection. |
| **Short-Term (v0.8.x)** | Adopt **Option 2 (Auto-Registration)** with the improved architecture above. This should be a dedicated refactoring effort with a version bump. |
| **CI/CD Enhancement** | Add the Python script (Option 3) as a GitHub Actions step to catch any regressions before merge. |

### Why Auto-Registration is the Best Choice

1.  **Eliminates the Root Cause:** You cannot define a test without registering it. The bug is impossible.
2.  **Scales with Codebase:** Adding new test files requires no changes to `Run()` or `common.cpp`.
3.  **Matches Industry Standard:** Google Test, Catch2, and doctest all use this pattern. Developers familiar with those frameworks will feel at home.
4.  **Enables Future Features:**
    *   **Test Discovery:** `--list-tests` flag to print all registered tests.
    *   **Parallel Execution:** Tests can be distributed to workers by name.
    *   **Selective Runs:** `--filter=test_slope*` to run only matching tests.

---

## Alternative Solutions Considered

### Alternative A: Adopt Google Test

**Description:** Replace the custom test framework entirely with Google Test (gtest).

**Pros:**
*   Industry-standard, battle-tested.
*   Provides `TEST_F`, `EXPECT_*` macros, parameterized tests, etc.
*   Auto-registration is built-in.

**Cons:**
*   **Breaking Change:** All ~591 tests would need to be rewritten to use gtest macros.
*   **External Dependency:** Adds gtest as a CMake dependency (FetchContent or submodule).
*   **Overkill:** LMUFFB's current needs are modest; the custom macros work well.

**Verdict:** Not recommended at this time. The custom framework is sufficient, and the migration cost is high.

### Alternative B: Use Catch2

**Description:** Catch2 is a header-only test framework with powerful features.

**Pros:**
*   Single-header inclusion (v2.x) or FetchContent (v3.x).
*   BDD-style `SCENARIO`/`GIVEN`/`WHEN`/`THEN` macros.
*   Excellent output formatting.

**Cons:**
*   Similar breaking change to gtest.
*   Catch2 v3.x requires C++14 (already satisfied).

**Verdict:** A reasonable alternative if a full rewrite is ever planned, but not necessary for this issue.

### Alternative C: CMake-Based Test Discovery

**Description:** Use CMake's `add_test()` with `gtest_discover_tests()` or a custom script to register tests at build time.

**Pros:**
*   Tests are registered in CMakeLists.txt, not code.
*   Works with CTest out of the box.

**Cons:**
*   Requires each test function to be a separate executable (impractical for 591 tests).
*   Doesn't solve the "defined but not called" problem at the source level.

**Verdict:** Not applicable to the current architecture.
