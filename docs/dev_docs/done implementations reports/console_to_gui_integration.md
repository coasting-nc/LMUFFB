# Console to GUI Integration - Implementation Reference

**Document Version:** 1.0  
**Date:** 2025-12-26  
**Status:** Future Enhancement Proposal

## Overview

This document provides a comprehensive reference for implementing an integrated console panel within the lmuFFB GUI. This would eliminate the need for a separate console window and allow all debug output to be displayed directly in the ImGui interface.

## Current Architecture

### Console Output Flow
```
main.cpp (FFB Thread) → std::cout → Windows Console Window
GuiLayer.cpp          → std::cout → Windows Console Window
DirectInputFFB.cpp    → std::cout → Windows Console Window
GameConnector.cpp     → std::cout → Windows Console Window
```

### Screenshot Behavior
- **Current (v0.6.4+):** Composite screenshot captures both GUI window and separate console window
- **After Integration:** Single window screenshot would capture everything

## Proposed Architecture

### 1. Console Buffer Manager

Create a new class to manage console output:

**File:** `src/ConsoleBuffer.h`

```cpp
#ifndef CONSOLEBUFFER_H
#define CONSOLEBUFFER_H

#include <string>
#include <vector>
#include <mutex>
#include <sstream>

class ConsoleBuffer {
public:
    struct LogEntry {
        std::string timestamp;
        std::string category;  // e.g., "[FFB]", "[GUI]", "[Game]"
        std::string message;
        int level;            // 0=Info, 1=Warning, 2=Error
    };

    static ConsoleBuffer& Get() {
        static ConsoleBuffer instance;
        return instance;
    }

    // Thread-safe logging
    void Log(const std::string& category, const std::string& message, int level = 0);
    
    // Get recent entries for display
    std::vector<LogEntry> GetRecentEntries(size_t maxCount = 1000);
    
    // Clear buffer
    void Clear();
    
    // Enable/disable also writing to stdout
    void SetMirrorToStdout(bool enable) { m_mirrorToStdout = enable; }

private:
    ConsoleBuffer() : m_mirrorToStdout(true), m_maxEntries(10000) {}
    
    std::vector<LogEntry> m_entries;
    std::mutex m_mutex;
    bool m_mirrorToStdout;
    size_t m_maxEntries;
};

#endif // CONSOLEBUFFER_H
```

**File:** `src/ConsoleBuffer.cpp`

```cpp
#include "ConsoleBuffer.h"
#include <iostream>
#include <chrono>
#include <iomanip>
#include <sstream>

void ConsoleBuffer::Log(const std::string& category, const std::string& message, int level) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    // Generate timestamp
    auto now = std::chrono::system_clock::now();
    auto time_t_now = std::chrono::system_clock::to_time_t(now);
    std::tm tm_now;
    localtime_s(&tm_now, &time_t_now);
    
    std::ostringstream oss;
    oss << std::put_time(&tm_now, "%H:%M:%S");
    
    LogEntry entry;
    entry.timestamp = oss.str();
    entry.category = category;
    entry.message = message;
    entry.level = level;
    
    m_entries.push_back(entry);
    
    // Limit buffer size
    if (m_entries.size() > m_maxEntries) {
        m_entries.erase(m_entries.begin(), m_entries.begin() + (m_entries.size() - m_maxEntries));
    }
    
    // Mirror to stdout if enabled
    if (m_mirrorToStdout) {
        std::cout << category << " " << message << std::endl;
    }
}

std::vector<ConsoleBuffer::LogEntry> ConsoleBuffer::GetRecentEntries(size_t maxCount) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    if (m_entries.size() <= maxCount) {
        return m_entries;
    }
    
    return std::vector<LogEntry>(m_entries.end() - maxCount, m_entries.end());
}

void ConsoleBuffer::Clear() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_entries.clear();
}
```

### 2. Custom Stream Buffer (Optional Advanced Approach)

For automatic capture of all `std::cout` calls without code changes:

**File:** `src/ConsoleStreamBuf.h`

