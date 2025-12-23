Here is the detailed plan to implement the "Always on Top" toggle and "Keyboard Fine-Tuning" for sliders.

### Feature 1: "Always on Top" Window
To keep the window visible over the game, we need to use the Windows API `SetWindowPos` with the `HWND_TOPMOST` flag. We will persist this setting in `Config` so the user doesn't have to toggle it every time.

### Feature 2: Keyboard Fine-Tuning
Dear ImGui supports `Ctrl + Click` to type exact numbers, but for arrow key adjustments, the widget usually needs to be "Active" (clicked and held). To make it easier (just **Hover + Arrow Keys**), we will modify the `FloatSetting` helper lambda in the GUI layer to intercept key presses when the mouse is over a slider.

---

### Implementation Plan

#### Step 1: Update Configuration (`src/Config.h`)
Add a variable to store the "Always on Top" preference.

**File:** `src/Config.h`
```cpp
class Config {
public:
    // ... existing members ...
    
    // Global App Settings
    static bool m_ignore_vjoy_version_warning;
    static bool m_enable_vjoy;        
    static bool m_output_ffb_to_vjoy; 
    
    // NEW: Window Setting
    static bool m_always_on_top; 
};
```

#### Step 2: Update Persistence (`src/Config.cpp`)
Initialize, Save, and Load the new setting.

**File:** `src/Config.cpp`

**A. Initialize:**
```cpp
// At the top of the file
bool Config::m_always_on_top = false; // Default off
```

**B. Update `Save`:**
```cpp
void Config::Save(const FFBEngine& engine, const std::string& filename) {
    std::ofstream file(filename);
    if (file.is_open()) {
        // ... existing settings ...
        file << "output_ffb_to_vjoy=" << m_output_ffb_to_vjoy << "\n";
        file << "always_on_top=" << m_always_on_top << "\n"; // NEW
        // ... rest of file ...
```

**C. Update `Load`:**
```cpp
void Config::Load(FFBEngine& engine, const std::string& filename) {
    // ... inside the parsing loop ...
    if (key == "ignore_vjoy_version_warning") m_ignore_vjoy_version_warning = std::stoi(value);
    else if (key == "enable_vjoy") m_enable_vjoy = std::stoi(value);
    else if (key == "output_ffb_to_vjoy") m_output_ffb_to_vjoy = std::stoi(value);
    else if (key == "always_on_top") m_always_on_top = std::stoi(value); // NEW
    // ... rest of parsing ...
}
```

#### Step 3: Implement Logic in GUI Layer (`src/GuiLayer.cpp`)

We need to:
1.  Apply the "Always on Top" setting to the window.
2.  Add the checkbox to the UI.
3.  Update the `FloatSetting` lambda to handle arrow keys.

**File:** `src/GuiLayer.cpp`

**A. Add Helper Function (Near top, after includes):**
This function applies the Windows API call.
```cpp
// Helper to toggle Topmost
void SetWindowAlwaysOnTop(HWND hwnd, bool enabled) {
    if (!hwnd) return;
    HWND insertAfter = enabled ? HWND_TOPMOST : HWND_NOTOPMOST;
    // SWP_NOMOVE | SWP_NOSIZE means we only change Z-order, not position/size
    ::SetWindowPos(hwnd, insertAfter, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
}
```

**B. Apply on Startup (Inside `GuiLayer::Init`):**
Right after creating the window, apply the saved config.
```cpp
bool GuiLayer::Init() {
    // ... existing window creation code ...
    g_hwnd = ::CreateWindowW(wc.lpszClassName, title.c_str(), WS_OVERLAPPEDWINDOW, 100, 100, 800, 600, NULL, NULL, wc.hInstance, NULL);

    // NEW: Apply saved "Always on Top" setting immediately
    if (Config::m_always_on_top) {
        SetWindowAlwaysOnTop(g_hwnd, true);
    }

    // ... rest of Init ...
}
```

**C. Update `DrawTuningWindow` (The UI Logic):**

Locate `void GuiLayer::DrawTuningWindow(FFBEngine& engine)` and modify the `FloatSetting` lambda and add the checkbox.

