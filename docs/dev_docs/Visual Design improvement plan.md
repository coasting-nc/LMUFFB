
## Introduction

This interface appears to be built using **Dear ImGui**, a popular immediate-mode GUI library for C++. While ImGui is powerful, its default "programmer art" style often leads exactly to the problems you described: a "wall of text" effect where everything has equal visual weight.

The core issue here is a lack of **Visual Hierarchy**. Your eyes don't know where to rest because the headers (structure) are shouting just as loud as the sliders (content).

Here are expert visual design strategies to improve readability, categorized by impact.

### 1. Fix the Headers (The "Solid Color" Problem)
You correctly identified that the solid blue bars are distracting. They create a "zebra striping" effect that cuts the interface into rigid blocks, making it hard to scan vertically.

*   **Option A: The "Underline" Style (Modern & Clean)**
    *   Remove the solid background fill from the headers entirely.
    *   Instead, use a **bottom border** (a thin line) or a **left accent border** (a thick vertical line on the left edge) to denote the start of a section.
    *   *Why:* This reduces "visual noise" while still separating sections.
*   **Option B: The "Darker Shade" Style (Subtle)**
    *   If you prefer a background, make the header background color much darker (e.g., a very dark grey or navy, just slightly lighter than the main background).
    *   *Why:* This maintains the "container" feel without the aggressive contrast that draws the eye away from the data.

### 2. Establish Color Semantics (Structure vs. Data)
Currently, the **Headers**, the **Sliders**, and the **Checkboxes** all use the exact same shade of blue. This confuses the brain—it can't distinguish between "a container for things" and "a thing I can adjust."

*   **De-saturate Structure:** Make the collapsible headers (and window borders) neutral colors (greys, dark slates).
*   **Highlight Data:** Keep the bright blue (or switch to a distinct accent color like amber or teal) *only* for the interactive elements: the slider bars, the checkboxes, and the active values.
*   **Result:** When you look at the screen, your eye will immediately snap to the sliders (the controls) rather than the headers.

### 3. Improve Alignment (The "Ragged Edge" Issue)
Currently, your labels are on the right, and the controls are on the left.
*   **The Problem:** In Western languages, we read Left-to-Right. The user has to scan the control, then look right to see what it is. This creates a jagged reading path.
*   **The Fix:** If the framework allows, move the **Labels to the Left** and the **Controls to the Right**.
*   **The Compromise:** If you must keep labels on the right, ensure the text is **Left-Aligned** within its column, rather than floating at different distances from the sliders.

### 4. Visual Anchors for Nesting
You have nested sections (e.g., "Signal Filtering" is inside "Understeer"). Currently, they look almost identical.

*   **Indentation:** Increase the indentation for nested groups significantly (e.g., 20px).
*   **Tree Lines:** Add faint vertical lines connecting the child elements to the parent header (like a file explorer tree).
*   **Remove Backgrounds on Children:** Keep the top-level header (e.g., "Understeer") with a background, but make the sub-header (e.g., "Signal Filtering") transparent, using only bold text to distinguish it.

### 5. Typography and Values
*   **Separate Values from Sliders:** Currently, the numbers (e.g., "1.00") are inside the slider bar. This is hard to read if the slider is half-full.
    *   *Suggestion:* Move the numerical value to the right of the slider (or between the slider and the label).
*   **Dim the Units:** If you have "100.0 Nm", make the "Nm" slightly grey/dimmer than the "100.0". This makes the actual data (the number) pop out more.

### Summary Mockup Idea
Imagine the interface with these changes:
1.  **Background:** Dark Grey.
2.  **Headers:** No background color, just bold white text with a thin grey line underneath.
3.  **Sliders:** Bright Blue.
4.  **Labels:** Moved to the left side.

This would transform the app from a "list of boxes" into a "dashboard of controls."

## Implementation Plan

Here is the detailed plan and the code changes to transform the interface from a "programmer list" into a "professional dashboard."

