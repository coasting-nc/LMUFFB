import pandas as pd
from .models import SessionMetadata
from .analyzers.grip_analyzer import analyze_grip_estimation
from .analyzers.slope_analyzer import (
    analyze_slope_stability,
    detect_oscillation_events,
    detect_singularities
)
from .analyzers.yaw_analyzer import (
    analyze_yaw_dynamics,
    analyze_clipping
)
from .analyzers.lateral_analyzer import (
    analyze_lateral_dynamics,
    analyze_longitudinal_dynamics,
    analyze_load_estimation
)

def generate_text_report(metadata: SessionMetadata, df: pd.DataFrame) -> str:
    """
    Generate a formatted text report for the session.
    """
    slope_results = analyze_slope_stability(df, metadata)
    grip_est_results = analyze_grip_estimation(df, metadata)
    oscillations = detect_oscillation_events(df)
    singularity_count, worst_slope = detect_singularities(df)
    yaw_results = analyze_yaw_dynamics(df)
    clipping_results = analyze_clipping(df)
    lateral_results = analyze_lateral_dynamics(df, metadata)
    long_results = analyze_longitudinal_dynamics(df, metadata)
    load_est_results = analyze_load_estimation(df)
    
    report = []
    report.append("=" * 60)
    report.append(" " * 15 + "LMUFFB DIAGNOSTIC REPORT")
    report.append("=" * 60)
    report.append("")
    
    report.append("SESSION INFORMATION")
    report.append("-" * 20)
    report.append(f"Driver:       {metadata.driver_name}")
    report.append(f"Vehicle:      {metadata.vehicle_name}")
    report.append(f"Car Class:    {metadata.car_class}")
    report.append(f"Car Brand:    {metadata.car_brand}")
    report.append(f"Track:        {metadata.track_name}")
    report.append(f"Date:         {metadata.timestamp}")
    report.append(f"App Version:  {metadata.app_version}")
    report.append(f"Duration:     {df['Time'].max():.1f} seconds")
    report.append("")
    
    report.append("SETTINGS")
    report.append("-" * 20)
    report.append(f"Gain:               {metadata.gain:.2f}")
    report.append(f"Understeer Effect:  {metadata.understeer_effect:.2f}")
    report.append(f"SOP Effect:          {metadata.sop_effect:.2f}")
    report.append(f"Lateral Load Effect: {metadata.lat_load_effect:.2f}")
    report.append(f"Long. Load Effect:   {metadata.long_load_effect:.2f}")
    report.append(f"SOP Scale:           {metadata.sop_scale:.2f}")
    report.append(f"SOP Smoothing:       {metadata.sop_smoothing:.3f}")
    report.append(f"Slope Detection:    {'Enabled' if metadata.slope_enabled else 'Disabled'}")
    report.append(f"Slope Sensitivity:  {metadata.slope_sensitivity:.2f}")
    report.append(f"Slope Threshold:    {metadata.slope_threshold:.2f}")
    report.append(f"Dynamic Norm (Torque): {'Enabled' if metadata.dynamic_normalization else 'Disabled'}")
    report.append(f"Auto Load Norm (Vib):  {'Enabled' if metadata.auto_load_normalization else 'Disabled'}")
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

    if slope_results.get('slope_grip_correlation') is not None:
        report.append(f"Grip Correlation: {slope_results['slope_grip_correlation']:.3f}")
        report.append(f"False Pos. Rate:  {slope_results['false_positive_rate']:.1f}%")

    report.append(f"Oscillations:      {len(oscillations)} events detected")
    report.append(f"Singularities:     {singularity_count} events detected (Worst: {worst_slope:.1f})")
    report.append("")

    report.append("LATERAL LOAD ANALYSIS")
    report.append("-" * 20)
    if lateral_results.get('load_transfer_correlation') is not None:
        report.append(f"Load Transfer Corr:  {lateral_results['load_transfer_correlation']:.3f}")
    if lateral_results.get('lat_g_vs_load_correlation') is not None:
        report.append(f"LatG vs Load Corr:   {lateral_results['lat_g_vs_load_correlation']:.3f}")
    if lateral_results.get('load_contribution_pct') is not None:
        report.append(f"Load Contribution:   {lateral_results['load_contribution_pct']:.1f}%")
        report.append(f"G-Force Contribution: {lateral_results['g_contribution_pct']:.1f}%")
    report.append("")

    report.append("LONGITUDINAL LOAD ANALYSIS")
    report.append("-" * 20)
    if long_results.get('long_load_factor_mean') is not None:
        report.append(f"Multiplier Mean:     {long_results['long_load_factor_mean']:.2f}x")
        report.append(f"Multiplier Range:    {long_results['long_load_factor_min']:.2f}x to {long_results['long_load_factor_max']:.2f}x")
        if not long_results.get('long_load_active', True):
            report.append("Status:              INACTIVE (Multiplier is stuck/clamped)")
        else:
            report.append("Status:              ACTIVE")
    report.append("")

    if long_results.get('missing_data_warnings'):
        report.append("DATA WARNINGS (ENCRYPTED/MISSING TELEMETRY)")
        report.append("-" * 20)
        for warn in long_results['missing_data_warnings']:
            report.append(f"  [!] {warn}")
        report.append("")

    if grip_est_results and 'status' in grip_est_results:
        report.append("GRIP ESTIMATION ACCURACY (Friction Circle Fallback)")
        report.append("-" * 20)
        status = grip_est_results.get('status')
        if status == "ENCRYPTED":
            report.append("Status: Cannot evaluate (Raw telemetry is encrypted/missing).")
        elif status == "NO_SLIP_EVENTS":
            report.append("Status: Cannot evaluate (No significant sliding occurred).")
        elif 'correlation' in grip_est_results:
            report.append(f"Correlation (r):       {grip_est_results['correlation']:.3f}")
            if 'mean_error_during_slip' in grip_est_results:
                report.append(f"Mean Error (Sliding):  {grip_est_results['mean_error_during_slip']:.3f}")
            if 'false_positive_rate' in grip_est_results:
                report.append(f"False Positive Rate:   {grip_est_results['false_positive_rate']:.1f}%")

            if grip_est_results['correlation'] > 0.85:
                report.append("Evaluation:            EXCELLENT (Fallback closely matches game engine)")
            elif grip_est_results['correlation'] > 0.70:
                report.append("Evaluation:            GOOD (Fallback is viable)")
            else:
                report.append("Evaluation:            POOR (Consider tuning Optimal Slip Angle/Ratio)")
        report.append("")

    if load_est_results:
        report.append("LOAD ESTIMATION ACCURACY (Fallback Diagnostics)")
        report.append("-" * 20)
        report.append("Note: FFB relies on the 'Dynamic Ratio' (Current / Static), not absolute Newtons.")
        report.append(f"Absolute Mean Error: {load_est_results.get('load_error_mean', 0):.1f} N")
        report.append(f"Dynamic Ratio Error: {load_est_results.get('ratio_error_mean', 0):.3f}x")
        report.append(f"Dynamic Correlation: {load_est_results.get('ratio_correlation', 0):.3f}")

        if load_est_results.get('ratio_correlation', 0) > 0.90:
            report.append("Status:              EXCELLENT (Approximation perfectly matches real dynamics)")
        elif load_est_results.get('ratio_correlation', 0) > 0.75:
            report.append("Status:              GOOD (Approximation is viable for FFB)")
        else:
            report.append("Status:              POOR (Approximation dynamics do not match reality)")
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
    if 'issues' in grip_est_results:
        all_issues.extend(grip_est_results['issues'])
    if 'issues' in lateral_results:
        all_issues.extend(lateral_results['issues'])

    threshold_crossing_rate = yaw_results.get('threshold_crossing_rate')
    if threshold_crossing_rate is not None and threshold_crossing_rate > 5.0:
        all_issues.append(f"HIGH YAW THRASHING ({threshold_crossing_rate:.1f} Hz) - Threshold may be too low or signal too noisy")

    yaw_kick_contribution = yaw_results.get('yaw_kick_contribution_pct')
    if yaw_kick_contribution is not None and yaw_kick_contribution > 30.0:
        all_issues.append(f"EXCESSIVE YAW KICK ({yaw_kick_contribution:.1f}%) - Yaw kick is dominating FFB")

    total_clipping = clipping_results.get('total_clipping_pct')
    if total_clipping is not None and total_clipping > 10.0:
        all_issues.append(f"HIGH CLIPPING ({total_clipping:.1f}%) - Reduce Gain or increase Max Torque Ref")

    if long_results.get('straight_brake_issue'):
        all_issues.append("LONGITUDINAL LOAD: Braking in a straight line produces near-zero FFB because the effect is a multiplier on base steering force (which is zero when driving straight). Turn the wheel slightly to feel the weight transfer.")

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
