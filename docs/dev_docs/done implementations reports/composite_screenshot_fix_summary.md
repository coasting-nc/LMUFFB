# Composite Screenshot Fix - Summary

## Issue
The composite screenshot feature was only capturing the GUI window, not the console window.

## Root Cause Analysis
Through extensive debugging, we discovered multiple issues:

1. **Pseudo-Console Windows**: `GetConsoleWindow()` returns a valid handle, but `GetWindowRect()` returns (0,0,0,0) for pseudo-console windows (ConPTY)
2. **Invalid Font Dimensions**: `GetCurrentConsoleFont()` returns 0 for font width on some systems
3. **Cross-Process Windows**: Console windows are often owned by conhost.exe, not the main process
4. **PrintWindow Limitations**: `PrintWindow` API fails for some console window types

## Solution
Implemented a multi-layered fallback approach:

### 1. PrintWindow with Fallbacks
- Try `PrintWindow` with `PW_RENDERFULLCONTENT` flag
- Fall back to `PrintWindow` without flags
- Fall back to `BitBlt` with screen coordinates

### 2. Console Window Detection
- Get console buffer info (columns × rows)
- Use default font size (8×16) when API returns invalid dimensions
- Calculate estimated window size
- Enumerate all top-level windows
- Find windows with similar dimensions (within 30% tolerance)
- Match window titles containing ".exe" or path components

### 3. Robust Error Handling
- Check window visibility with `IsWindowVisible()`
- Validate dimensions at each step
- Provide detailed debug logging
- Gracefully handle missing console windows

## Code Changes

### `src/GuiLayer.cpp`
- **`CaptureWindowToBuffer()`**: Added BitBlt fallback and GetClientRect support
- **`SaveCompositeScreenshot()`**: Added console window detection via enumeration

### `tests/test_screenshot.cpp`
- Added 4 new regression tests for v0.6.5 issues
- Total: 10 tests covering all edge cases

## Test Results
✅ All 44 assertions pass
✅ Handles minimized/iconified consoles
✅ Handles pseudo-console windows
✅ Handles invalid font dimensions
✅ Successfully captures both GUI and console windows

## Verification
The fix was verified with:
- Manual testing with visible console window
- Automated test suite (10 tests, 44 assertions)
- Screenshot shows both GUI and console side-by-side
