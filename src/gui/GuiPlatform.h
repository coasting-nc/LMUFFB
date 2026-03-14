#pragma once
#include <string>

class IGuiPlatform {
public:
    virtual ~IGuiPlatform() = default;
    virtual void SetAlwaysOnTop(bool enabled) = 0;
    virtual void ResizeWindow(int x, int y, int w, int h) = 0;
    virtual void SaveWindowGeometry(bool is_graph_mode) = 0;
    virtual bool OpenPresetFileDialog(std::string& outPath) = 0;
    virtual bool SavePresetFileDialog(std::string& outPath, const std::string& defaultName) = 0;
    virtual void* GetWindowHandle() = 0;

    // Test support
    virtual bool GetAlwaysOnTopMock() { return false; }
};

// Singleton access
IGuiPlatform& GetGuiPlatform();

// Global helper for simple access (compatibility)
void SetWindowAlwaysOnTopPlatform(bool enabled);