```cpp
#ifndef CONSOLESTREAMBUF_H
#define CONSOLESTREAMBUF_H

#include <streambuf>
#include <string>
#include "ConsoleBuffer.h"

class ConsoleStreamBuf : public std::streambuf {
public:
    ConsoleStreamBuf(std::streambuf* originalBuf) 
        : m_originalBuf(originalBuf), m_currentCategory("[App]") {}
    
    void SetCategory(const std::string& category) { m_currentCategory = category; }

protected:
    virtual int overflow(int c) override {
        if (c != EOF) {
            m_buffer += static_cast<char>(c);
            
            // Flush on newline
            if (c == '\n') {
                flush();
            }
        }
        return c;
    }
    
    virtual int sync() override {
        flush();
        return 0;
    }

private:
    void flush() {
        if (!m_buffer.empty()) {
            // Remove trailing newline
            if (m_buffer.back() == '\n') {
                m_buffer.pop_back();
            }
            
            // Parse category from message if present
            std::string category = m_currentCategory;
            std::string message = m_buffer;
            
            size_t bracketPos = m_buffer.find(']');
            if (m_buffer[0] == '[' && bracketPos != std::string::npos) {
                category = m_buffer.substr(0, bracketPos + 1);
                message = m_buffer.substr(bracketPos + 2);
            }
            
            ConsoleBuffer::Get().Log(category, message);
            m_buffer.clear();
        }
    }
    
    std::streambuf* m_originalBuf;
    std::string m_buffer;
    std::string m_currentCategory;
};

#endif // CONSOLESTREAMBUF_H
```

### 3. GUI Console Panel

Add console display to `GuiLayer.cpp`:

```cpp
void GuiLayer::DrawConsolePanel() {
    ImGui::Begin("Console", nullptr, ImGuiWindowFlags_None);
    
    // Toolbar
    if (ImGui::Button("Clear")) {
        ConsoleBuffer::Get().Clear();
    }
    ImGui::SameLine();
    
    static bool autoScroll = true;
    ImGui::Checkbox("Auto-scroll", &autoScroll);
    ImGui::SameLine();
    
    static bool showTimestamps = true;
    ImGui::Checkbox("Timestamps", &showTimestamps);
    
    ImGui::Separator();
    
    // Console output area
    ImGui::BeginChild("ConsoleScrolling", ImVec2(0, 0), false, 
                      ImGuiWindowFlags_HorizontalScrollbar);
    
    auto entries = ConsoleBuffer::Get().GetRecentEntries(1000);
    
    for (const auto& entry : entries) {
        ImVec4 color = ImVec4(1.0f, 1.0f, 1.0f, 1.0f); // White (Info)
        
        if (entry.level == 1) {
            color = ImVec4(1.0f, 1.0f, 0.0f, 1.0f); // Yellow (Warning)
        } else if (entry.level == 2) {
            color = ImVec4(1.0f, 0.0f, 0.0f, 1.0f); // Red (Error)
        }
        
        // Color-code categories
        if (entry.category == "[FFB]") {
            ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "%s", entry.category.c_str());
        } else if (entry.category == "[GUI]") {
            ImGui::TextColored(ImVec4(0.0f, 0.6f, 0.85f, 1.0f), "%s", entry.category.c_str());
        } else if (entry.category == "[Game]") {
            ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), "%s", entry.category.c_str());
        } else {
            ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "%s", entry.category.c_str());
        }
        
        ImGui::SameLine();
        
        if (showTimestamps) {
            ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "[%s]", entry.timestamp.c_str());
            ImGui::SameLine();
        }
        
        ImGui::TextColored(color, "%s", entry.message.c_str());
    }
    
    if (autoScroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY()) {
        ImGui::SetScrollHereY(1.0f);
    }
    
    ImGui::EndChild();
    ImGui::End();
}
```

### 4. Layout Integration Options

#### Option A: Tabbed Interface
```cpp
void GuiLayer::DrawTuningWindow(FFBEngine& engine) {
    // ... existing code ...
    
    if (ImGui::BeginTabBar("MainTabs")) {
        if (ImGui::BeginTabItem("Settings")) {
            // Existing settings UI
            ImGui::EndTabItem();
        }
        
        if (ImGui::BeginTabItem("Console")) {
            DrawConsolePanel();
            ImGui::EndTabItem();
        }
        
        if (Config::show_graphs && ImGui::BeginTabItem("Graphs")) {
            DrawDebugWindow(engine);
            ImGui::EndTabItem();
        }
        
        ImGui::EndTabBar();
    }
}
```

#### Option B: Collapsible Section
```cpp
if (ImGui::CollapsingHeader("Console Output", ImGuiTreeNodeFlags_None)) {
    ImGui::BeginChild("ConsoleRegion", ImVec2(0, 200), true);
    DrawConsolePanel();
    ImGui::EndChild();
}
```

