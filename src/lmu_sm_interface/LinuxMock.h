#ifndef LINUXMOCK_H
#define LINUXMOCK_H

#ifndef _WIN32
#include <chrono>
#include <optional>
#include <cstdint>
#include <cstring>
#include <algorithm>
#include <map>
#include <vector>
#include <string>
#include <cstdarg>
#include <cstdio>

// Dummy typedefs for Linux compatibility
using DWORD = uint32_t;
using HANDLE = void*;
using HWND = void*;
using BOOL = int;
using UINT = unsigned int;
using LONG = long;
using SHORT = short;
using VOID = void;
using PVOID = void*;
using UINT_PTR = unsigned long;
using LPARAM = long;
using WPARAM = unsigned long;
using LRESULT = long;
using HRESULT = long;
using WORD = uint16_t;
using BYTE = uint8_t;
using LONG_PTR = intptr_t;
using ULONG_PTR = uintptr_t;
using LPDWORD = uint32_t*;
using HMODULE = void*;
using HICON = void*;
using HRSRC = void*;
using LPVOID = void*;
using LPCSTR = const char*;

typedef struct _GUID {
    uint32_t Data1;
    uint16_t Data2;
    uint16_t Data3;
    uint8_t Data4[8];
} GUID;

#ifndef __cdecl
#define __cdecl
#endif

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

#ifndef NULL
#define NULL 0
#endif

#ifndef MAX_PATH
#define MAX_PATH 260
#endif

#ifndef _TRUNCATE
#define _TRUNCATE ((size_t)-1)
#endif

// Windows Constants for Mocking
#define INVALID_HANDLE_VALUE ((HANDLE)(intptr_t)-1)
#define PAGE_READWRITE 0x04
#define FILE_MAP_ALL_ACCESS 0xF001F
#define FILE_MAP_READ 0x04
#define ERROR_ALREADY_EXISTS 183
#define WAIT_OBJECT_0 0
#define WAIT_FAILED 0xFFFFFFFF
#define INFINITE 0xFFFFFFFF
#define SYNCHRONIZE 0x00100000L
#define PROCESS_QUERY_LIMITED_INFORMATION 0x1000

// Window Styles & Constants
#define WS_OVERLAPPEDWINDOW 0
#define WS_VISIBLE 0
#define GWL_EXSTYLE -20
#define WS_EX_TOPMOST 0x00000008L
#define HWND_TOPMOST ((HWND)(intptr_t)-1)
#define HWND_NOTOPMOST ((HWND)(intptr_t)-2)
#define SWP_NOMOVE 0x0002
#define SWP_NOSIZE 0x0001
#define SWP_FRAMECHANGED 0x0020

// Resources
#define RT_GROUP_ICON 14
#define LOAD_LIBRARY_AS_DATAFILE 0x00000002
#define MAKEINTRESOURCE(i) ((char*)(intptr_t)((WORD)(i)))
#define MAKEINTRESOURCEA(i) ((char*)(intptr_t)((WORD)(i)))

// Memory Mapping Mock (Global Storage)
namespace MockSM {
    inline std::map<std::string, std::vector<uint8_t>>& GetMaps() {
        static std::map<std::string, std::vector<uint8_t>> maps;
        return maps;
    }
    inline DWORD& LastError() {
        static DWORD err = 0;
        return err;
    }
    inline bool& FailNext() {
        static bool fail = false;
        return fail;
    }
    inline DWORD& WaitResult() {
        static DWORD res = 0; // WAIT_OBJECT_0
        return res;
    }
}

// Interlocked functions for Linux mocking
inline long InterlockedCompareExchange(long volatile* Destination, long Exchange, long Comparand) {
    long old = *Destination;
    if (old == Comparand) *Destination = Exchange;
    return old;
}
inline long InterlockedIncrement(long volatile* Addend) { return ++(*Addend); }
inline long InterlockedDecrement(long volatile* Addend) { return --(*Addend); }
inline long InterlockedExchange(long volatile* Target, long Value) {
    long old = *Target;
    *Target = Value;
    return old;
}

