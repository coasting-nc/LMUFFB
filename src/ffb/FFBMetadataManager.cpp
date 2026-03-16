#include "FFBMetadataManager.h"
#include "logging/Logger.h"
#include "io/RestApiProvider.h"

bool FFBMetadataManager::UpdateMetadata(const SharedMemoryObjectOut& data) {
    const char* trackName = data.scoring.scoringInfo.mTrackName;
    const char* vehicleClass = nullptr;
    const char* vehicleName = nullptr;

    if (data.telemetry.playerHasVehicle) {
        int idx = data.telemetry.playerVehicleIdx;
        const VehicleScoringInfoV01& veh = data.scoring.vehScoringInfo[idx];
        vehicleClass = veh.mVehicleClass;
        vehicleName = veh.mVehicleName;

        // Issue #368: Log all fields that might contain brand info if a change is detected
        if (vehicleName && m_last_logged_veh != vehicleName) {
            std::string restBrand = RestApiProvider::Get().GetManufacturer();
            Logger::Get().LogFile("[Metadata] Vehicle Change Detected: '%s' (Class: '%s', PitGroup: '%s', Filename: '%s', REST Brand: '%s')",
                vehicleName, vehicleClass, veh.mPitGroup, veh.mVehFilename, restBrand.c_str());
            m_last_logged_veh = vehicleName;
        }
    }

    return UpdateInternal(vehicleClass, vehicleName, trackName);
}

bool FFBMetadataManager::UpdateInternal(const char* vehicleClass, const char* vehicleName, const char* trackName) {
    std::lock_guard<std::mutex> lock(m_mutex);
    bool changed = false;

    if (vehicleName && std::strcmp(vehicleName, m_vehicle_name) != 0) {
        StringUtils::SafeCopy(m_vehicle_name, STR_BUF_64, vehicleName);
        changed = true;

        // Issue #368: Request manufacturer info from REST API on car change
        RestApiProvider::Get().ResetManufacturer();
        RestApiProvider::Get().ResetSteeringRange(); // Issue #379
        RestApiProvider::Get().RequestManufacturer(6397, m_vehicle_name); // TODO: use configured port
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
