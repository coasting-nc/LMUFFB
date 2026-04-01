## 2025-04-01 - ImPlot Transition for Real-time Telemetry
**Learning:** Transitioning from basic ImGui::PlotLines to ImPlot significantly improves performance and visual clarity for high-frequency telemetry (400Hz). The ScrollingBuffer pattern with ImPlot::PlotLine avoids costly array shifts and provides built-in axis management and annotations.
**Action:** Use ImPlot for any complex or high-frequency data visualization in ImGui-based apps. Ensure ImPlot context is managed in unit tests that touch GUI logic.
