The proposed code change is a comprehensive and high-quality solution to Issue #207. It correctly addresses all primary and secondary requirements outlined in the user's request and subsequent interactions.

### Analysis and Reasoning:

1.  **User's Goal:** The objective is to disable session-learned dynamic normalization by default for both structural forces and tactile haptics, while providing UI toggles for users to re-enable them. Additionally, the system must ensure that "learned" values are reset when disabling the feature or switching vehicles.

2.  **Evaluation of the Solution:**
    *   **Core Functionality:** The patch introduces two independent toggles (`m_dynamic_normalization_enabled` and `m_auto_load_normalization_enabled`), both defaulting to `false`. The core FFB calculation logic in `FFBEngine::calculate_force` correctly gates the peak-following logic behind these flags. When disabled, the engine falls back to using the manual `m_target_rim_nm` for structural forces and class-based default seeds for tactile haptics, ensuring consistent and predictable FFB scaling.
    *   **Safety & Side Effects:** The implementation is safe. It uses proper thread synchronization (`g_engine_mutex`) in the new `ResetNormalization` method. It prevents division-by-zero using `EPSILON_DIV`. Importantly, it preserves the specialized normalization logic for "In-Game FFB" (source 1), which uses the wheelbase max torque.
    *   **Completeness:** The patch is exceptionally complete. It includes:
        *   Engine logic updates.
        *   Configuration persistence (INI parsing/saving and Preset management).
        *   UI updates (checkboxes and descriptive tooltips).
        *   A robust reset mechanism (`ResetNormalization`) called during UI toggles and vehicle changes, which resolves the "cross-car pollution" issue where peaks from one car would affect another.
        *   Full documentation (Changelogs, VERSION increment, and a detailed Implementation Plan).
    *   **Testing:** The developer followed the strict instruction to provide "disabled" versions of every test that was modified to enable normalization. This ensures both legacy behavior and the new default behavior are verified. New tests were also added to specifically verify toggle behavior and configuration persistence.

3.  **Merge Assessment:**
    *   **Blocking:** None. The patch meets all functional, procedural, and documentation requirements.
    *   **Nitpicks:** There is a minor redundancy in `InitializeLoadReference` where `ParseVehicleClass` is called twice (once inside `ResetNormalization` and once immediately after), but this occurs only on car changes and has zero impact on performance or correctness.

The solution is professional, maintainable, and thoroughly tested.

### Final Rating: #Correct#