### The Design Strategy
1.  **Grid Layout (Columns):** We will move from "Widget + Label" to a strict **2-Column Grid**.
    *   **Left Column:** Text Labels (Right-aligned or Left-aligned).
    *   **Right Column:** The Control (Slider/Checkbox).
    *   *Benefit:* This creates a clean vertical line that guides the eye. You can scan names on the left and values on the right instantly.
2.  **"Phantom" Headers:** We will remove the solid background blocks from the collapsible headers. Instead, we will use **Text + Separator Line**. This removes the "zebra stripe" distraction while keeping the grouping.
3.  **Color Palette:** We will switch to a "Dark Flat" theme.
    *   **Background:** Deep Grey (reduces eye strain).
    *   **Accent:** Bright Blue/Teal (only for active data/sliders).
    *   **Text:** Off-White (high contrast).

### Implementation Steps

#### 1. Update `src/GuiLayer.cpp` - Add Style Function
Add this function at the top of `src/GuiLayer.cpp` (before `Init`) to define the new professional look.

```cpp
// src/GuiLayer.cpp

// ... [Includes] ...

// NEW: Professional "Flat Dark" Theme
void SetupGUIStyle() {
    ImGuiStyle& style = ImGui::GetStyle();
    
    // 1. Geometry
    style.WindowRounding = 5.0f;
    style.FrameRounding = 4.0f;
    style.GrabRounding = 4.0f;
    style.FramePadding = ImVec2(8, 4);
    style.ItemSpacing = ImVec2(8, 6);
    
    // 2. Colors
    ImVec4* colors = style.Colors;
    
    // Backgrounds: Deep Grey
    colors[ImGuiCol_WindowBg]       = ImVec4(0.12f, 0.12f, 0.12f, 1.00f);
    colors[ImGuiCol_ChildBg]        = ImVec4(0.15f, 0.15f, 0.15f, 1.00f);
    colors[ImGuiCol_PopupBg]        = ImVec4(0.15f, 0.15f, 0.15f, 0.98f);
    
    // Headers: Transparent (Just text highlight)
    colors[ImGuiCol_Header]         = ImVec4(0.20f, 0.20f, 0.20f, 0.00f); // Transparent!
    colors[ImGuiCol_HeaderHovered]  = ImVec4(0.25f, 0.25f, 0.25f, 0.50f);
    colors[ImGuiCol_HeaderActive]   = ImVec4(0.30f, 0.30f, 0.30f, 0.50f);
    
    // Controls (Sliders/Buttons): Dark Grey container
    colors[ImGuiCol_FrameBg]        = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
    colors[ImGuiCol_FrameBgHovered] = ImVec4(0.25f, 0.25f, 0.25f, 1.00f);
    colors[ImGuiCol_FrameBgActive]  = ImVec4(0.30f, 0.30f, 0.30f, 1.00f);
    
    // Accents (The Data): Bright Blue/Teal
    // This draws the eye ONLY to the values
    ImVec4 accent = ImVec4(0.00f, 0.60f, 0.85f, 1.00f); 
    colors[ImGuiCol_SliderGrab]     = accent;
    colors[ImGuiCol_SliderGrabActive] = ImVec4(0.00f, 0.70f, 0.95f, 1.00f);
    colors[ImGuiCol_Button]         = ImVec4(0.25f, 0.25f, 0.25f, 1.00f);
    colors[ImGuiCol_ButtonHovered]  = accent;
    colors[ImGuiCol_ButtonActive]   = ImVec4(0.00f, 0.50f, 0.75f, 1.00f);
    colors[ImGuiCol_CheckMark]      = accent;
    
    // Text
    colors[ImGuiCol_Text]           = ImVec4(0.90f, 0.90f, 0.90f, 1.00f);
    colors[ImGuiCol_TextDisabled]   = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
}
```

#### 2. Update `src/GuiLayer.cpp` - Apply Style
Call the function inside `Init`.

```cpp
bool GuiLayer::Init() {
    // ... [Window Creation] ...

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    // APPLY NEW STYLE HERE
    SetupGUIStyle(); 

    // ... [Backends Init] ...
}
```

