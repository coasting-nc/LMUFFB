---
name: gui_development
description: Patterns for ImGui development and thread-safe data visualization
---

# GUI Development

Use this skill when modifying `src/GuiLayer.cpp` or adding new UI elements.

## Producer-Consumer Pattern
The GUI (60Hz) and FFB Engine (400Hz) communicate via a producer-consumer pattern to avoid aliasing in plots.

1. **Producer (FFB Thread)**: Pushes `FFBSnapshot` structs into a debug buffer every tick.
2. **Consumer (GUI Thread)**: Calls `GetDebugBatch()` to swap buffers and render all ticks since the last frame.

**Constraint**: Never read `FFBEngine` state directly for plots; always use the snapshot batch.

## Thread Safety
Access to `FFBEngine` settings from the GUI thread (e.g., slider adjustments) must be protected by the global engine mutex:
```cpp
std::lock_guard<std::mutex> lock(g_engine_mutex);
// update engine setting
```

## Adding UI Controls
- Use `GuiLayer::DrawTuningWindow` for effect parameters.
- Ensure new settings are persisted via `Config` or `FFBEngine` state as appropriate.
- Visualize new effects by adding fields to `FFBSnapshot` and plotting them.