#### Option C: Separate Dockable Window
```cpp
// Enable docking in Init()
ImGuiIO& io = ImGui::GetIO();
io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

// In Render()
ImGui::DockSpaceOverViewport(ImGui::GetMainViewport());
DrawConsolePanel(); // Will be dockable
```

## Migration Strategy

### Phase 1: Parallel Operation
1. Implement `ConsoleBuffer` class
2. Add GUI console panel
3. Keep existing `std::cout` calls mirrored to both GUI and console window
4. Add config option: `Config::m_show_console_window` (default: true for backwards compatibility)

### Phase 2: Gradual Migration
1. Update all `std::cout` calls to use `ConsoleBuffer::Get().Log()`
2. Add helper macros for convenience:
   ```cpp
   #define LOG_INFO(category, msg) ConsoleBuffer::Get().Log(category, msg, 0)
   #define LOG_WARN(category, msg) ConsoleBuffer::Get().Log(category, msg, 1)
   #define LOG_ERROR(category, msg) ConsoleBuffer::Get().Log(category, msg, 2)
   ```

### Phase 3: Console Window Optional
1. Change default: `Config::m_show_console_window = false`
2. Add command-line flag: `--console` to show traditional console
3. Update documentation

### Phase 4: Complete Integration
1. Remove console window entirely
2. All output through GUI
3. Update screenshot code to remove composite logic

## Code Changes Required

### Files to Modify
- `src/ConsoleBuffer.h` (new)
- `src/ConsoleBuffer.cpp` (new)
- `src/GuiLayer.h` (add `DrawConsolePanel()`)
- `src/GuiLayer.cpp` (implement console panel)
- `src/main.cpp` (initialize console buffer, optional: redirect stdout)
- `src/Config.h` (add `m_show_console_window` setting)
- `CMakeLists.txt` (add new source files)

### Estimated Lines of Code
- New code: ~400 lines
- Modified code: ~50 lines
- Total effort: ~450 lines

## Benefits

1. **Single Window Experience:** Cleaner UI, no window management
2. **Better Screenshot Workflow:** One button captures everything
3. **Enhanced Filtering:** Filter by category, level, search text
4. **Persistent History:** Console survives window minimize/restore
5. **Copy/Paste:** Easy to copy specific log entries
6. **Color Coding:** Visual distinction between message types
7. **Performance:** Can limit displayed entries without losing history

## Considerations

### Performance
- Circular buffer with max size (10,000 entries recommended)
- Only render visible entries (ImGui handles this automatically)
- Mutex protection for thread-safe logging

### User Experience
- Provide option to export console to text file
- Add search/filter functionality
- Consider log levels (Info/Warning/Error)
- Keyboard shortcuts (Ctrl+L to clear, etc.)

### Backwards Compatibility
- Keep `--headless` mode working
- Maintain ability to redirect stdout to file
- Support `--console` flag for traditional console window

## Testing Checklist

- [ ] Console captures all existing `std::cout` output
- [ ] Thread-safe logging from FFB thread
- [ ] Auto-scroll works correctly
- [ ] Clear button functions
- [ ] Color coding displays correctly
- [ ] Performance with 10,000+ entries
- [ ] Screenshot captures console panel
- [ ] Export to file works
- [ ] Headless mode still functional
- [ ] No memory leaks with continuous logging

## Future Enhancements

1. **Log Levels:** Implement filtering by severity
2. **Search:** Real-time search within console
3. **Regex Filtering:** Advanced filtering with regex patterns
4. **Export:** Save console to timestamped log file
5. **Notifications:** Toast notifications for errors/warnings
6. **Statistics:** Show message counts by category
7. **Performance Metrics:** Log timing information for FFB loop

## References

- ImGui Demo: `imgui_demo.cpp` - Console example (search for "ExampleAppLog")
- Current implementation: `src/GuiLayer.cpp` lines 285-455 (composite screenshot)
- Console output locations: Search codebase for `std::cout`

## Conclusion

Integrating the console into the GUI is a significant enhancement that improves user experience and simplifies the screenshot workflow. The phased migration approach ensures backwards compatibility while allowing gradual adoption of the new system.

**Recommended Timeline:**
- Phase 1: 1-2 days (basic infrastructure)
- Phase 2: 2-3 days (migration of all log calls)
- Phase 3: 1 day (configuration and testing)
- Phase 4: 1 day (cleanup and documentation)

**Total Estimated Effort:** 5-7 days
