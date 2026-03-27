#include "FFBMetadataManager.h"
#include "logging/Logger.h"
#include "io/RestApiProvider.h"
#include "physics/VehicleUtils.h"

using namespace LMUFFB::Logging;
using namespace LMUFFB::Utils;
using namespace LMUFFB::Physics;

namespace LMUFFB::FFB {

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
            const char* brand = LMUFFB::Physics::ParseVehicleBrand(vehicleClass, vehicleName);
            Logger::Get().LogFile("[Metadata] Vehicle Change Detected: '%s' (Brand: '%s', Class: '%s', PitGroup: '%s', Filename: '%s')",
                vehicleName, brand, vehicleClass, veh.mPitGroup, veh.mVehFilename);
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

        // Issue #379: Reset steering range on car change
        LMUFFB::IO::RestApiProvider::Get().ResetSteeringRange();
    }

    if (vehicleClass && std::string(vehicleClass) != m_current_class_name) {
        m_current_class_name = vehicleClass;
        changed = true;
    }

    if (changed) {
        m_current_vclass = LMUFFB::Physics::ParseVehicleClass(m_current_class_name.c_str(), m_vehicle_name);
    }

    if (trackName && std::strcmp(trackName, m_track_name) != 0) {
        StringUtils::SafeCopy(m_track_name, STR_BUF_64, trackName);
    }

    return changed;
}

} // namespace LMUFFB::FFB
