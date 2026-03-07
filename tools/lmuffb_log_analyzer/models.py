from pydantic import BaseModel
from typing import Optional, List
from datetime import datetime

class SessionMetadata(BaseModel):
    """Extracted from log file header"""
    log_version: str
    timestamp: datetime
    app_version: str
    driver_name: str
    vehicle_name: str
    car_class: str = "Unknown" # v1.2.1
    car_brand: str = "Unknown" # v1.2.1
    track_name: str
    
    # Settings snapshot
    gain: float
    understeer_effect: float
    sop_effect: float
    lat_load_effect: float = 0.0 # New v1.2.2
    sop_scale: float = 1.0       # New v1.2.2
    slope_enabled: bool
    slope_sensitivity: float
    slope_threshold: float
    slope_alpha_threshold: Optional[float] = None
    slope_decay_rate: Optional[float] = None

class MarkerEvent(BaseModel):
    """User-triggered marker"""
    timestamp: float
    frame_index: int
    context: dict
