# Implementation Plan: Reactive Auto-Save Functionality

**Plan Date:** 2025-12-31  
**Target Version:** v0.6.27  
**Status:** ❌ Needs Implementation

## 1. Overview
This plan describes the implementation of a "Reactive Auto-Save" mechanism. The goal is to ensure that every adjustment made in the GUI is persisted to `config.ini` without requiring the user to manually click a save button.

## 2. Technical Strategy

### 2.1. The "Deactivation" Pattern
To avoid excessive disk I/O when dragging sliders, we will use `ImGui::IsItemDeactivatedAfterEdit()`. 
- **During a drag:** Values are updated in memory and the physics engine responds in real-time.
- **On release:** The moment the user lets go of the mouse or finishes a keyboard adjustment, `Config::Save()` is called.

### 2.2. The "Immediate" Pattern
For discrete inputs like Checkboxes and Combo boxes, any change represents a completed intent. These will trigger `Config::Save()` immediately upon the function returning `true`.

## 3. Implementation Details

### 3.1. Update Helper Lambdas in `src/GuiLayer.cpp`

We will modify the core helper lambdas in `GuiLayer::DrawTuningWindow` to include the save trigger.

#### A. `FloatSetting` Update
```cpp
auto FloatSetting = [&](const char* label, float* v, float min, float max, const char* fmt = "%.2f", const char* tooltip = nullptr) {
    ImGui::Text("%s", label);
    ImGui::NextColumn();
    ImGui::SetNextItemWidth(-1);
    std::string id = "##" + std::string(label);
    
    // 1. Standard Slider
    if (ImGui::SliderFloat(id.c_str(), v, min, max, fmt)) {
        selected_preset = -1;
    }
    
    // 2. Trigger Save on Interaction End (Mouse Release or Enter key)
    if (ImGui::IsItemDeactivatedAfterEdit()) {
        Config::Save(engine);
    }

    if (ImGui::IsItemHovered()) {
        // ... (existing arrow key logic)
        if (changed) { 
            *v = (std::max)(min, (std::min)(max, *v)); 
            selected_preset = -1; 
            Config::Save(engine); // Save keyboard adjustments immediately
        }
        // ...
    }
    ImGui::NextColumn();
};
```

#### B. `BoolSetting` Update
```cpp
auto BoolSetting = [&](const char* label, bool* v, const char* tooltip = nullptr) {
    ImGui::Text("%s", label);
    ImGui::NextColumn();
    std::string id = "##" + std::string(label);
    if (ImGui::Checkbox(id.c_str(), v)) {
        selected_preset = -1;
        Config::Save(engine); // Save immediately on toggle
    }
    // ...
};
```

#### C. `IntSetting` Update
```cpp
auto IntSetting = [&](const char* label, int* v, const char* const items[], int items_count, const char* tooltip = nullptr) {
    ImGui::Text("%s", label);
    ImGui::NextColumn();
    std::string id = "##" + std::string(label);
    if (ImGui::Combo(id.c_str(), v, items, items_count)) {
        selected_preset = -1;
        Config::Save(engine); // Save immediately on selection change
    }
    // ...
};
```

### 3.2. Update Individual Controls (Manual Implementation)
Several controls are defined directly in the UI loop, often because they have custom layouts (e.g., latency/color indicators) or exist outside the main grid. These **DO NOT** use the helper lambdas and must be updated individually:

#### A. Custom Sliders (Latency Indicated)
The following sliders use raw `ImGui::SliderFloat` calls to accommodate latency text/color above them. They need `ImGui::IsItemDeactivatedAfterEdit()` and `Config::Save(engine)` added:
- **Steering Shaft Smoothing:** `##ShaftSmooth`
- **Yaw Kick Response (Smoothing):** `##YawSmooth`
- **Gyro Smoothing:** `##GyroSmooth`
- **SoP Smoothing:** `##SoP Smoothing` (Note: Uses Arrow Key logic too)
- **Slip Angle Smoothing:** `##Slip Angle Smoothing` (Note: Uses Arrow Key logic too)
- **Chassis Inertia:** `##ChassisSmooth`

