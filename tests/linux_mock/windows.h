#ifndef WINDOWS_H_MOCK
#define WINDOWS_H_MOCK

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

inline LONG_PTR GetWindowLongPtr(HWND hWnd, int nIndex) { return 0; }
inline BOOL SetWindowPos(HWND hWnd, HWND hWndInsertAfter, int X, int Y, int cx, int cy, UINT uFlags) { return TRUE; }
inline HANDLE GetModuleHandle(const char* lpModuleName) { return (HANDLE)1; }
inline HWND CreateWindowA(const char* lpClassName, const char* lpWindowName, DWORD dwStyle, int x, int y, int nWidth, int nHeight, HWND hWndParent, void* hMenu, HANDLE hInstance, void* lpParam) { return (HWND)1; }
inline BOOL DestroyWindow(HWND hWnd) { return TRUE; }

#endif
