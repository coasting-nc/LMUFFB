#ifndef FFBMETADATA_MANAGER_H
#define FFBMETADATA_MANAGER_H

#include <string>
#include <cstring>
#include <atomic>
#include <mutex>
#include "VehicleUtils.h"
#include "utils/StringUtils.h"
#include "io/lmu_sm_interface/LmuSharedMemoryWrapper.h"

class FFBMetadataManager {
public:
    static constexpr int STR_BUF_64 = 64;

    FFBMetadataManager() = default;

    // Updates metadata. Returns true if vehicle class/name changed (meaning FFB should seed/initialize)
    bool UpdateMetadata(const SharedMemoryObjectOut& data);
    
    // Low-level update. 
    // NOTE: This is a hot-path method called from the FFB thread. 
    // It is protected by g_engine_mutex in FFBEngine.
    bool UpdateInternal(const char* vehicleClass, const char* vehicleName, const char* trackName);

    void ResetWarnings() { m_warned_invalid_range = false; }
    
    // API methods for FFBEngine
    const char* GetVehicleName() const { return m_vehicle_name; }
    const char* GetTrackName() const { return m_track_name; }
    const char* GetCurrentClassName() const { return m_current_class_name.c_str(); }
    LMUFFB::ParsedVehicleClass GetCurrentClass() const { return m_current_vclass; }
    bool HasWarnedInvalidRange() const { return m_warned_invalid_range; }
    void SetWarnedInvalidRange(bool val) { m_warned_invalid_range = val; }

    // Internal state (Kept public for legacy test compatibility as per Approach A)
    char m_vehicle_name[STR_BUF_64] = "Unknown";
    char m_track_name[STR_BUF_64] = "Unknown";
    std::string m_current_class_name = "";
    LMUFFB::ParsedVehicleClass m_current_vclass = LMUFFB::ParsedVehicleClass::UNKNOWN;
    std::atomic<bool> m_warned_invalid_range{false};
    std::string m_last_handled_vehicle_name = "";
    std::string m_last_logged_veh = "";

private:
    std::mutex m_mutex;
};

#endif // FFBMETADATA_MANAGER_H