// Memory and Event mocks
inline void YieldProcessor() {}
inline int sprintf_s(char* buf, size_t size, const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    int ret = vsnprintf(buf, size, fmt, args);
    va_end(args);
    return ret;
}
inline int sprintf_s(char* buf, const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    int ret = vsprintf(buf, fmt, args);
    va_end(args);
    return ret;
}
inline DWORD WaitForSingleObject(HANDLE hHandle, DWORD dwMilliseconds) {
    if (MockSM::WaitResult() != 0) {
        DWORD res = MockSM::WaitResult();
        MockSM::WaitResult() = 0;
        return res;
    }
    return WAIT_OBJECT_0;
}
inline BOOL SetEvent(HANDLE hEvent) { return TRUE; }
inline BOOL CloseHandle(HANDLE hObject) {
    if (hObject != (HANDLE)0 && hObject != (HANDLE)1 && hObject != (HANDLE)2 && hObject != (HANDLE)(intptr_t)-1) {
        delete static_cast<std::string*>(hObject);
    }
    return TRUE;
}
inline DWORD GetLastError() { return MockSM::LastError(); }

// Shared Memory Mocks
inline HANDLE CreateFileMappingA(HANDLE hFile, void* lpAttributes, DWORD flProtect, DWORD dwMaximumSizeHigh, DWORD dwMaximumSizeLow, const char* lpName) {
    if (MockSM::FailNext()) {
        MockSM::FailNext() = false;
        return NULL;
    }
    if (lpName == nullptr) return (HANDLE)1;
    std::string name(lpName);
    auto& maps = MockSM::GetMaps();
    if (maps.find(name) == maps.end()) {
        maps[name].resize(dwMaximumSizeLow);
        MockSM::LastError() = 0;
    } else {
        MockSM::LastError() = ERROR_ALREADY_EXISTS;
    }
    return (HANDLE)new std::string(name);
}

inline HANDLE OpenFileMappingA(DWORD dwDesiredAccess, BOOL bInheritHandle, const char* lpName) {
    if (lpName == nullptr) return nullptr;
    std::string name(lpName);
    auto& maps = MockSM::GetMaps();
    if (maps.find(name) != maps.end()) {
        return (HANDLE)new std::string(name);
    }
    return nullptr;
}

inline void* MapViewOfFile(HANDLE hFileMappingObject, DWORD dwDesiredAccess, DWORD dwFileOffsetHigh, DWORD dwFileOffsetLow, size_t dwNumberOfBytesToMap) {
    if (hFileMappingObject == (HANDLE)1 || hFileMappingObject == nullptr || hFileMappingObject == INVALID_HANDLE_VALUE) return nullptr;
    std::string* name = (std::string*)hFileMappingObject;
    return MockSM::GetMaps()[*name].data();
}

inline BOOL UnmapViewOfFile(const void* lpBaseAddress) { return TRUE; }
inline HANDLE CreateEventA(void* lpEventAttributes, BOOL bManualReset, BOOL bInitialState, const char* lpName) { return (HANDLE)1; }

// Window mocks
namespace MockGUI {
    inline LONG_PTR& ExStyle() { static LONG_PTR style = 0; return style; }
}

inline HWND GetConsoleWindow() { return (HWND)static_cast<intptr_t>(1); }
inline BOOL IsWindow(HWND hWnd) {
    return (hWnd == (HWND)static_cast<intptr_t>(1) || hWnd == (HWND)static_cast<intptr_t>(2)); // Accept our mock handles
}
inline HWND CreateWindowA(const char* lpClassName, const char* lpWindowName, DWORD dwStyle, int x, int y, int nWidth, int nHeight, HWND hWndParent, void* hMenu, HMODULE hInstance, void* lpParam) {
    return (HWND)static_cast<intptr_t>(2);
}
inline LONG_PTR GetWindowLongPtr(HWND hWnd, int nIndex) {
    if (nIndex == GWL_EXSTYLE) return MockGUI::ExStyle();
    return 0;
}
inline BOOL SetWindowPos(HWND hWnd, HWND hWndInsertAfter, int X, int Y, int cx, int cy, UINT uFlags) {
    if (hWndInsertAfter == HWND_TOPMOST) MockGUI::ExStyle() |= WS_EX_TOPMOST;
    if (hWndInsertAfter == HWND_NOTOPMOST) MockGUI::ExStyle() &= ~WS_EX_TOPMOST;
    return TRUE;
}
inline BOOL DestroyWindow(HWND hWnd) { return TRUE; }

