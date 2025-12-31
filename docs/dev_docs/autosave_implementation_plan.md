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

### 3.2. Update Individual Controls
Several controls are defined directly in the UI loop rather than via lambdas. These must be updated as well:
- **"Always on Top" Checkbox**: Already calls `SetWindowAlwaysOnTop`, needs `Config::Save(engine)`.
- **"Graphs" Checkbox**: Already calls `Config::Save`. (Verify)
- **Speed Gate Sliders**: Inside "Advanced Settings", these use raw `ImGui::SliderFloat`.

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

### Test 4: Presets Coexistence
1. Load a preset.
2. Verify the preset name is NOT saved as a global setting (it shouldn't be), but the values within the preset are written to the main section.

## 5. Implementation Roadmap
- [ ] Refactor `FloatSetting` lambda.
- [ ] Refactor `BoolSetting` lambda.
- [ ] Refactor `IntSetting` lambda.
- [ ] Audit remaining `ImGui` calls (`Checkbox`, `SliderFloat`, `Combo`) outside lambdas.
- [ ] Verify persistence of "Always on Top" and "Graphs" state.
- [ ] Perform binary size / performance check (disk thrashing test).