```cpp
void GuiLayer::DrawTuningWindow(FFBEngine& engine) {
    // ... [Lock and Begin] ...

    // --- NEW: Window Settings Section ---
    // Place this near the top, perhaps after Connection Status
    if (ImGui::Checkbox("Always on Top", &Config::m_always_on_top)) {
        SetWindowAlwaysOnTop(g_hwnd, Config::m_always_on_top);
    }
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Keep this window visible over the game.");
    ImGui::Separator();
    // ------------------------------------

    // ... [Device Selection Code] ...

    // ... [Presets Code] ...

    ImGui::Separator();
    
    // --- MODIFIED: FloatSetting Lambda for Keyboard Control ---
    auto FloatSetting = [&](const char* label, float* v, float min, float max, const char* fmt = "%.2f") {
        // 1. Draw the standard slider
        if (ImGui::SliderFloat(label, v, min, max, fmt)) {
            selected_preset = -1; // Mark as Custom
        }

        // 2. NEW: Keyboard Fine-Tuning Logic
        // If the item is hovered, allow arrow keys to adjust it
        if (ImGui::IsItemHovered()) {
            // Calculate a fine step (e.g., 1/100th of the range, or fixed 0.01/0.1)
            // For most FFB settings (0.0-1.0), 0.01 is good. 
            // For larger ranges (0-100), 0.1 or 1.0 is better.
            float range = max - min;
            float step = (range > 50.0f) ? 0.5f : 0.01f; 
            
            // Check for Arrow Keys
            bool changed = false;
            if (ImGui::IsKeyPressed(ImGuiKey_LeftArrow)) {
                *v -= step;
                changed = true;
            }
            if (ImGui::IsKeyPressed(ImGuiKey_RightArrow)) {
                *v += step;
                changed = true;
            }

            if (changed) {
                // Clamp and mark as custom
                *v = (std::max)(min, (std::min)(max, *v));
                selected_preset = -1;
            }
            
            // Optional: Add a tooltip to inform the user
            ImGui::BeginTooltip();
            ImGui::Text("Fine Tune: Arrow Keys");
            ImGui::Text("Exact Input: Ctrl + Click");
            ImGui::EndTooltip();
        }
    };
    
    // ... [Rest of the function uses FloatSetting automatically] ...
```

### Summary of Changes

1.  **Always on Top:**
    *   Added `Config::m_always_on_top`.
    *   Added `SetWindowAlwaysOnTop` helper using Win32 API.
    *   Added Checkbox in GUI to toggle it live.
    *   Applied setting during App Initialization.

2.  **Keyboard Control:**
    *   Modified the `FloatSetting` lambda.
    *   Now, simply **hovering** the mouse over any slider and pressing **Left/Right Arrow** will adjust the value by a small step (`0.01` for small ranges, `0.5` for large ranges).
    *   Added a tooltip hint reminding users they can also use `Ctrl + Click` to type exact numbers (a built-in ImGui feature).


## 4. Testing Strategy

Since `SetWindowPos` (Windows API) and `ImGui::IsKeyPressed` (GUI Interaction) are difficult to unit test in a headless environment, we will focus on verifying the **Persistence Logic** and **Default States**.

### Automated Tests (`tests\test_windows_platform.cpp`)

We will create a new test file `tests\test_windows_platform.cpp` to isolate platform-specific feature toggles.

**Test Case 1: Always On Top Persistence**
*   **Goal:** Verify that the `always_on_top` flag is correctly written to and read from the `config.ini` file.
*   **Steps:**
    1.  Initialize `Config::m_always_on_top = true`.
    2.  Call `Config::Save` to a temporary file.
    3.  Reset `Config::m_always_on_top = false`.
    4.  Call `Config::Load` from the temporary file.
    5.  **Assert:** `Config::m_always_on_top` is `true`.

**Test Case 2: Default State**
*   **Goal:** Ensure the feature defaults to `false` (standard window behavior) to avoid confusing new users.
*   **Steps:**
    1.  Reset Config static variables.
    2.  **Assert:** `Config::m_always_on_top` is `false`.

### Manual Verification (Post-Build)

**1. Always on Top:**
*   Launch LMUFFB.
*   Open another window (e.g., Notepad) and drag it over LMUFFB. It should cover LMUFFB.
*   Check "Always on Top".
*   Click on Notepad. LMUFFB should remain **visible** on top of Notepad.
*   Uncheck "Always on Top". LMUFFB should fall behind Notepad.

**2. Keyboard Fine-Tuning:**
*   Hover the mouse over "Master Gain".
*   Press **Right Arrow**. Value should increase by `0.01`.
*   Press **Left Arrow**. Value should decrease by `0.01`.
*   Hover over "Max Torque Ref" (Range 1-100).
*   Press Arrow Keys. Value should change by `0.5` (due to larger range logic).