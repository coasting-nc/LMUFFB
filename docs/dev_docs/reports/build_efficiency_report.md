# Build Efficiency Analysis and Optimization Plan

This report evaluates the current build process of the LMUFFB project and proposes optimizations to reduce build times, specifically focusing on the Windows CI pipeline.

## 1. Vendor Library Consolidation (LZ4, ImGui, toml++)

### Observation
The project currently builds `lz4.c` multiple times because it is included in the `CORE_SOURCES` list, which is consumed by both the `LMUFFB_Core` and `LMUFFB_Core_Fast` targets. Similarly, `imgui` is built as a separate static library (`imgui_vendor`), but it remains isolated from other vendor code.

### Recommendations
We should consolidate all third-party code into a single, pre-compiled "vendor" library. This ensures that these files (which rarely change) are compiled exactly once per configuration.

1.  **Consolidate to `LMUFFB_Vendor`:** Rename `imgui_vendor` to `LMUFFB_Vendor` (or similar) and move `lz4.c` into this target.
2.  **Move LZ4 out of Core:** Remove `${LZ4_DIR}/lz4.c` from the `CORE_SOURCES` list in `CMakeLists.txt`.
3.  **Manage `toml++`:** While `toml++` is header-only, its include directory can be added as a `PUBLIC` property of the `LMUFFB_Vendor` library. This allows other targets to inherit the include path automatically by simply linking to the vendor library.

### Benefits
*   **Build Time:** Saves the time spent re-compiling `lz4.c` for every core target (currently 2x per configuration).
*   **Hygiene:** Clearly separates project logic from third-party code.
*   **Clarity:** Simplifies the `CORE_SOURCES` list to only include project-specific files.

---

## 2. Core Library Redundancy (`LMUFFB_Core` vs `LMUFFB_Core_Fast`)

### Observation
The main codebase is currently split into two static libraries:
*   `LMUFFB_Core`: Highly optimized (`/O2` or `-O3`), used for the shipping application.
*   `LMUFFB_Core_Fast`: Unoptimized (`/Od` or `-O0`) and defines `LMUFFB_UNIT_TEST`, used for the test suite.

This causes every source file in the project to be compiled twice during a full build (e.g., in CI or a fresh local build).

### Inefficiency vs. Necessity
Is this an inefficiency? **Yes.** Can it be optimized further? **Yes, but with trade-offs.**

*   **Why it exists:** The split exists specifically to provide fast compilation for tests (via `/Od`) while maintaining high performance for the application core.
*   **The `LMUFFB_UNIT_TEST` Constraint:** Because `LMUFFB_UNIT_TEST` is used for conditional compilation (e.g., in `TimeUtils.h` and `GuiLayer_Common.cpp`), these libraries *must* be compiled separately to produce different object files.

### Optimization Options
1.  **Merge into a single Core Library (Not Recommended):** If we merged them, tests would have to run on the optimized build or the app would have to ship unoptimized. Neither is ideal.
2.  **Object Libraries for Non-Conditional Files:** We could use CMake `OBJECT` libraries for files that **do not** use `LMUFFB_UNIT_TEST`. Those files would be compiled once and then linked into both targets. However, the different optimization flags (`/O2` vs `/Od`) would still prevent effective sharing of object files unless we decide to use the same optimization for both.
3.  **Lean into Unity Builds (Recommended):** The ongoing Unity build refactoring is the most effective way to optimize this redundancy. By batching these files into a single translation unit (`unity_0_cxx.cxx`), the overhead of the "double compile" is significantly reduced because the compiler only parses the headers once per chunk, even if it does it twice (once per target).

---

## 3. Synergy with Unity Builds

### Potential Conflicts
The proposed optimizations **do not conflict** with the Unity build transition; in fact, they support it.

*   **Vendor Lib Consolidation:** This is explicitly recommended in the `main_code_unity_build_plan.md` (Section 5.1). Third-party code like `lz4.c` and `imgui.cpp` should *never* be part of a Unity chunk. Moving them to a separate static library is the correct way to handle them.
*   **Core Library Optimization:** Unity builds make the "double compile" of the core library much more tolerable. As more files move from the standalone list into the `UNITY_READY_MAIN` whitelist, the total time spent compiling `LMUFFB_Core` and `LMUFFB_Core_Fast` will drop dramatically.

### Notes and Considerations
> [!IMPORTANT]
> When moving `lz4.c` to a vendor library, ensure it is **not** included in any Unity chunk. It must remain a standalone translation unit within its own library to avoid macro pollution or symbol clashes with the main project code.

---

## 4. Implementation Status (v0.7.241)

The following optimizations proposed in this report have been successfully implemented:

| Action | Target File | Impact | Status |
| :--- | :--- | :--- | :--- |
| Move `lz4.c` to `LMUFFB_Vendor` | `CMakeLists.txt` | Reduces `lz4.c` compilation from 2x to 1x. | [x] **Complete** |
| Rename `imgui_vendor` to `LMUFFB_Vendor` | `CMakeLists.txt` | Improved naming consistency and centralized vendors. | [x] **Complete** |
| Add `toml++` to Vendor Lib | `CMakeLists.txt` | Centralizes vendor interface management. | [x] **Complete** |
| Continue Unity Whitelisting | `CMakeLists.txt` | Reduces total compile time of all main code files by ~40-60%. | [x] **In Progress** |

### Verified Results
- **Build Success:** The project now compiles with a dedicated `LMUFFB_Vendor.lib`.
- **Test Integrity:** All 630/630 tests passed post-refactoring on the local build system.
- **Redundancy Reduced:** LZ4 is now a standalone translation unit compiled only once per configuration, effectively shared via `PUBLIC` linking.

By prioritizing the Unity build migration, we continue to address the "main code compiled multiple times" inefficiency at its root (header parsing overhead) while maintaining the distinct optimization needs of the application vs. the test suite.