#### B. Advanced Settings
- **Speed Gate (Mute Below/Full Above):** `##Mute Below` (Derived), `##Full Above` (Derived). These use raw `ImGui::SliderFloat`.

#### C. Top Bar Controls
- **"Always on Top" Checkbox**: Already calls `SetWindowAlwaysOnTop`. Add `Config::Save(engine)`.
- **"Graphs" Checkbox**: **ALREADY IMPLEMENTED**. (Calls `Config::Save(engine)`).

### 3.3. Thread Safety Note
Since `DrawTuningWindow` already holds `g_engine_mutex`, calling `Config::Save(engine)` is safe as it accesses the engine state while the mutex is locked.

## 4. Test Specifications

### Test 1: Slider Drag Persistence
1. Start App.
2. Drag "Master Gain" to 1.5. **Do not release the mouse.**
3. Verify `config.ini` has NOT changed yet.
4. Release mouse.
5. Verify `config.ini` now contains `gain=1.5`.

### Test 2: Arrow Key Persistence
1. Hover "Master Gain".
2. Tap Right Arrow.
3. Verify `config.ini` updates immediately.

### Test 3: Toggle Persistence
1. Uncheck "Invert FFB Signal".
2. Verify `config.ini` contains `invert_force=0` immediately.

### Test 4: Custom Slider Persistence (Latency Controls)
1. Drag "SoP Smoothing" slider.
2. Release mouse.
3. Verify `config.ini` updates `sop_smoothing_factor`.

### Test 5: Presets Coexistence
1. Load a preset.
2. Verify the preset name is NOT saved as a global setting (it shouldn't be), but the values within the preset are written to the main section.

## 5. Implementation Roadmap
- [ ] Refactor `FloatSetting` lambda.
- [ ] Refactor `BoolSetting` lambda.
- [ ] Refactor `IntSetting` lambda.
- [ ] Implement Auto-Save for "Steering Shaft Smoothing".
- [ ] Implement Auto-Save for "Yaw Kick Smoothing".
- [ ] Implement Auto-Save for "Gyro Smoothing".
- [ ] Implement Auto-Save for "SoP Smoothing".
- [ ] Implement Auto-Save for "Slip Angle Smoothing".
- [ ] Implement Auto-Save for "Chassis Inertia".
- [ ] Implement Auto-Save for "Speed Gate" sliders (Lower/Upper).
- [ ] Add Auto-Save to "Always on Top".
- [ ] Perform binary size / performance check (disk thrashing test).

## 6. Design Analysis & Refactoring Recommendation

### The "Manual Control" Issue
The need to duplicate the auto-save logic across multiple "manual" controls highlights a minor design smell in the `GuiLayer`. Specifically:
- **Inconsistent Abstraction:** Most controls use the `FloatSetting` abstraction, but significantly complex ones (e.g., those needing latency indicators) drop down to raw `ImGui` calls. All these controls share the same underlying need: *Display Label -> Edit Value -> Save on Completion*.
- **Code Duplication:** The logic for "save on release" and "save on arrow key press" (which is quite verbose) must now be copied to 6+ different locations. This increases the risk of bugs where one slider behaves differently from another.
- **Maintenance Burden:** Future global behavior changes (e.g., "disable editing when game is running") would require updating both the lambda and every individual manual control.

### Recommendation
While out of scope for the immediate Auto-Save implementation, it is recommended to refactor these manual controls into a more capable helper or builder pattern in a future update (e.g., v0.7.0).

**Proposed Solution: `FloatSettingEx`**
Create an extended helper that accepts optional "header" or "decorator" lambdas:
```cpp
auto FloatSettingv2 = [&](const char* label, float* v, ..., std::function<void()> pre_render_callback = nullptr) {
    if (pre_render_callback) pre_render_callback(); // Renders the latency text/color
    // ... Standard Slider Logic ...
    // ... Standard Auto-Save Logic ...
};
```
This would unify behavior across all 20+ sliders in the application, ensuring consistent Auto-Save, Arrow Key, and Tooltip behavior.
