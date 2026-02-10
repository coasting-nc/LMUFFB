#ifndef DYNAMICVJOY_H
#define DYNAMICVJOY_H

#ifdef _WIN32
#include <windows.h>
#else
#include "lmu_sm_interface/LinuxMock.h"
// Mock types for Linux
typedef void* HMODULE;
#define WINAPI
#endif
#include <iostream>

// vJoy Status Enum (from vJoy SDK, defined here to avoid dependency)
enum VjdStat {
    VJD_STAT_OWN,   // The vJoy Device is owned by this application
    VJD_STAT_FREE,  // The vJoy Device is free
    VJD_STAT_BUSY,  // The vJoy Device is owned by another application
    VJD_STAT_MISS,  // The vJoy Device is missing
    VJD_STAT_UNKN   // Unknown
};

// Typedefs for vJoy functions
typedef BOOL (WINAPI *vJoyEnabled_t)();
typedef BOOL (WINAPI *AcquireVJD_t)(UINT);
typedef VOID (WINAPI *RelinquishVJD_t)(UINT);
typedef BOOL (WINAPI *SetAxis_t)(LONG, UINT, UINT);
typedef enum VjdStat (WINAPI *GetVJDStatus_t)(UINT);
typedef SHORT (WINAPI *GetvJoyVersion_t)();
typedef PVOID (WINAPI *GetvJoyProductString_t)();
typedef PVOID (WINAPI *GetvJoyManufacturerString_t)();
typedef PVOID (WINAPI *GetvJoySerialNumberString_t)();

class DynamicVJoy {
public:
    static DynamicVJoy& Get() {
        static DynamicVJoy instance;
        return instance;
    }

    bool Load() {
        if (m_hModule) return true; // Already loaded

#ifdef _WIN32
        m_hModule = LoadLibraryA("vJoyInterface.dll");
        if (!m_hModule) {
            std::cout << "[vJoy] Library not found. vJoy support disabled." << std::endl;
            return false;
        }

        m_vJoyEnabled = (vJoyEnabled_t)GetProcAddress(m_hModule, "vJoyEnabled");
        m_AcquireVJD = (AcquireVJD_t)GetProcAddress(m_hModule, "AcquireVJD");
        m_RelinquishVJD = (RelinquishVJD_t)GetProcAddress(m_hModule, "RelinquishVJD");
        m_SetAxis = (SetAxis_t)GetProcAddress(m_hModule, "SetAxis");
        m_GetVJDStatus = (GetVJDStatus_t)GetProcAddress(m_hModule, "GetVJDStatus");
        m_GetvJoyVersion = (GetvJoyVersion_t)GetProcAddress(m_hModule, "GetvJoyVersion");
        m_GetvJoyProductString = (GetvJoyProductString_t)GetProcAddress(m_hModule, "GetvJoyProductString");
        m_GetvJoyManufacturerString = (GetvJoyManufacturerString_t)GetProcAddress(m_hModule, "GetvJoyManufacturerString");
        m_GetvJoySerialNumberString = (GetvJoySerialNumberString_t)GetProcAddress(m_hModule, "GetvJoySerialNumberString");

        if (!m_vJoyEnabled || !m_AcquireVJD || !m_RelinquishVJD || !m_SetAxis || !m_GetVJDStatus) {
            std::cerr << "[vJoy] Library loaded but functions missing." << std::endl;
            FreeLibrary(m_hModule);
            m_hModule = NULL;
            return false;
        }

        std::cout << "[vJoy] Library loaded successfully." << std::endl;
        return true;
#else
        return false;
#endif
    }

    bool Enabled() { return (m_hModule && m_vJoyEnabled) ? m_vJoyEnabled() : false; }
    BOOL Acquire(UINT id) { return (m_hModule && m_AcquireVJD) ? m_AcquireVJD(id) : FALSE; }
    VOID Relinquish(UINT id) { if (m_hModule && m_RelinquishVJD) m_RelinquishVJD(id); }
    BOOL SetAxis(LONG value, UINT id, UINT axis) { return (m_hModule && m_SetAxis) ? m_SetAxis(value, id, axis) : FALSE; }
    VjdStat GetStatus(UINT id) { return (m_hModule && m_GetVJDStatus) ? m_GetVJDStatus(id) : VJD_STAT_MISS; }
    
    SHORT GetVersion() { return (m_hModule && m_GetvJoyVersion) ? m_GetvJoyVersion() : 0; }
    const char* GetManufacturerString() { return (m_hModule && m_GetvJoyManufacturerString) ? (const char*)m_GetvJoyManufacturerString() : ""; }
    const char* GetProductString() { return (m_hModule && m_GetvJoyProductString) ? (const char*)m_GetvJoyProductString() : ""; }
    const char* GetSerialNumberString() { return (m_hModule && m_GetvJoySerialNumberString) ? (const char*)m_GetvJoySerialNumberString() : ""; }

    bool IsLoaded() const { return m_hModule != NULL; }

private:
    DynamicVJoy() {}
    ~DynamicVJoy() {
#ifdef _WIN32
        if (m_hModule) FreeLibrary(m_hModule);
#endif
    }

    HMODULE m_hModule = NULL;
    vJoyEnabled_t m_vJoyEnabled = NULL;
    AcquireVJD_t m_AcquireVJD = NULL;
    RelinquishVJD_t m_RelinquishVJD = NULL;
    SetAxis_t m_SetAxis = NULL;
    GetVJDStatus_t m_GetVJDStatus = NULL;
    GetvJoyVersion_t m_GetvJoyVersion = NULL;
    GetvJoyProductString_t m_GetvJoyProductString = NULL;
    GetvJoyManufacturerString_t m_GetvJoyManufacturerString = NULL;
    GetvJoySerialNumberString_t m_GetvJoySerialNumberString = NULL;
};

#endif // DYNAMICVJOY_H
