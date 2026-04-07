#ifndef GUIWIDGETS_H
#define GUIWIDGETS_H

#ifdef ENABLE_IMGUI
#include "imgui.h"
#include "Tooltips.h"
#include <string>
#include <algorithm>
#include <functional>
#include <cstring>

namespace LMUFFB {
namespace GUI {
namespace GuiWidgets {

    inline bool& UseTableLayoutFlag() {
        static bool use_table_layout = false;
        return use_table_layout;
    }

    struct ScopedTableLayout {
        explicit ScopedTableLayout(bool enabled) : previous(UseTableLayoutFlag()) {
            UseTableLayoutFlag() = enabled;
        }

        ~ScopedTableLayout() {
            UseTableLayoutFlag() = previous;
        }

    private:
        bool previous;
    };
    
    inline void SkipRow() {
        ImGui::TableNextRow(); 
        ImGui::TableSetColumnIndex(0); ImGui::Dummy(ImVec2(0.0f, 4.0f));
        ImGui::TableSetColumnIndex(1); ImGui::Dummy(ImVec2(0.0f, 4.0f));
        ImGui::TableNextRow(); 
        ImGui::TableSetColumnIndex(0);
    }

    inline void AdvanceRow() {
        ImGui::TableNextRow(); ImGui::TableSetColumnIndex(0);
    }

    inline void AdvanceColumn() {
        if (UseTableLayoutFlag()) {
            ImGui::TableNextColumn();
        } else {
            ImGui::NextColumn();
        }
    }

    /**
     * Represents the result of a widget interaction.
     * Use this to trigger higher-level logic like auto-save or preset dirtying.
     */
    struct Result {
        bool changed = false;     // True if value was modified this frame
        bool deactivated = false; // True if interaction finished (mouse release, enter key, or discrete change)
    };

    /**
     * A standardized float slider with label, adaptive arrow-key support, and decorators.
     */
    inline Result Float(const char* label, float* v, float min, float max, const char* fmt = "%.2f", const char* tooltip = nullptr, std::function<void()> decorator = nullptr) {
        Result res;
        ImGui::Text("%s", label);
        bool labelHovered = ImGui::IsItemHovered();
        AdvanceColumn();

        // Render decorator (e.g., latency indicator) above the slider
        if (decorator) {
            decorator();
        }

        ImGui::SetNextItemWidth(-1);
        std::string id = "##" + std::string(label);

        // Core Slider
        if (ImGui::SliderFloat(id.c_str(), v, min, max, fmt)) {
            res.changed = true;
        }

        // Detect mouse release or Enter key after a series of edits
        if (ImGui::IsItemDeactivatedAfterEdit()) {
            res.deactivated = true;
        }

        // Unified Interaction Logic (Arrow Keys & Tooltips)
        if (ImGui::IsItemHovered() || labelHovered) {
            float range = max - min;
            // Adaptive step size: finer steps for smaller ranges
            float step = (range > 50.0f) ? 0.5f : (range < 1.0f) ? 0.001f : 0.01f; 
            
            bool keyChanged = false;
            // Note: We use IsKeyPressed which supports repeats
            if (ImGui::IsKeyPressed(ImGuiKey_LeftArrow)) { *v -= step; keyChanged = true; }
            if (ImGui::IsKeyPressed(ImGuiKey_RightArrow)) { *v += step; keyChanged = true; }

            if (keyChanged) {
                *v = (std::max)(min, (std::min)(max, *v));
                res.changed = true;
                res.deactivated = true; // Arrow keys are discrete adjustments, save immediately
            }

            // Show tooltip only if not actively interacting
            if (!keyChanged && !ImGui::IsItemActive()) {
                ImGui::BeginTooltip();
                if (tooltip && strlen(tooltip) > 0) {
                    ImGui::Text("%s", tooltip);
                    ImGui::Separator();
                }
                ImGui::Text("%s", LMUFFB::Tooltips::FINE_TUNE);
                ImGui::EndTooltip();
            }
        }

        AdvanceColumn();
        return res;
    }

    /**
     * A standardized checkbox with label and tooltip.
     */
    inline Result Checkbox(const char* label, bool* v, const char* tooltip = nullptr) {
        Result res;
        ImGui::Text("%s", label);
        bool labelHovered = ImGui::IsItemHovered();
        AdvanceColumn();
        std::string id = "##" + std::string(label);
        
        if (ImGui::Checkbox(id.c_str(), v)) {
            res.changed = true;
            res.deactivated = true; // Checkboxes are immediate
        }

        if (tooltip && (ImGui::IsItemHovered() || labelHovered)) {
            ImGui::SetTooltip("%s", tooltip);
        }

        AdvanceColumn();
        return res;
    }

    /**
     * A standardized combo box with label and tooltip.
     */
    inline Result Combo(const char* label, int* v, const char* const items[], int items_count, const char* tooltip = nullptr) {
        Result res;
        ImGui::Text("%s", label);
        bool labelHovered = ImGui::IsItemHovered();
        AdvanceColumn();
        ImGui::SetNextItemWidth(-1);
        std::string id = "##" + std::string(label);

        if (ImGui::Combo(id.c_str(), v, items, items_count)) {
            res.changed = true;
            res.deactivated = true; // Selection changes are immediate
        }

        if (tooltip && (ImGui::IsItemHovered() || labelHovered)) {
            ImGui::SetTooltip("%s", tooltip);
        }

        AdvanceColumn();
        return res;
    }
}
}
}

namespace LMUFFB {
    namespace GuiWidgets = GUI::GuiWidgets;
}

#endif // ENABLE_IMGUI

#endif // GUIWIDGETS_H