// Resource Mocks
inline HMODULE GetModuleHandle(const char* lpModuleName) { return (HMODULE)static_cast<intptr_t>(1); }
inline HICON LoadIcon(HMODULE hInstance, const char* lpIconName) { return (HICON)static_cast<intptr_t>(1); }
inline DWORD GetModuleFileNameA(HMODULE hModule, char* lpFilename, DWORD nSize) {
    strncpy(lpFilename, "LMUFFB.exe", nSize);
    return strlen(lpFilename);
}
inline HMODULE LoadLibraryExA(const char* lpLibFileName, HANDLE hFile, DWORD dwFlags) { return (HMODULE)static_cast<intptr_t>(1); }
inline HRSRC FindResourceA(HMODULE hModule, const char* lpName, const char* lpType) { return (HRSRC)static_cast<intptr_t>(1); }
inline BOOL FreeLibrary(HMODULE hLibModule) { return TRUE; }

// Version Info Mocks
inline DWORD GetFileVersionInfoSizeA(const char* lptstrFilename, LPDWORD lpdwHandle) {
    if (lpdwHandle) *lpdwHandle = 0;
    return 1024;
}
inline BOOL GetFileVersionInfoA(const char* lptstrFilename, DWORD dwHandle, DWORD dwLen, void* lpData) {
    memset(lpData, 0, dwLen);
    return TRUE;
}
inline BOOL VerQueryValueA(const void* pBlock, const char* lpSubBlock, void** lplpBuffer, UINT* puLen) {
    static uint16_t translation[2] = { 0x0409, 0x04B0 }; // English (US), Unicode
    if (strstr(lpSubBlock, "Translation")) {
        *lplpBuffer = &translation;
        if (puLen) *puLen = sizeof(translation);
        return TRUE;
    }
    if (strstr(lpSubBlock, "CompanyName")) {
        *lplpBuffer = (void*)"lmuFFB";
        if (puLen) *puLen = 7;
        return TRUE;
    }
    if (strstr(lpSubBlock, "ProductVersion")) {
        *lplpBuffer = (void*)LMUFFB_VERSION;
        if (puLen) *puLen = (UINT)strlen(LMUFFB_VERSION) + 1;
        return TRUE;
    }
    return FALSE;
}

// D3D11 & DXGI Mocks for Issue #189
typedef enum D3D_DRIVER_TYPE { D3D_DRIVER_TYPE_HARDWARE = 0 } D3D_DRIVER_TYPE;
typedef enum D3D_FEATURE_LEVEL { D3D_FEATURE_LEVEL_10_0 = 0x1000, D3D_FEATURE_LEVEL_11_0 = 0xb000 } D3D_FEATURE_LEVEL;
#define D3D11_SDK_VERSION (7)

typedef struct DXGI_SAMPLE_DESC { UINT Count; UINT Quality; } DXGI_SAMPLE_DESC;
typedef enum DXGI_FORMAT { DXGI_FORMAT_R8G8B8A8_UNORM = 28, DXGI_FORMAT_UNKNOWN = 0 } DXGI_FORMAT;
typedef enum DXGI_USAGE { DXGI_USAGE_RENDER_TARGET_OUTPUT = 1 << 4 } DXGI_USAGE;
typedef enum DXGI_SWAP_EFFECT { DXGI_SWAP_EFFECT_DISCARD = 0, DXGI_SWAP_EFFECT_FLIP_DISCARD = 4 } DXGI_SWAP_EFFECT;
typedef enum DXGI_SWAP_CHAIN_FLAG { DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH = 2 } DXGI_SWAP_CHAIN_FLAG;
typedef enum DXGI_SCALING { DXGI_SCALING_STRETCH = 0 } DXGI_SCALING;
typedef enum DXGI_ALPHA_MODE { DXGI_ALPHA_MODE_UNSPECIFIED = 0 } DXGI_ALPHA_MODE;

