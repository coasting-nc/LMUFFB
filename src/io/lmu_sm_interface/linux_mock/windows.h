#ifndef WINDOWS_H_MOCK
#define WINDOWS_H_MOCK

#include "../LinuxMock.h"

// Any additional Windows-specific macros needed by ISI headers
#ifndef CALLBACK
#define CALLBACK __cdecl
#endif

#ifndef WINAPI
#define WINAPI __cdecl
#endif

#endif // WINDOWS_H_MOCK
