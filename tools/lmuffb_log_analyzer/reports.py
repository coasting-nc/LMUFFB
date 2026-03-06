import pandas as pd
from .models import SessionMetadata
from .analyzers.slope_analyzer import (
    analyze_slope_stability,
    detect_oscillation_events,
    detect_singularities
)
from .analyzers.yaw_analyzer import (
    analyze_yaw_dynamics,
    analyze_clipping
)

def generate_text_report(metadata: SessionMetadata, df: pd.DataFrame) -> str:
    """
    Generate a formatted text report for the session.
    """
    slope_results = analyze_slope_stability(df)
    oscillations = detect_oscillation_events(df)
    singularity_count, worst_slope = detect_singularities(df)
    yaw_results = analyze_yaw_dynamics(df)
    clipping_results = analyze_clipping(df)
    
    report = []
    report.append("=" * 60)
    report.append(" " * 15 + "LMUFFB DIAGNOSTIC REPORT")
    report.append("=" * 60)
    report.append("")
    
    report.append("SESSION INFORMATION")
    report.append("-" * 20)
    report.append(f"Driver:       {metadata.driver_name}")
    report.append(f"Vehicle:      {metadata.vehicle_name}")
    report.append(f"Track:        {metadata.track_name}")
    report.append(f"Date:         {metadata.timestamp}")
    report.append(f"App Version:  {metadata.app_version}")
    report.append("")
    
    report.append("SETTINGS")
    report.append("-" * 20)
    report.append(f"Gain:               {metadata.gain:.2f}")
    report.append(f"Understeer Effect:  {metadata.understeer_effect:.2f}")
    report.append(f"SOP Effect:          {metadata.sop_effect:.2f}")
    report.append(f"Slope Detection:    {'Enabled' if metadata.slope_enabled else 'Disabled'}")
    report.append(f"Slope Sensitivity:  {metadata.slope_sensitivity:.2f}")
    report.append(f"Slope Threshold:    {metadata.slope_threshold:.2f}")
    report.append("")
    
    report.append("SLOPE ANALYSIS")
    report.append("-" * 20)
    report.append(f"Slope Mean:       {slope_results['slope_mean']:.2f}")
    report.append(f"Slope Std Dev:    {slope_results['slope_std']:.2f}")
    report.append(f"Slope Range:      {slope_results['slope_min']:.1f} to {slope_results['slope_max']:.1f}")
    
    if slope_results.get('active_percentage') is not None:
        report.append(f"Active Time:      {slope_results['active_percentage']:.1f}%")
        
    if slope_results.get('floor_percentage') is not None:
        report.append(f"Floor Hits:       {slope_results['floor_percentage']:.1f}%")
        
    report.append(f"Oscillations:      {len(oscillations)} events detected")
    report.append(f"Singularities:     {singularity_count} events detected (Worst: {worst_slope:.1f})")
    report.append("")

    report.append("SIGNAL QUALITY & STABILITY")
    report.append("-" * 20)
    if slope_results.get('zero_crossing_rate') is not None:
        report.append(f"Zero-Crossing Rate: {slope_results['zero_crossing_rate']:.2f} Hz")
    if slope_results.get('binary_residence') is not None:
        report.append(f"Binary Residence:   {slope_results['binary_residence']:.1f}%")
    if slope_results.get('derivative_energy_ratio') is not None:
        report.append(f"D-Energy Ratio:     {slope_results['derivative_energy_ratio']:.2f}")
    report.append("")
    
    report.append("YAW KICK & STABILITY")
    report.append("-" * 20)
    if yaw_results.get('threshold_crossing_rate') is not None:
        report.append(f"Threshold Crossing Rate: {yaw_results['threshold_crossing_rate']:.2f} Hz")
    if yaw_results.get('yaw_kick_contribution_pct') is not None:
        report.append(f"Yaw Kick Contribution:  {yaw_results['yaw_kick_contribution_pct']:.1f}%")
    if yaw_results.get('straightaway_yaw_kick_mean') is not None:
        report.append(f"Straightaway Yaw Mean:   {yaw_results['straightaway_yaw_kick_mean']:.3f} Nm")
    if yaw_results.get('yaw_accel_nan_count', 0) > 0:
        report.append(f"NaN Yaw Accel frames:    {yaw_results['yaw_accel_nan_count']}")
    report.append("")

    report.append("CLIPPING ANALYSIS")
    report.append("-" * 20)
    if clipping_results.get('total_clipping_pct') is not None:
        report.append(f"Total Clipping Time: {clipping_results['total_clipping_pct']:.1f}%")
        if clipping_results.get('clipping_component_means'):
            report.append("Top Contributors during clipping:")
            sorted_comps = sorted(clipping_results['clipping_component_means'].items(),
                                key=lambda x: x[1], reverse=True)[:3]
            for comp, val in sorted_comps:
                report.append(f"  - {comp}: {val:.2f} Nm")
    report.append("")

    # Overall Issues
    all_issues = slope_results['issues'].copy()

    threshold_crossing_rate = yaw_results.get('threshold_crossing_rate')
    if threshold_crossing_rate is not None and threshold_crossing_rate > 5.0:
        all_issues.append(f"HIGH YAW THRASHING ({threshold_crossing_rate:.1f} Hz) - Threshold may be too low or signal too noisy")

    yaw_kick_contribution = yaw_results.get('yaw_kick_contribution_pct')
    if yaw_kick_contribution is not None and yaw_kick_contribution > 30.0:
        all_issues.append(f"EXCESSIVE YAW KICK ({yaw_kick_contribution:.1f}%) - Yaw kick is dominating FFB")

    total_clipping = clipping_results.get('total_clipping_pct')
    if total_clipping is not None and total_clipping > 10.0:
        all_issues.append(f"HIGH CLIPPING ({total_clipping:.1f}%) - Reduce Gain or increase Max Torque Ref")

    if all_issues:
        report.append("ISSUES DETECTED")
        report.append("-" * 20)
        for issue in all_issues:
            report.append(f"  [!] {issue}")
        report.append("")
    else:
        report.append("No significant issues detected.")
        report.append("")
        
    report.append("=" * 60)
    
    return "\n".join(report)