#### 3. Update `src/GuiLayer.cpp` - Refactor `DrawTuningWindow`
We completely restructure the drawing logic to use **Columns** and **Clean Headers**.

```cpp
void GuiLayer::DrawTuningWindow(FFBEngine& engine) {
    std::lock_guard<std::mutex> lock(g_engine_mutex);

    std::string title = std::string("LMUFFB v") + LMUFFB_VERSION + " - Configuration";
    ImGui::Begin(title.c_str());

    // --- HELPER: Section Header (Clean Line) ---
    auto Section = [](const char* name) {
        ImGui::Spacing();
        ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[0]); // Bold if available
        ImGui::TextColored(ImVec4(0.0f, 0.6f, 0.85f, 1.0f), "%s", name); // Accent Color Text
        ImGui::PopFont();
        ImGui::Separator();
        ImGui::Spacing();
    };

    // --- HELPER: 2-Column Property Row ---
    static int selected_preset = 0;
    
    // Float Slider Row
    auto FloatSetting = [&](const char* label, float* v, float min, float max, const char* fmt = "%.2f") {
        ImGui::Text("%s", label);               // Column 1: Label
        ImGui::NextColumn();                    // Switch to Column 2
        
        ImGui::SetNextItemWidth(-1);            // Fill width
        std::string id = "##" + std::string(label); // Hidden ID
        if (ImGui::SliderFloat(id.c_str(), v, min, max, fmt)) selected_preset = -1;
        
        // Tooltip logic
        if (ImGui::IsItemHovered()) {
            // ... [Keep existing tooltip logic] ...
            ImGui::SetTooltip("Ctrl+Click to input value.\nArrows to fine tune.");
        }
        ImGui::NextColumn();                    // Switch back to Column 1
    };

    // Boolean Checkbox Row
    auto BoolSetting = [&](const char* label, bool* v) {
        ImGui::Text("%s", label);
        ImGui::NextColumn();
        // Right align checkbox? Or keep left in col 2.
        std::string id = "##" + std::string(label);
        if (ImGui::Checkbox(id.c_str(), v)) selected_preset = -1;
        ImGui::NextColumn();
    };

    // --- 1. TOP BAR (Status & Global Controls) ---
    // Keep this outside columns for full width
    
    // Device & Status Logic (Keep existing logic here)
    // ... [Device Combo] ...
    // ... [Connection Status] ...
    
    ImGui::Spacing();
    
    // --- 2. MAIN SETTINGS GRID ---
    // Start 2 Columns: [Labels 40%] | [Controls 60%]
    ImGui::Columns(2, "SettingsGrid", false); // false = no border
    ImGui::SetColumnWidth(0, ImGui::GetWindowWidth() * 0.4f);

    // --- GROUP: GENERAL ---
    // Instead of CollapsingHeader, we use our clean Section helper
    // If you really want collapsing, use TreeNode but keep the clean style
    if (ImGui::TreeNodeEx("General FFB", ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Framed)) {
        ImGui::NextColumn(); ImGui::NextColumn(); // Skip header row
        
        BoolSetting("Invert FFB Signal", &engine.m_invert_force);
        FloatSetting("Master Gain", &engine.m_gain, 0.0f, 2.0f);
        FloatSetting("Max Torque Ref", &engine.m_max_torque_ref, 1.0f, 200.0f, "%.1f Nm");
        FloatSetting("Min Force", &engine.m_min_force, 0.0f, 0.20f);
        FloatSetting("Load Cap", &engine.m_max_load_factor, 1.0f, 3.0f, "%.1fx");
        
        ImGui::TreePop();
    } else { ImGui::NextColumn(); ImGui::NextColumn(); } // Handle closed state column sync

    // --- GROUP: FRONT AXLE ---
    if (ImGui::TreeNodeEx("Front Axle (Understeer)", ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Framed)) {
        ImGui::NextColumn(); ImGui::NextColumn();
        
        FloatSetting("Understeer Effect", &engine.m_understeer_effect, 0.0f, 50.0f);
        
        // Signal Filtering (Nested)
        // We can just indent the label in the left column
        ImGui::TextColored(ImVec4(0.6f,0.6f,0.6f,1.0f), "Signal Filtering");
        ImGui::NextColumn(); ImGui::NextColumn(); // Skip control col
        
        BoolSetting("  Dynamic Flatspot Filter", &engine.m_flatspot_suppression);
        if (engine.m_flatspot_suppression) {
            FloatSetting("  Filter Width (Q)", &engine.m_notch_q, 0.5f, 10.0f);
            FloatSetting("  Suppression Strength", &engine.m_flatspot_strength, 0.0f, 1.0f);
        }
        
        ImGui::TreePop();
    } else { ImGui::NextColumn(); ImGui::NextColumn(); }

    // --- GROUP: REAR AXLE ---
    if (ImGui::TreeNodeEx("Rear Axle (Oversteer)", ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Framed)) {
        ImGui::NextColumn(); ImGui::NextColumn();
        
        FloatSetting("SoP (Lateral G)", &engine.m_sop_effect, 0.0f, 20.0f);
        FloatSetting("Rear Align Torque", &engine.m_rear_align_effect, 0.0f, 20.0f);
        FloatSetting("Yaw Kick", &engine.m_sop_yaw_gain, 0.0f, 20.0f);
        FloatSetting("Oversteer Boost", &engine.m_oversteer_boost, 0.0f, 20.0f);
        FloatSetting("Gyro Damping", &engine.m_gyro_gain, 0.0f, 1.0f);
        
        ImGui::TreePop();
    } else { ImGui::NextColumn(); ImGui::NextColumn(); }

    // --- GROUP: TEXTURES ---
    if (ImGui::TreeNodeEx("Tactile Textures", ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Framed)) {
        ImGui::NextColumn(); ImGui::NextColumn();
        
        BoolSetting("Slide Rumble", &engine.m_slide_texture_enabled);
        if (engine.m_slide_texture_enabled) {
            FloatSetting("  Slide Gain", &engine.m_slide_texture_gain, 0.0f, 5.0f);
            FloatSetting("  Slide Freq", &engine.m_slide_freq_scale, 0.5f, 5.0f, "%.1fx");
        }
        
        BoolSetting("Road Details", &engine.m_road_texture_enabled);
        if (engine.m_road_texture_enabled) {
            FloatSetting("  Road Gain", &engine.m_road_texture_gain, 0.0f, 5.0f);
        }
        
        BoolSetting("Lockup Vibration", &engine.m_lockup_enabled);
        if (engine.m_lockup_enabled) {
             FloatSetting("  Lockup Gain", &engine.m_lockup_gain, 0.0f, 5.0f);
        }
        
        ImGui::TreePop();
    } else { ImGui::NextColumn(); ImGui::NextColumn(); }

    // End Columns
    ImGui::Columns(1);
    
    ImGui::End();
}
```

### 4. Update `CHANGELOG.md`

```markdown
## [0.4.49] - 2025-12-23
### Changed
- **GUI Overhaul**: Redesigned the Tuning Window for improved readability and workflow.
    - **Dark Theme**: Implemented a professional dark grey theme with high-contrast blue accents for active controls.
    - **Grid Layout**: Switched to a 2-column layout (Labels Left, Controls Right) to align sliders perfectly and reduce visual clutter.
    - **Clean Headers**: Removed solid background bars from sections to eliminate the "zebra striping" effect.
    - **Visual Hierarchy**: Grouped related settings with clear indentation and spacing.
```

### 5. Update `VERSION`
Increment to `0.4.49`.

### Summary of Changes
*   **Readability:** The 2-column layout forces alignment. Your eyes don't have to hunt for the slider.
*   **Focus:** The "Dark Mode" with "Blue Accents" means the only thing that "pops" on the screen is the **Data** (the slider handles and values). The structure fades into the background.
*   **Space:** Removing the heavy header backgrounds makes the UI feel lighter and less cramped.