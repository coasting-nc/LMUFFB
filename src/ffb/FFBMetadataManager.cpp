#include "FFBMetadataManager.h"
#include "logging/Logger.h"

bool FFBMetadataManager::UpdateMetadata(const SharedMemoryObjectOut& data) {
    const char* trackName = data.scoring.scoringInfo.mTrackName;
    const char* vehicleClass = nullptr;
    const char* vehicleName = nullptr;

    if (data.telemetry.playerHasVehicle) {
        int idx = data.telemetry.playerVehicleIdx;
        const VehicleScoringInfoV01& veh = data.scoring.vehScoringInfo[idx];
        vehicleClass = veh.mVehicleClass;
        vehicleName = veh.mVehicleName;
    }

    return UpdateInternal(vehicleClass, vehicleName, trackName);
}

bool FFBMetadataManager::UpdateInternal(const char* vehicleClass, const char* vehicleName, const char* trackName) {
    std::lock_guard<std::mutex> lock(m_mutex);
    bool changed = false;

    if (vehicleName && std::strcmp(vehicleName, m_vehicle_name) != 0) {
        StringUtils::SafeCopy(m_vehicle_name, STR_BUF_64, vehicleName);
        changed = true;
    }

    if (vehicleClass && std::string(vehicleClass) != m_current_class_name) {
        m_current_class_name = vehicleClass;
        changed = true;
    }

    if (changed) {
        m_current_vclass = ParseVehicleClass(m_current_class_name.c_str(), m_vehicle_name);
    }

    if (trackName && std::strcmp(trackName, m_track_name) != 0) {
        StringUtils::SafeCopy(m_track_name, STR_BUF_64, trackName);
    }

    return changed;
}
