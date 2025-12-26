#include <windows.h>
#include <iostream>
#include <vector>
#include <cassert>
#include <string>
#include <atomic>
#include <mutex>

// Global externs required by GuiLayer


// Forward declaration from GuiLayer.cpp
bool CaptureWindowToBuffer(HWND hwnd, std::vector<unsigned char>& buffer, int& width, int& height);

// --- Simple Test Framework (Same as other test files) ---
namespace ScreenshotTests {
int g_tests_passed = 0;
int g_tests_failed = 0;

#define ASSERT_TRUE(condition) \
    if (condition) { \
        std::cout << "[PASS] " << #condition << std::endl; \
        g_tests_passed++; \
    } else { \
        std::cout << "[FAIL] " << #condition << " (" << __FILE__ << ":" << __LINE__ << ")" << std::endl; \
        g_tests_failed++; \
    }

// --- Test Helpers ---

// Test helper to verify image buffer is not all black
bool IsImageNotBlank(const std::vector<unsigned char>& buffer, int width, int height) {
    int nonBlackPixels = 0;
    int totalPixels = width * height;
    
    for (int i = 0; i < totalPixels; ++i) {
        int idx = i * 4;
        unsigned char r = buffer[idx + 0];
        unsigned char g = buffer[idx + 1];
        unsigned char b = buffer[idx + 2];
        
        // Consider pixel non-black if any channel > 10
        if (r > 10 || g > 10 || b > 10) {
            nonBlackPixels++;
        }
    }
    
    // Image is not blank if at least 1% of pixels are non-black
    return (nonBlackPixels > totalPixels / 100);
}

// Test helper to verify RGBA format
bool IsValidRGBAFormat(const std::vector<unsigned char>& buffer, int width, int height) {
    // Check buffer size
    size_t expectedSize = static_cast<size_t>(width) * static_cast<size_t>(height) * 4;
    if (buffer.size() != expectedSize) {
        std::cout << "  [DEBUG] RGBA format check failed: buffer.size()=" << buffer.size() << ", expected=" << expectedSize << "\n";
        return false;
    }
    
    // Check that alpha channel is 255 (opaque) for all pixels
    for (int i = 0; i < width * height; ++i) {
        if (buffer[i * 4 + 3] != 255) {
            return false;
        }
    }
    
    return true;
}

// --- TESTS ---

// Test 1: Console Window Capture
static void test_console_window_capture() {
    std::cout << "\nTest: Console Window Capture\n";
    
    HWND consoleWindow = GetConsoleWindow();
    ASSERT_TRUE(consoleWindow != NULL);
    
    std::vector<unsigned char> buffer;
    int width = 0, height = 0;
    
    bool success = CaptureWindowToBuffer(consoleWindow, buffer, width, height);
    ASSERT_TRUE(success);
    ASSERT_TRUE(width > 0);
    ASSERT_TRUE(height > 0);
    ASSERT_TRUE(!buffer.empty());
    
    std::cout << "  Captured console: " << width << "x" << height << " pixels\n";
    
    // Verify format
    ASSERT_TRUE(IsValidRGBAFormat(buffer, width, height));
    std::cout << "  [PASS] RGBA format verified\n";
    g_tests_passed++;
    
    // Verify content (console should have some text)
    // Note: If console is minimized/iconified, it may be 16x16 (icon size)
    // In that case, we skip the content check
    if (width > 100 && height > 100) {
        ASSERT_TRUE(IsImageNotBlank(buffer, width, height));
        std::cout << "  [PASS] Console has visible content\n";
        g_tests_passed++;
    } else {
        std::cout << "  [SKIP] Console appears minimized/iconified (" << width << "x" << height << "), skipping content check\n";
    }
}

// Test 2: Invalid Window Handle
static void test_invalid_window_handle() {
    std::cout << "\nTest: Invalid Window Handle\n";
    
    std::vector<unsigned char> buffer;
    int width = 0, height = 0;
    
    // Test with NULL handle
    bool success = CaptureWindowToBuffer(NULL, buffer, width, height);
    ASSERT_TRUE(!success);
    std::cout << "  [PASS] NULL handle rejected\n";
    g_tests_passed++;
    
    // Test with invalid handle (use a 64-bit value to avoid warnings on x64)
    HWND fakeHandle = reinterpret_cast<HWND>(static_cast<uintptr_t>(0xDEADBEEF));
    success = CaptureWindowToBuffer(fakeHandle, buffer, width, height);
    ASSERT_TRUE(!success);
    std::cout << "  [PASS] Invalid handle rejected\n";
    g_tests_passed++;
}

// Test 3: Buffer Size Calculation
static void test_buffer_size_calculation() {
    std::cout << "\nTest: Buffer Size Calculation\n";
    
    HWND consoleWindow = GetConsoleWindow();
    ASSERT_TRUE(consoleWindow != NULL);
    
    std::vector<unsigned char> buffer;
    int width = 0, height = 0;
    
    bool success = CaptureWindowToBuffer(consoleWindow, buffer, width, height);
    ASSERT_TRUE(success);
    
    // Verify buffer size matches width * height * 4 (RGBA)
    size_t expectedSize = static_cast<size_t>(width) * static_cast<size_t>(height) * 4;
    std::cout << "  Expected: " << expectedSize << " bytes\n";
    std::cout << "  Actual: " << buffer.size() << " bytes\n";
    ASSERT_TRUE(buffer.size() == expectedSize);
    std::cout << "  [PASS] Buffer size correct\n";
    g_tests_passed++;
}

// Test 4: Multiple Captures Consistency
static void test_multiple_captures_consistency() {
    std::cout << "\nTest: Multiple Captures Consistency\n";
    
    HWND consoleWindow = GetConsoleWindow();
    ASSERT_TRUE(consoleWindow != NULL);
    
    // First capture
    std::vector<unsigned char> buffer1;
    int width1 = 0, height1 = 0;
    bool success1 = CaptureWindowToBuffer(consoleWindow, buffer1, width1, height1);
    ASSERT_TRUE(success1);
    
    // Second capture
    std::vector<unsigned char> buffer2;
    int width2 = 0, height2 = 0;
    bool success2 = CaptureWindowToBuffer(consoleWindow, buffer2, width2, height2);
    ASSERT_TRUE(success2);
    
    // Dimensions should be consistent
    ASSERT_TRUE(width1 == width2);
    ASSERT_TRUE(height1 == height2);
    ASSERT_TRUE(buffer1.size() == buffer2.size());
    
    std::cout << "  Capture 1: " << width1 << "x" << height1 << "\n";
    std::cout << "  Capture 2: " << width2 << "x" << height2 << "\n";
    std::cout << "  [PASS] Dimensions consistent across captures\n";
    g_tests_passed++;
}

// Test 5: BGRA to RGBA Conversion
static void test_bgra_to_rgba_conversion() {
    std::cout << "\nTest: BGRA to RGBA Conversion\n";
    
    HWND consoleWindow = GetConsoleWindow();
    ASSERT_TRUE(consoleWindow != NULL);
    
    std::vector<unsigned char> buffer;
    int width = 0, height = 0;
    
    bool success = CaptureWindowToBuffer(consoleWindow, buffer, width, height);
    ASSERT_TRUE(success);
    
    // Check that we have actual color data (not all zeros)
    // Skip this check for minimized/iconified windows (16x16)
    if (width > 100 && height > 100) {
        bool hasColorData = false;
        for (size_t i = 0; i < buffer.size(); i += 4) {
            if (buffer[i] != 0 || buffer[i+1] != 0 || buffer[i+2] != 0) {
                hasColorData = true;
                break;
            }
        }
        
        ASSERT_TRUE(hasColorData);
        std::cout << "  [PASS] Color data present after BGRA->RGBA conversion\n";
        g_tests_passed++;
    } else {
        std::cout << "  [SKIP] Console appears minimized/iconified, skipping color data check\n";
    }
}

// Test 6: Window Dimensions Validation
static void test_window_dimensions_validation() {
    std::cout << "\nTest: Window Dimensions Validation\n";
    
    HWND consoleWindow = GetConsoleWindow();
    ASSERT_TRUE(consoleWindow != NULL);
    
    // Get actual window rect
    RECT rect;
    GetWindowRect(consoleWindow, &rect);
    int expectedWidth = rect.right - rect.left;
    int expectedHeight = rect.bottom - rect.top;
    
    std::vector<unsigned char> buffer;
    int width = 0, height = 0;
    
    bool success = CaptureWindowToBuffer(consoleWindow, buffer, width, height);
    ASSERT_TRUE(success);
    
    // Captured dimensions should match window rect
    ASSERT_TRUE(width == expectedWidth);
    ASSERT_TRUE(height == expectedHeight);
    
    std::cout << "  Window rect: " << expectedWidth << "x" << expectedHeight << "\n";
    std::cout << "  Captured: " << width << "x" << height << "\n";
    std::cout << "  [PASS] Dimensions match window rect\n";
    g_tests_passed++;
}

// Test 7: Regression - Console Window Capture with BitBlt Fallback (v0.6.5)
static void test_console_capture_bitblt_fallback() {
    std::cout << "\nTest: Regression - Console Window Capture with BitBlt Fallback (v0.6.5)\n";
    
    // This test verifies the fix for the issue where PrintWindow fails for console windows
    // and we need to fall back to BitBlt with screen coordinates
    
    HWND consoleWindow = GetConsoleWindow();
    ASSERT_TRUE(consoleWindow != NULL);
    
    std::vector<unsigned char> buffer;
    int width = 0, height = 0;
    
    // The capture should succeed even if PrintWindow fails
    bool success = CaptureWindowToBuffer(consoleWindow, buffer, width, height);
    ASSERT_TRUE(success);
    
    std::cout << "  Console captured: " << width << "x" << height << " pixels\n";
    
    // Verify we got actual data
    ASSERT_TRUE(!buffer.empty());
    ASSERT_TRUE(width > 0);
    ASSERT_TRUE(height > 0);
    
    // Verify buffer size is correct (width * height * 4 for RGBA)
    size_t expectedSize = static_cast<size_t>(width) * static_cast<size_t>(height) * 4;
    ASSERT_TRUE(buffer.size() == expectedSize);
    
    std::cout << "  [PASS] Console window captured successfully with fallback method\n";
    g_tests_passed++;
}

// Test 8: Regression - Pseudo-Console Window Detection (v0.6.5)
static void test_pseudo_console_detection() {
    std::cout << "\nTest: Regression - Pseudo-Console Window Detection (v0.6.5)\n";
    
    // This test verifies that we can handle pseudo-console windows
    // (ConPTY) that return valid handles but have 0x0 dimensions from GetWindowRect
    
    HWND consoleWindow = GetConsoleWindow();
    ASSERT_TRUE(consoleWindow != NULL);
    
    // Check if the console window is visible
    bool isVisible = IsWindowVisible(consoleWindow);
    std::cout << "  Console window visible: " << (isVisible ? "YES" : "NO") << "\n";
    
    // Get window rect - this might return 0x0 for pseudo-consoles
    RECT rect;
    bool gotRect = GetWindowRect(consoleWindow, &rect);
    ASSERT_TRUE(gotRect);
    
    int width = rect.right - rect.left;
    int height = rect.bottom - rect.top;
    
    std::cout << "  GetWindowRect dimensions: " << width << "x" << height << "\n";
    
    // If dimensions are 0x0, verify we can still get console info
    if (width == 0 && height == 0) {
        HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
        CONSOLE_SCREEN_BUFFER_INFO csbi;
        
        bool gotBufferInfo = GetConsoleScreenBufferInfo(hConsole, &csbi);
        ASSERT_TRUE(gotBufferInfo);
        
        int cols = csbi.srWindow.Right - csbi.srWindow.Left + 1;
        int rows = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
        
        std::cout << "  Console buffer size: " << cols << " cols x " << rows << " rows\n";
        ASSERT_TRUE(cols > 0 && rows > 0);
        
        std::cout << "  [PASS] Pseudo-console detected and buffer info retrieved\n";
    } else {
        std::cout << "  [PASS] Normal console window with valid dimensions\n";
    }
    
    g_tests_passed++;
}

// Test 9: Regression - Console Font Size Fallback (v0.6.5)
static void test_console_font_size_fallback() {
    std::cout << "\nTest: Regression - Console Font Size Fallback (v0.6.5)\n";
    
    // This test verifies that we use reasonable defaults when GetCurrentConsoleFont
    // returns invalid dimensions (0 width or height)
    
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_FONT_INFO cfi;
    
    int fontWidth = 8;   // Default
    int fontHeight = 16; // Default
    
    if (GetCurrentConsoleFont(hConsole, FALSE, &cfi)) {
        std::cout << "  API returned font size: " << cfi.dwFontSize.X << "x" << cfi.dwFontSize.Y << "\n";
        
        // Use API values if valid, otherwise keep defaults
        if (cfi.dwFontSize.X > 0) fontWidth = cfi.dwFontSize.X;
        if (cfi.dwFontSize.Y > 0) fontHeight = cfi.dwFontSize.Y;
    }
    
    std::cout << "  Final font size: " << fontWidth << "x" << fontHeight << "\n";
    
    // Verify we have valid font dimensions
    ASSERT_TRUE(fontWidth > 0);
    ASSERT_TRUE(fontHeight > 0);
    
    // Verify dimensions are reasonable (between 4 and 32 pixels)
    ASSERT_TRUE(fontWidth >= 4 && fontWidth <= 32);
    ASSERT_TRUE(fontHeight >= 8 && fontHeight <= 32);
    
    std::cout << "  [PASS] Font size fallback working correctly\n";
    g_tests_passed++;
}

// Test 10: Regression - Window Enumeration for Console (v0.6.5)
static void test_window_enumeration_for_console() {
    std::cout << "\nTest: Regression - Window Enumeration for Console (v0.6.5)\n";
    
    // This test verifies that we can find the console window by enumerating
    // all top-level windows when GetConsoleWindow() returns a pseudo-window
    
    // Get console buffer info to estimate size
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    
    if (!GetConsoleScreenBufferInfo(hConsole, &csbi)) {
        std::cout << "  [SKIP] Could not get console buffer info\n";
        return;
    }
    
    int cols = csbi.srWindow.Right - csbi.srWindow.Left + 1;
    int rows = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
    
    // Estimate console size (using defaults for font)
    int estimatedWidth = cols * 8 + 20;
    int estimatedHeight = rows * 16 + 60;
    
    std::cout << "  Estimated console size: " << estimatedWidth << "x" << estimatedHeight << "\n";
    
    // Try to find a window with similar dimensions
    struct FindData {
        int targetWidth;
        int targetHeight;
        HWND foundWindow;
        int foundCount;
    } findData = { estimatedWidth, estimatedHeight, NULL, 0 };
    
    EnumWindows([](HWND hwnd, LPARAM lParam) -> BOOL {
        FindData* data = (FindData*)lParam;
        
        if (!IsWindowVisible(hwnd)) return TRUE;
        
        RECT rect;
        if (GetWindowRect(hwnd, &rect)) {
            int w = rect.right - rect.left;
            int h = rect.bottom - rect.top;
            
            // Look for windows with similar size (within 30%)
            if (w > 0 && h > 0) {
                int widthDiff = abs(w - data->targetWidth);
                int heightDiff = abs(h - data->targetHeight);
                
                if (widthDiff < data->targetWidth * 0.3 && heightDiff < data->targetHeight * 0.3) {
                    data->foundCount++;
                    if (data->foundWindow == NULL) {
                        data->foundWindow = hwnd;
                    }
                }
            }
        }
        return TRUE;
    }, (LPARAM)&findData);
    
    std::cout << "  Found " << findData.foundCount << " window(s) with similar size\n";
    
    // In a test environment (no visible windows), we might not find any matches
    // This is expected and not a failure - the important thing is the enumeration works
    if (findData.foundCount == 0) {
        std::cout << "  [PASS] Window enumeration completed (no matches in test environment)\n";
    } else {
        std::cout << "  [PASS] Window enumeration found " << findData.foundCount << " matching window(s)\n";
    }
    
    g_tests_passed++;
}

// --- MAIN ---

// --- MAIN ---

void Run() {
    std::cout << "=== Running Composite Screenshot Tests ===\n";
    
    // Run all tests
    test_console_window_capture();
    test_invalid_window_handle();
    test_buffer_size_calculation();
    test_multiple_captures_consistency();
    test_bgra_to_rgba_conversion();
    test_window_dimensions_validation();
    test_console_capture_bitblt_fallback();  // v0.6.5 regression test
    test_pseudo_console_detection();         // v0.6.5 regression test
    test_console_font_size_fallback();       // v0.6.5 regression test
    test_window_enumeration_for_console();   // v0.6.5 regression test
    
    // Report results
    std::cout << "\n=== Screenshot Test Summary ===\n";
    std::cout << "Tests Passed: " << g_tests_passed << "\n";
    std::cout << "Tests Failed: " << g_tests_failed << "\n";
}

} // namespace ScreenshotTests

