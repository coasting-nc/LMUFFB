# Question


See the full codebase and documentation of a force feedback (FFB) app. 
Recently, we added a feature to display numerical readouts in the plots. See the changelog for 0.4.21.
The problem is that the info for min, max and current were added to the title of each plot, making the whole window quite wide to display everything. I am wondering how I could redesign this to make the window more compact. My idea is to display the new values (min, max, current) on the plot itself, eg. as some king of legend, in small print, in order not to take any additional space. What do you think about this idea? Do you suggest alternative designs?


# Answer


To address the issue of the debug window becoming too wide due to the numerical readouts in the titles, I will redesign the `PlotWithStats` function in `src/GuiLayer.cpp`.

The new design will:
1.  Display the **Title** (Label) on its own line, keeping the layout compact.
2.  Draw the **Plot** using a hidden label (to avoid redundancy).
3.  **Overlay** the numerical statistics (Current, Min, Max) directly onto the top-left corner of the plot area, acting as a "legend".
4.  Add a semi-transparent background to the text to ensure readability against the graph lines.

This approach satisfies the requirement for compactness while retaining the precise diagnostic data.

Here are the changes:

# File: src\GuiLayer.cpp
```cpp
<<<<
// Helper function to plot with numerical readouts
// Displays: [Title] | Val: X.XXX | Min: Y.YY | Max: Z.ZZ
inline void PlotWithStats(const char* label, const RollingBuffer& buffer, 
                          float scale_min, float scale_max, 
                          const ImVec2& size = ImVec2(0, 40),
                          const char* tooltip = nullptr) {
    // Get statistics
    float current = buffer.GetCurrent();
    float min_val = buffer.GetMin();
    float max_val = buffer.GetMax();
    
    // Format the label with statistics
    char stats_label[256];
    snprintf(stats_label, sizeof(stats_label), "%s | Val: %.4f | Min: %.3f | Max: %.3f", 
             label, current, min_val, max_val);
    
    ImGui::Text("%s", stats_label);
    
    // Draw the plot
    ImGui::PlotLines(label, buffer.data.data(), (int)buffer.data.size(), 
                     buffer.offset, NULL, scale_min, scale_max, size);
    
    // Add tooltip if provided
    if (tooltip && ImGui::IsItemHovered()) {
        ImGui::SetTooltip("%s", tooltip);
    }
}
====
// Helper function to plot with numerical readouts
// Displays: [Title]
// Overlay:  Cur: X.XXXX Min: Y.YYY Max: Z.ZZZ
inline void PlotWithStats(const char* label, const RollingBuffer& buffer, 
                          float scale_min, float scale_max, 
                          const ImVec2& size = ImVec2(0, 40),
                          const char* tooltip = nullptr) {
    // 1. Draw Title (Short, keeps window compact)
    ImGui::Text("%s", label);
    
    // 2. Draw Plot
    // Use hidden label "##Label" so ImGui doesn't print it again next to the graph
    char hidden_label[256];
    snprintf(hidden_label, sizeof(hidden_label), "##%s", label);
    
    ImGui::PlotLines(hidden_label, buffer.data.data(), (int)buffer.data.size(), 
                     buffer.offset, NULL, scale_min, scale_max, size);
    
    // 3. Handle Tooltip
    if (tooltip && ImGui::IsItemHovered()) {
        ImGui::SetTooltip("%s", tooltip);
    }

    // 4. Draw Stats Overlay (Legend style)
    float current = buffer.GetCurrent();
    float min_val = buffer.GetMin();
    float max_val = buffer.GetMax();
    
    char stats_overlay[128];
    // Compact format for the overlay
    snprintf(stats_overlay, sizeof(stats_overlay), "Cur:%.4f Min:%.3f Max:%.3f", 
             current, min_val, max_val);
    
    // Get position of the plot we just drew
    ImVec2 p_min = ImGui::GetItemRectMin();
    
    // Padding from top-left of the graph
    p_min.x += 4;
    p_min.y += 2;
    
    // Draw Overlay using the Window Draw List
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    
    // Calculate text size for background box
    ImVec2 text_size = ImGui::CalcTextSize(stats_overlay);
    ImVec2 box_max = ImVec2(p_min.x + text_size.x + 2, p_min.y + text_size.y + 2);
    
    // Draw semi-transparent black background for readability
    draw_list->AddRectFilled(ImVec2(p_min.x - 2, p_min.y - 2), box_max, IM_COL32(0, 0, 0, 160));
    
    // Draw text (White)
    draw_list->AddText(p_min, IM_COL32(255, 255, 255, 255), stats_overlay);
}
>>>>
```

# Fixes for first implementation

To fix the issues with the overlay being too large, opaque, and overflowing, I will update the `PlotWithStats` function in `src/GuiLayer.cpp`.

The improvements include:
1.  **Font Scaling**: Using `ImFont::CalcTextSizeA` and `AddText` with a scale factor (0.70f) to draw "small print" without needing a separate font file.
2.  **Adaptive Formatting**: Checking the text width against the plot width. If it overflows, it switches to a more compact format (`Val [Min, Max]`), and finally to just `Val` if space is extremely tight.
3.  **Transparency**: Reducing the background alpha to `90` (approx 35%) to make it less obtrusive while maintaining readability.

Here is the code update:

# File: src\GuiLayer.cpp
```cpp
<<<<
// Helper function to plot with numerical readouts
// Displays: [Title]
// Overlay:  Cur: X.XXXX Min: Y.YYY Max: Z.ZZZ
inline void PlotWithStats(const char* label, const RollingBuffer& buffer, 
                          float scale_min, float scale_max, 
                          const ImVec2& size = ImVec2(0, 40),
                          const char* tooltip = nullptr) {
    // 1. Draw Title (Short, keeps window compact)
    ImGui::Text("%s", label);
    
    // 2. Draw Plot
    // Use hidden label "##Label" so ImGui doesn't print it again next to the graph
    char hidden_label[256];
    snprintf(hidden_label, sizeof(hidden_label), "##%s", label);
    
    ImGui::PlotLines(hidden_label, buffer.data.data(), (int)buffer.data.size(), 
                     buffer.offset, NULL, scale_min, scale_max, size);
    
    // 3. Handle Tooltip
    if (tooltip && ImGui::IsItemHovered()) {
        ImGui::SetTooltip("%s", tooltip);
    }

    // 4. Draw Stats Overlay (Legend style)
    float current = buffer.GetCurrent();
    float min_val = buffer.GetMin();
    float max_val = buffer.GetMax();
    
    char stats_overlay[128];
    // Compact format for the overlay
    snprintf(stats_overlay, sizeof(stats_overlay), "Cur:%.4f Min:%.3f Max:%.3f", 
             current, min_val, max_val);
    
    // Get position of the plot we just drew
    ImVec2 p_min = ImGui::GetItemRectMin();
    
    // Padding from top-left of the graph
    p_min.x += 4;
    p_min.y += 2;
    
    // Draw Overlay using the Window Draw List
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    
    // Calculate text size for background box
    ImVec2 text_size = ImGui::CalcTextSize(stats_overlay);
    ImVec2 box_max = ImVec2(p_min.x + text_size.x + 2, p_min.y + text_size.y + 2);
    
    // Draw semi-transparent black background for readability
    draw_list->AddRectFilled(ImVec2(p_min.x - 2, p_min.y - 2), box_max, IM_COL32(0, 0, 0, 160));
    
    // Draw text (White)
    draw_list->AddText(p_min, IM_COL32(255, 255, 255, 255), stats_overlay);
}
====
// Helper function to plot with numerical readouts
// Displays: [Title]
// Overlay:  Cur: X.XXXX Min: Y.YYY Max: Z.ZZZ (Small print)
inline void PlotWithStats(const char* label, const RollingBuffer& buffer, 
                          float scale_min, float scale_max, 
                          const ImVec2& size = ImVec2(0, 40),
                          const char* tooltip = nullptr) {
    // 1. Draw Title
    ImGui::Text("%s", label);
    
    // 2. Draw Plot
    char hidden_label[256];
    snprintf(hidden_label, sizeof(hidden_label), "##%s", label);
    
    ImGui::PlotLines(hidden_label, buffer.data.data(), (int)buffer.data.size(), 
                     buffer.offset, NULL, scale_min, scale_max, size);
    
    // 3. Handle Tooltip
    if (tooltip && ImGui::IsItemHovered()) {
        ImGui::SetTooltip("%s", tooltip);
    }

    // 4. Draw Stats Overlay (Small Legend)
    float current = buffer.GetCurrent();
    float min_val = buffer.GetMin();
    float max_val = buffer.GetMax();
    
    char stats_overlay[128];
    snprintf(stats_overlay, sizeof(stats_overlay), "Cur:%.4f Min:%.3f Max:%.3f", 
             current, min_val, max_val);
    
    ImVec2 p_min = ImGui::GetItemRectMin();
    ImVec2 p_max = ImGui::GetItemRectMax();
    float plot_width = p_max.x - p_min.x;
    
    // Padding
    p_min.x += 2;
    p_min.y += 2;
    
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    
    // Use current font but scaled down (Small Print)
    ImFont* font = ImGui::GetFont();
    float font_size = ImGui::GetFontSize() * 0.70f; // 70% size
    
    ImVec2 text_size = font->CalcTextSizeA(font_size, FLT_MAX, 0.0f, stats_overlay);
    
    // Adaptive Formatting: If text is too wide, switch to compact mode
    if (text_size.x > plot_width - 4) {
         // Compact: 0.0000 [0.000, 0.000]
         snprintf(stats_overlay, sizeof(stats_overlay), "%.4f [%.3f, %.3f]", current, min_val, max_val);
         text_size = font->CalcTextSizeA(font_size, FLT_MAX, 0.0f, stats_overlay);
         
         // If still too wide, just show current value
         if (text_size.x > plot_width - 4) {
             snprintf(stats_overlay, sizeof(stats_overlay), "Val: %.4f", current);
             text_size = font->CalcTextSizeA(font_size, FLT_MAX, 0.0f, stats_overlay);
         }
    }

    ImVec2 box_max = ImVec2(p_min.x + text_size.x + 2, p_min.y + text_size.y);
    
    // Semi-transparent background (Alpha 90/255 approx 35%)
    draw_list->AddRectFilled(ImVec2(p_min.x - 1, p_min.y), box_max, IM_COL32(0, 0, 0, 90));
    
    // Draw Text with scaled font
    draw_list->AddText(font, font_size, p_min, IM_COL32(255, 255, 255, 255), stats_overlay);
}
>>>>
```