typedef struct DXGI_SWAP_CHAIN_DESC1 {
    UINT Width; UINT Height; DXGI_FORMAT Format; BOOL Stereo; DXGI_SAMPLE_DESC SampleDesc;
    DXGI_USAGE BufferUsage; UINT BufferCount; DXGI_SCALING Scaling; DXGI_SWAP_EFFECT SwapEffect;
    DXGI_ALPHA_MODE AlphaMode; UINT Flags;
} DXGI_SWAP_CHAIN_DESC1;

typedef struct DXGI_SWAP_CHAIN_FULLSCREEN_DESC {
    struct { UINT Numerator; UINT Denominator; } RefreshRate;
    int ScanlineOrdering; int Scaling; BOOL Windowed;
} DXGI_SWAP_CHAIN_FULLSCREEN_DESC;

// Mock interfaces
struct IUnknown {
    virtual HRESULT QueryInterface(const GUID& riid, void** ppvObject) = 0;
    virtual ULONG_PTR Release() = 0;
};

struct IDXGIObject : public IUnknown {};
struct IDXGIDevice : public IDXGIObject {
    virtual HRESULT GetAdapter(struct IDXGIAdapter** ppAdapter) = 0;
};
struct IDXGIAdapter : public IDXGIObject {
    virtual HRESULT GetParent(const GUID& riid, void** ppParent) = 0;
};
struct IDXGIFactory2 : public IDXGIObject {
    virtual HRESULT CreateSwapChainForHwnd(IUnknown* pDevice, HWND hWnd, const DXGI_SWAP_CHAIN_DESC1* pDesc, const DXGI_SWAP_CHAIN_FULLSCREEN_DESC* pFullscreenDesc, struct IDXGIOutput* pRestrictToOutput, struct IDXGISwapChain1** ppSwapChain) = 0;
};

// Concrete mock for testing
struct MockDXGIFactory2 : public IDXGIFactory2 {
    HRESULT QueryInterface(const GUID& riid, void** ppvObject) override { return -1; }
    ULONG_PTR Release() override { return 0; }
    HRESULT CreateSwapChainForHwnd(IUnknown* pDevice, HWND hWnd, const DXGI_SWAP_CHAIN_DESC1* pDesc, const DXGI_SWAP_CHAIN_FULLSCREEN_DESC* pFullscreenDesc, struct IDXGIOutput* pRestrictToOutput, struct IDXGISwapChain1** ppSwapChain) override {
        if (pDesc) {
            extern DXGI_SWAP_CHAIN_DESC1 g_captured_swap_chain_desc;
            g_captured_swap_chain_desc = *pDesc;
        }
        return 0; // S_OK
    }
};
struct IDXGISwapChain : public IDXGIObject {
    virtual HRESULT GetBuffer(UINT Buffer, const GUID& riid, void** ppSurface) = 0;
    virtual HRESULT Present(UINT SyncInterval, UINT Flags) = 0;
    virtual HRESULT ResizeBuffers(UINT BufferCount, UINT Width, UINT Height, DXGI_FORMAT NewFormat, UINT SwapChainFlags) = 0;
};
struct IDXGISwapChain1 : public IDXGISwapChain {};

struct ID3D11DeviceChild : public IUnknown {};
struct ID3D11DeviceContext : public ID3D11DeviceChild {
    virtual void OMSetRenderTargets(UINT NumViews, struct ID3D11RenderTargetView* const* ppRenderTargetViews, struct ID3D11DepthStencilView* pDepthStencilView) = 0;
    virtual void ClearRenderTargetView(struct ID3D11RenderTargetView* pRenderTargetView, const float ColorRGBA[4]) = 0;
};
struct ID3D11Device : public IUnknown {
    virtual HRESULT CreateRenderTargetView(struct ID3D11Resource* pResource, const struct D3D11_RENDER_TARGET_VIEW_DESC* pDesc, struct ID3D11RenderTargetView** ppRTView) = 0;
};

#define IID_PPV_ARGS(ppType) GUID(), (void**)(ppType)

#endif // _WIN32

#endif // LINUXMOCK_H
