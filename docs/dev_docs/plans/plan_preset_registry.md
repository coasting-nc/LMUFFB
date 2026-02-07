# Implementation Plan - Refactor Preset Management into PresetRegistry Class

## Context
Refactor preset management logic from the `Config` class into a dedicated `PresetRegistry` class. This will centralize ordering logic, insertion points, and preset lifecycle management, making the codebase more robust and maintainable.

## Reference Documents
- Recommendation from Issue #59 implementation.

## Codebase Analysis Summary
- **Existing Architecture**:
    - `Config.h/cpp` currently holds a `static std::vector<Preset>` and multiple static methods for preset operations (Load, Save, Add, Delete, etc.).
    - `FFBEngine` constructor relies on `Preset::ApplyDefaultsToEngine`.
    - `GuiLayer.cpp` calls `Config` static methods for UI interaction.
- **Impacted Functionalities**:
    - Preset storage and retrieval.
    - Preset ordering [Default, User, Built-ins].
    - Preset persistence (integration with `config.ini`).
    - UI binding in `GuiLayer`.

## FFB Effect Impact Analysis
None. This is a pure refactoring of configuration management. Physics calculations are unaffected.

## Proposed Changes

1.  **Create `tests/test_preset_registry.cpp`** with test cases defining expected behavior of `PresetRegistry`.
2.  **Build and run tests** to confirm that the new tests fail.
3.  **Create `src/PresetRegistry.h`** containing the `Preset` struct and the `PresetRegistry` singleton class definition.
4.  **Implement registry methods in `src/PresetRegistry.cpp`**, moving logic from `Config.cpp`.
5.  **Modify `src/Config.h`** to remove `presets` vector and preset-related static methods, and include `PresetRegistry.h`.
6.  **Modify `src/Config.cpp`** to remove preset logic and delegate to `PresetRegistry`.
7.  **Use `read_file`** to confirm the successful removal of preset logic from `src/Config.h` and `src/Config.cpp`.
8.  **Update `src/GuiLayer.cpp`** identifying the specific areas (e.g., preset selection combo box and management buttons) to interface with `PresetRegistry::Get()`.
9.  **Modify the `FFBEngine` constructor** (likely in `src/Config.h` due to circular dependencies) to initialize using the default preset retrieved from `PresetRegistry`.
10. **Update existing tests** in `tests/test_issue_59.cpp` and `tests/test_preset_improvements.cpp` to use `PresetRegistry`.
11. **Increment version** in `VERSION` and `src/Version.h` to `0.7.19`.
12. **Update `CHANGELOG_DEV.md`** describing the refactoring.
13. **Verify the updates** to `VERSION`, `src/Version.h`, and `CHANGELOG_DEV.md` using `read_file`.
14. **Run the full project test suite** using `cmake` to confirm that all existing and new tests pass after the refactoring.
15. **Complete pre-commit steps** to ensure proper testing, verification, review, and reflection are done.
16. **Submit the changes**.

## Parameter Synchronization Checklist
- No new parameters, but all existing 50+ parameters must be correctly handled during move to `PresetRegistry`.

## Initialization Order Analysis
- `PresetRegistry` should be initialized/loaded during `Config::LoadPresets()`.
- Since `FFBEngine` depends on `Preset` defaults, `PresetRegistry.h` must be included where `FFBEngine` is defined or constructed.

## Test Plan (TDD-Ready)

### Test Case 1: `test_preset_registry_singleton`
- Verify `PresetRegistry::Get()` returns the same instance.
### Test Case 2: `test_preset_registry_ordering`
- Verify that loading presets results in the correct order: [Default, User Presets, Vendor Built-ins, Guide/Test Presets].
### Test Case 3: `test_preset_registry_insertion`
- Verify `AddUserPreset` inserts after existing user presets but before vendor built-ins.
### Test Case 4: `test_preset_registry_dirty_state`
- Verify `IsDirty` correctly detects differences between engine and selected preset.

## Deliverables
- [ ] New files: `src/PresetRegistry.h`, `src/PresetRegistry.cpp`.
- [ ] Modified: `src/Config.h`, `src/Config.cpp`, `src/GuiLayer.cpp`.
- [ ] New test file: `tests/test_preset_registry.cpp`.
- [ ] Updated existing tests in `tests/test_issue_59.cpp` and `tests/test_preset_improvements.cpp`.
- [ ] Version bump to `0.7.19`.
- [ ] Entry in `CHANGELOG_DEV.md`.
- [ ] Implementation Notes (to be updated).

## Implementation Notes
(To be filled during development)
