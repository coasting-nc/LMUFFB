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
    lat_load_effect: float = 0.0 # v0.7.152
    long_load_effect: float = 0.0 # v0.7.162
    sop_scale: float = 1.0       # v0.7.152
    sop_smoothing: float = 0.0   # v0.7.152
    optimal_slip_angle: float = 0.10 # v0.7.172
    optimal_slip_ratio: float = 0.12 # v0.7.172
    slope_enabled: bool
    slope_sensitivity: float
    slope_threshold: float
    slope_alpha_threshold: Optional[float] = None
    slope_decay_rate: Optional[float] = None
    dynamic_normalization: bool = False
    auto_load_normalization: bool = False

class MarkerEvent(BaseModel):
    """User-triggered marker"""
    timestamp: float
    frame_index: int
    context: dict
