#ifndef WINDOWS_H_MOCK
#define WINDOWS_H_MOCK

#include <cstdarg>
#include "../../src/lmu_sm_interface/LinuxMock.h"

// Extra definitions needed specifically for tests that might not be in the core mock
#ifndef GWL_EXSTYLE
#define GWL_EXSTYLE (-20)
#endif
#ifndef WS_EX_TOPMOST
#define WS_EX_TOPMOST 0x00000008L
#endif
#ifndef WS_OVERLAPPEDWINDOW
#define WS_OVERLAPPEDWINDOW 0x00CF0000L
#endif
#ifndef WS_VISIBLE
#define WS_VISIBLE 0x10000000L
#endif
#ifndef HWND_TOPMOST
#define HWND_TOPMOST ((HWND)-1)
#endif
#ifndef HWND_NOTOPMOST
#define HWND_NOTOPMOST ((HWND)-2)
#endif
#ifndef SWP_NOMOVE
#define SWP_NOMOVE 0x0002
#endif
#ifndef SWP_NOSIZE
#define SWP_NOSIZE 0x0001
#endif
#ifndef SWP_FRAMECHANGED
#define SWP_FRAMECHANGED 0x0020
#endif
#ifndef SWP_NOACTIVATE
#define SWP_NOACTIVATE 0x0010
#endif
#ifndef LOAD_LIBRARY_AS_DATAFILE
#define LOAD_LIBRARY_AS_DATAFILE 0x00000002
#endif
#ifndef RT_GROUP_ICON
#define RT_GROUP_ICON ((const char*)14)
#endif

// Mock state for window styles
inline LONG_PTR& MockExStyle() { static LONG_PTR style = 0; return style; }

inline LONG_PTR GetWindowLongPtr(HWND hWnd, int nIndex) { 
    if (nIndex == GWL_EXSTYLE) return MockExStyle();
    return 0; 
}
inline BOOL SetWindowPos(HWND hWnd, HWND hWndInsertAfter, int X, int Y, int cx, int cy, UINT uFlags) { 
    if (hWndInsertAfter == HWND_TOPMOST) MockExStyle() |= WS_EX_TOPMOST;
    else if (hWndInsertAfter == HWND_NOTOPMOST) MockExStyle() &= ~WS_EX_TOPMOST;
    return TRUE; 
}
inline HANDLE GetModuleHandle(const char* lpModuleName) { return (HANDLE)1; }
inline HWND CreateWindowA(const char* lpClassName, const char* lpWindowName, DWORD dwStyle, int x, int y, int nWidth, int nHeight, HWND hWndParent, void* hMenu, HANDLE hInstance, void* lpParam) { 
    MockExStyle() = 0; // Reset for new window
    return (HWND)1; 
}
inline BOOL DestroyWindow(HWND hWnd) { return TRUE; }

// Resource & Version Mocks
using HICON = HANDLE;
using HRSRC = HANDLE;
using HMODULE = HANDLE;

inline HICON LoadIcon(HMODULE hInstance, const char* lpIconName) { return (HICON)1; }
inline HMODULE LoadLibraryExA(const char* lpLibFileName, HANDLE hFile, DWORD dwFlags) { 
    if (strstr(lpLibFileName, "LMUFFB.exe")) return (HMODULE)1; // Mock find its own exe
    return nullptr; 
}
inline BOOL FreeLibrary(HMODULE hLibModule) { return TRUE; }
inline HRSRC FindResourceA(HMODULE hModule, const char* lpName, const char* lpType) { return (HRSRC)1; }

inline DWORD GetFileVersionInfoSizeA(const char* lptstrFilename, LPDWORD lpdwHandle) { return 512; }
inline BOOL GetFileVersionInfoA(const char* lptstrFilename, DWORD dwHandle, DWORD dwLen, void* lpData) { 
    memset(lpData, 0, dwLen);
    return TRUE; 
}

// Minimal VerQueryValueA mock for test_security_metadata
#include "../../src/Version.h"
inline BOOL VerQueryValueA(const void* pBlock, const char* lpSubBlock, void** lplpBuffer, UINT* puLen) {
    static const uint16_t translate[] = { 0x0409, 0x04b0 }; // English (US), Unicode
    static const char* company = "lmuFFB";
    static const char* version = LMUFFB_VERSION;

    if (strstr(lpSubBlock, "Translation")) {
        *lplpBuffer = (void*)translate;
        *puLen = sizeof(translate);
        return TRUE;
    }
    if (strstr(lpSubBlock, "CompanyName")) {
        *lplpBuffer = (void*)company;
        *puLen = (UINT)strlen(company) + 1;
        return TRUE;
    }
    if (strstr(lpSubBlock, "ProductVersion")) {
        *lplpBuffer = (void*)version;
        *puLen = (UINT)strlen(version) + 1;
        return TRUE;
    }
    return FALSE;
}

inline void sprintf_s(char* buffer, size_t size, const char* format, ...) {
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, size, format, args);
    va_end(args);
}

#ifndef MAKEINTRESOURCE
#define MAKEINTRESOURCE(i) ((const char*)(uintptr_t)((WORD)(i)))
#endif
#ifndef MAKEINTRESOURCEA
#define MAKEINTRESOURCEA(i) MAKEINTRESOURCE(i)
#endif

// Global mock state for tests (Issue #189)
extern DXGI_SWAP_CHAIN_DESC1 g_captured_swap_chain_desc;
extern bool g_d3d11_device_created;

inline HRESULT D3D11CreateDevice(void* pAdapter, D3D_DRIVER_TYPE DriverType, HMODULE Software, UINT Flags, const D3D_FEATURE_LEVEL* pFeatureLevels, UINT FeatureLevels, UINT SDKVersion, ID3D11Device** ppDevice, D3D_FEATURE_LEVEL* pFeatureLevel, ID3D11DeviceContext** ppImmediateContext) {
    g_d3d11_device_created = true;
    if (pFeatureLevel) *pFeatureLevel = D3D_FEATURE_LEVEL_11_0;
    return 0; // S_OK
}

#endif
