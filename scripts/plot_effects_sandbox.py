
import numpy as np
import matplotlib.pyplot as plt
from matplotlib.widgets import Slider

def plot_lateral_load():
    # Create a figure with 2 subplots side-by-side
    fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(16, 6))

    # ==========================================
    # PLOT 1: The Transfer Function (Left)
    # ==========================================
    # Input range for normalized lateral load (-1.0 to 1.0)
    x_transfer = np.linspace(-1.0, 1.0, 500)

    y1_linear = x_transfer
    y1_cubic = 1.5 * x_transfer - 0.5 * (x_transfer ** 3)
    y1_sine = np.sin(x_transfer * np.pi / 2)

    ax1.plot(x_transfer, y1_linear, label='Unchanged (Linear)', color='gray', linestyle='--', linewidth=2)
    ax1.plot(x_transfer, y1_cubic, label='Cubic Polynomial ($1.5x - 0.5x^3$)', color='dodgerblue', linewidth=2.5)
    ax1.plot(x_transfer, y1_sine, label='Sine Curve ($\sin(x \cdot \pi/2)$)', color='darkorange', linestyle='-.', linewidth=2.5)

    ax1.axhline(0, color='black', linewidth=1)
    ax1.axvline(0, color='black', linewidth=1)
    ax1.grid(True, linestyle=':', alpha=0.7)

    ax1.set_title('1. Transfer Function (Math)', fontsize=14, fontweight='bold')
    ax1.set_xlabel('Raw Normalized Load Transfer (Input)', fontsize=12)
    ax1.set_ylabel('Transformed FFB Output', fontsize=12)

    ax1.axvspan(0.8, 1.0, color='red', alpha=0.1, label='Peak Load Area (Notchy Zone)')
    ax1.axvspan(-1.0, -0.8, color='red', alpha=0.1)

    ax1.legend(fontsize=11, loc='upper left')
    ax1.set_xlim(-1.1, 1.1)
    ax1.set_ylim(-1.1, 1.1)


    # ==========================================
    # PLOT 2: The "Bell Curve" (Right)
    # ==========================================
    # Simulate Steering Angle / Slip Angle from 0 to 2.5
    x_steer = np.linspace(0, 2.5, 500)

    # Simulate the Raw Load Transfer (Tire slip curve: x * e^(1-x))
    raw_load = x_steer * np.exp(1 - x_steer)

    y2_linear = raw_load
    y2_cubic = 1.5 * raw_load - 0.5 * (raw_load ** 3)
    y2_sine = np.sin(raw_load * np.pi / 2)

    ax2.plot(x_steer, y2_linear, label='Raw Formula (Sharp Peak / Notchy)', color='gray', linestyle='--', linewidth=2)
    ax2.plot(x_steer, y2_cubic, label='Cubic Transformed (Smoother Peak)', color='dodgerblue', linewidth=2.5)
    ax2.plot(x_steer, y2_sine, label='Sine Transformed', color='darkorange', linestyle='-.', linewidth=2.5)

    ax2.axhline(0, color='black', linewidth=1)
    ax2.grid(True, linestyle=':', alpha=0.7)

    ax2.set_title('2. FFB Force vs. Steering Angle (Physics)', fontsize=14, fontweight='bold')
    ax2.set_xlabel('Steering Angle / Slip Angle (1.0 = Limit of Grip)', fontsize=12)
    ax2.set_ylabel('Final FFB Output Force', fontsize=12)

    ax2.axvspan(0.7, 1.4, color='red', alpha=0.1, label='The Limit (Transition to Understeer)')

    ax2.legend(fontsize=11, loc='lower right')
    ax2.set_xlim(0, 2.5)
    ax2.set_ylim(0, 1.05)

    # ==========================================
    # Final Rendering
    # ==========================================
    plt.tight_layout()
    plt.show()

def plot_lateral_load_bellcurve():
    # Simulate Steering Angle (or Slip Angle) from 0 to 2.5
    # 0.0 = Driving straight
    # 1.0 = The exact limit of grip (Peak load transfer)
    # >1.0 = Understeering / Sliding
    steering_angle = np.linspace(0, 2.5, 500)

    # Simulate the Raw Load Transfer (The "Bell Curve")
    # We use a standard physics approximation for a tire slip curve: x * e^(1-x)
    # This creates a curve that rises to 1.0 at x=1, then smoothly drops off.
    raw_load = steering_angle * np.exp(1 - steering_angle)

    # Apply the transformations to the raw load
    # 1. Unchanged (Linear)
    y_linear = raw_load

    # 2. Cubic Polynomial S-Curve
    y_cubic = 1.5 * raw_load - 0.5 * (raw_load ** 3)

    # 3. Sine Curve
    y_sine = np.sin(raw_load * np.pi / 2)

    # --- Plotting Setup ---
    plt.figure(figsize=(10, 8))

    # Plot the curves
    plt.plot(steering_angle, y_linear, label='Raw Formula (Sharp Peak / Notchy)', color='gray', linestyle='--', linewidth=2)
    plt.plot(steering_angle, y_cubic, label='Cubic Transformed (Smoother Peak)', color='dodgerblue', linewidth=2.5)
    plt.plot(steering_angle, y_sine, label='Sine Transformed', color='darkorange', linestyle='-.', linewidth=2.5)

    # Add grid, axes, and labels
    plt.axhline(0, color='black', linewidth=1)
    plt.grid(True, linestyle=':', alpha=0.7)

    plt.title('FFB Force vs. Steering Angle (The "Bell Curve")', fontsize=14, fontweight='bold')
    plt.xlabel('Steering Angle / Slip Angle (1.0 = Limit of Grip)', fontsize=12)
    plt.ylabel('Final FFB Output Force', fontsize=12)

    # Highlight the Peak / Notch area
    plt.axvspan(0.7, 1.4, color='red', alpha=0.1, label='The Limit (Transition to Understeer)')

    plt.legend(fontsize=11, loc='lower right')

    # Set axis limits
    plt.xlim(0, 2.5)
    plt.ylim(0, 1.05)

    # Display the plot
    plt.tight_layout()
    plt.show()

def plot_lateral_load_1():
    # Define the input range for normalized lateral load (-1.0 to 1.0)
    # We use 500 points to ensure the curves are perfectly smooth
    x = np.linspace(-1.0, 1.0, 500)

    # 1. Unchanged (Linear)
    # This is your current formula: f(x) = x
    y_linear = x

    # 2. Cubic Polynomial S-Curve
    # f(x) = 1.5x - 0.5x^3
    y_cubic = 1.5 * x - 0.5 * (x ** 3)

    # 3. Sine Curve
    # f(x) = sin(x * pi / 2)
    y_sine = np.sin(x * np.pi / 2)

    # --- Plotting Setup ---
    plt.figure(figsize=(10, 8))

    # Plot the three functions
    plt.plot(x, y_linear, label='Unchanged (Linear)', color='gray', linestyle='--', linewidth=2)
    plt.plot(x, y_cubic, label='Cubic Polynomial ($1.5x - 0.5x^3$)', color='dodgerblue', linewidth=2.5)
    plt.plot(x, y_sine, label='Sine Curve ($\sin(x \cdot \pi/2)$)', color='darkorange', linestyle='-.', linewidth=2.5)

    # Add grid, axes, and labels for readability
    plt.axhline(0, color='black', linewidth=1) # X-axis
    plt.axvline(0, color='black', linewidth=1) # Y-axis
    plt.grid(True, linestyle=':', alpha=0.7)

    plt.title('Lateral Load FFB Transformations', fontsize=14, fontweight='bold')
    plt.xlabel('Raw Normalized Load Transfer (Input)', fontsize=12)
    plt.ylabel('Transformed FFB Output', fontsize=12)

    # Highlight the "Peak" areas where the notchiness occurs
    plt.axvspan(0.8, 1.0, color='red', alpha=0.1, label='Peak Load Area (Notchy Zone)')
    plt.axvspan(-1.0, -0.8, color='red', alpha=0.1)

    plt.legend(fontsize=11, loc='upper left')

    # Set axis limits slightly beyond [-1, 1] to see the edges clearly
    plt.xlim(-1.1, 1.1)
    plt.ylim(-1.1, 1.1)

    # Display the plot
    plt.tight_layout()
    plt.show()

def plot_lateral_load_Cubic_vs_Quadratic():

    # 1. The previous Cubic formula (k=1.0)
    def cubic_smooth(x):
        return 1.5 * x - 0.5 * (x ** 3)

    # 2. The new Quadratic formula (k=1.0)
    def quadratic_smooth(x):
        return 2.0 * x - x * np.abs(x)

    fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(16, 6))

    # ==========================================
    # PLOT 1: The Transfer Function (Left)
    # ==========================================
    x_transfer = np.linspace(-1.0, 1.0, 500)

    ax1.plot(x_transfer, x_transfer, label='Linear (No Smoothing)', color='gray', linestyle='--', linewidth=2)
    ax1.plot(x_transfer, cubic_smooth(x_transfer), label='Cubic (Exponent 3)', color='dodgerblue', linewidth=2.5)
    ax1.plot(x_transfer, quadratic_smooth(x_transfer), label='Quadratic (Exponent 2)', color='darkviolet', linewidth=2.5)

    ax1.axhline(0, color='black', linewidth=1)
    ax1.axvline(0, color='black', linewidth=1)
    ax1.grid(True, linestyle=':', alpha=0.7)
    ax1.set_title('1. Transfer Function (Math)', fontsize=14, fontweight='bold')
    ax1.set_xlabel('Raw Normalized Load Transfer (Input)', fontsize=12)
    ax1.set_ylabel('Transformed FFB Output', fontsize=12)
    ax1.legend(fontsize=11, loc='upper left')
    ax1.set_xlim(-1.1, 1.1)
    ax1.set_ylim(-1.1, 1.1)

    # ==========================================
    # PLOT 2: The "Bell Curve" (Right)
    # ==========================================
    x_steer = np.linspace(0, 2.5, 500)
    raw_load = x_steer * np.exp(1 - x_steer) # Simulated tire slip curve

    ax2.plot(x_steer, raw_load, label='Linear (Sharp Peak)', color='gray', linestyle='--', linewidth=2)
    ax2.plot(x_steer, cubic_smooth(raw_load), label='Cubic (Smooth Peak)', color='dodgerblue', linewidth=2.5)
    ax2.plot(x_steer, quadratic_smooth(raw_load), label='Quadratic (Ultra-Broad Peak)', color='darkviolet', linewidth=2.5)

    ax2.axhline(0, color='black', linewidth=1)
    ax2.grid(True, linestyle=':', alpha=0.7)
    ax2.set_title('2. FFB Force vs. Steering Angle (Physics)', fontsize=14, fontweight='bold')
    ax2.set_xlabel('Steering Angle / Slip Angle (1.0 = Limit of Grip)', fontsize=12)
    ax2.set_ylabel('Final FFB Output Force', fontsize=12)
    ax2.legend(fontsize=11, loc='lower right')
    ax2.set_xlim(0, 2.5)
    ax2.set_ylim(0, 1.05)

    plt.tight_layout()
    plt.show()

def plot_lateral_load_2():

    # The Parametric Function
    def apply_rolloff(x, k):
        return (1.0 + 0.5 * k) * x - (0.5 * k) * (x ** 3)

    # Setup subplots
    fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(16, 6))

    # Slider values to test
    k_values =[0.0, 0.33, 0.66, 1.0, 1.5, 2.0]
    colors = ['gray', 'mediumseagreen', 'dodgerblue', 'darkviolet', 'purple', 'pink']
    labels =['k = 0.00 (Linear / Sharp)', 'k = 0.33 (Slight Smoothing)', 'k = 0.66 (Moderate Smoothing)', 'k = 1.00 (Full Cubic / Smooth)', 'k = 1.50 (Strong Smoothing)', 'k = 2.00 (Very Strong Smoothing)']

    # ==========================================
    # PLOT 1: The Transfer Function (Left)
    # ==========================================
    x_transfer = np.linspace(-1.0, 1.0, 500)

    for k, color, label in zip(k_values, colors, labels):
        y = apply_rolloff(x_transfer, k)
        linestyle = '--' if k == 0.0 else '-'
        ax1.plot(x_transfer, y, label=label, color=color, linestyle=linestyle, linewidth=2.5)

    ax1.axhline(0, color='black', linewidth=1)
    ax1.axvline(0, color='black', linewidth=1)
    ax1.grid(True, linestyle=':', alpha=0.7)
    ax1.set_title('1. Transfer Function (Math)', fontsize=14, fontweight='bold')
    ax1.set_xlabel('Raw Normalized Load Transfer (Input)', fontsize=12)
    ax1.set_ylabel('Transformed FFB Output', fontsize=12)
    ax1.legend(fontsize=11, loc='upper left')
    ax1.set_xlim(-1.1, 1.1)
    ax1.set_ylim(-1.1, 1.1)

    # ==========================================
    # PLOT 2: The "Bell Curve" (Right)
    # ==========================================
    x_steer = np.linspace(0, 2.5, 500)
    raw_load = x_steer * np.exp(1 - x_steer) # Simulated tire slip curve

    for k, color, label in zip(k_values, colors, labels):
        y = apply_rolloff(raw_load, k)
        linestyle = '--' if k == 0.0 else '-'
        ax2.plot(x_steer, y, label=label, color=color, linestyle=linestyle, linewidth=2.5)

    ax2.axhline(0, color='black', linewidth=1)
    ax2.grid(True, linestyle=':', alpha=0.7)
    ax2.set_title('2. FFB Force vs. Steering Angle (Physics)', fontsize=14, fontweight='bold')
    ax2.set_xlabel('Steering Angle / Slip Angle (1.0 = Limit of Grip)', fontsize=12)
    ax2.set_ylabel('Final FFB Output Force', fontsize=12)
    ax2.legend(fontsize=11, loc='lower right')
    ax2.set_xlim(0, 2.5)
    ax2.set_ylim(0, 1.05)

    plt.tight_layout()
    plt.show()


def plot_lateral_load_Cubic_Hermite_Spline():

    # The Cubic Hermite Spline Function
    def custom_curve(x, S0, S1):
        # S0 = Center Slope (Steepness at x=0)
        # S1 = Peak Slope (Steepness at x=1)
        
        # Calculate polynomial coefficients
        a = S0 + S1 - 2.0
        b = 3.0 - 2.0 * S0 - S1
        c = S0
        
        # f(x) = ax^3 + bx^2 + cx
        return a * (x**3) + b * (x**2) + c * x

    # Setup subplots
    fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(16, 6))

    # Scenarios to plot: (Center Slope, Peak Slope, Color, Label)
    scenarios =[
        (1.0, 1.0, 'gray', 'Linear (Standard)'),
        (0.5, 0.0, 'dodgerblue', 'Gentle Center + Broad/Flat Peak'),
        (0.5, 1.0, 'mediumseagreen', 'Gentle Center + Sharp/Notchy Peak'),
        (1.5, 0.0, 'darkviolet', 'Aggressive Center + Broad/Flat Peak')
    ]

    # ==========================================
    # PLOT 1: The Transfer Function (Left)
    # ==========================================
    x_transfer = np.linspace(0.0, 1.0, 500) # Only plotting positive side for clarity

    for S0, S1, color, label in scenarios:
        y = custom_curve(x_transfer, S0, S1)
        linestyle = '--' if S0==1.0 and S1==1.0 else '-'
        ax1.plot(x_transfer, y, label=f'{label}\n(S0={S0}, S1={S1})', color=color, linestyle=linestyle, linewidth=2.5)

    ax1.grid(True, linestyle=':', alpha=0.7)
    ax1.set_title('1. Transfer Function (Math)', fontsize=14, fontweight='bold')
    ax1.set_xlabel('Raw Normalized Load Transfer (Input)', fontsize=12)
    ax1.set_ylabel('Transformed FFB Output', fontsize=12)
    ax1.legend(fontsize=10, loc='upper left')
    ax1.set_xlim(0, 1.05)
    ax1.set_ylim(0, 1.05)

    # ==========================================
    # PLOT 2: The "Bell Curve" (Right)
    # ==========================================
    x_steer = np.linspace(0, 2.5, 500)
    raw_load = x_steer * np.exp(1 - x_steer) # Simulated tire slip curve

    for S0, S1, color, label in scenarios:
        y = custom_curve(raw_load, S0, S1)
        linestyle = '--' if S0==1.0 and S1==1.0 else '-'
        ax2.plot(x_steer, y, label=f'S0={S0}, S1={S1}', color=color, linestyle=linestyle, linewidth=2.5)

    ax2.grid(True, linestyle=':', alpha=0.7)
    ax2.set_title('2. FFB Force vs. Steering Angle (Physics)', fontsize=14, fontweight='bold')
    ax2.set_xlabel('Steering Angle / Slip Angle (1.0 = Limit of Grip)', fontsize=12)
    ax2.set_ylabel('Final FFB Output Force', fontsize=12)
    ax2.legend(fontsize=11, loc='lower right')
    ax2.set_xlim(0, 2.5)
    ax2.set_ylim(0, 1.05)

    plt.tight_layout()
    plt.show()

def plot_lateral_load_Generalized_Power_Spline():

    # The Generalized Power Spline Function
    def generalized_spline(x, S0, S1, E):
        # Calculate dynamic coefficients
        B = E + 1.0 - (E * S0) - S1
        C = S1 - E + (E - 1.0) * S0
        
        # f(x) = S0*x + B*x^E + C*x^(E+1)
        return S0 * x + B * (x ** E) + C * (x ** (E + 1.0))

    # Setup subplots
    fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(16, 6))

    # We will lock the slopes to show ONLY the effect of the Exponent (E)
    # S0 = 1.0 (Linear center), S1 = 0.0 (Perfectly flat peak)
    S0_fixed = 1.0
    S1_fixed = 0.0

    # Scenarios to plot: (Exponent E, Color, Label)
    scenarios =[
        (3.0, 'gray', 'E = 3.0 (Narrow / Late turn)'),
        (2.0, 'dodgerblue', 'E = 2.0 (Standard Cubic Spline)'),
        (1.5, 'mediumseagreen', 'E = 1.5 (Broader than Quadratic)'),
        (1.1, 'darkviolet', 'E = 1.1 (Ultra-Broad / Very Gentle)')
    ]

    # ==========================================
    # PLOT 1: The Transfer Function (Left)
    # ==========================================
    x_transfer = np.linspace(0.0, 1.0, 500)

    for E, color, label in scenarios:
        y = generalized_spline(x_transfer, S0_fixed, S1_fixed, E)
        ax1.plot(x_transfer, y, label=label, color=color, linewidth=2.5)

    ax1.grid(True, linestyle=':', alpha=0.7)
    ax1.set_title(f'1. Transfer Function (S0={S0_fixed}, S1={S1_fixed})', fontsize=14, fontweight='bold')
    ax1.set_xlabel('Raw Normalized Load Transfer (Input)', fontsize=12)
    ax1.set_ylabel('Transformed FFB Output', fontsize=12)
    ax1.legend(fontsize=11, loc='lower right')
    ax1.set_xlim(0, 1.05)
    ax1.set_ylim(0, 1.05)

    # ==========================================
    # PLOT 2: The "Bell Curve" (Right)
    # ==========================================
    x_steer = np.linspace(0, 2.5, 500)
    raw_load = x_steer * np.exp(1 - x_steer) # Simulated tire slip curve

    for E, color, label in scenarios:
        y = generalized_spline(raw_load, S0_fixed, S1_fixed, E)
        ax2.plot(x_steer, y, label=label, color=color, linewidth=2.5)

    ax2.grid(True, linestyle=':', alpha=0.7)
    ax2.set_title('2. FFB Force vs. Steering Angle (Physics)', fontsize=14, fontweight='bold')
    ax2.set_xlabel('Steering Angle / Slip Angle (1.0 = Limit of Grip)', fontsize=12)
    ax2.set_ylabel('Final FFB Output Force', fontsize=12)
    ax2.legend(fontsize=11, loc='lower right')
    ax2.set_xlim(0, 2.5)
    ax2.set_ylim(0, 1.05)

    plt.tight_layout()
    plt.show()


def plot_interactive_generalized_spline():
    
    # ==========================================
    # 1. The Math Function
    # ==========================================
    def generalized_spline(x, S0, S1, E):
        """
        Calculates the Generalized Power Spline.
        S0 = Center Steepness
        S1 = Peak Notchiness (Slope at x=1)
        E  = Broadness (Exponent)
        """
        # Calculate dynamic coefficients
        B = E + 1.0 - (E * S0) - S1
        C = S1 - E + (E - 1.0) * S0
        
        # f(x) = S0*x + B*x^E + C*x^(E+1)
        y = S0 * x + B * (x ** E) + C * (x ** (E + 1.0))
        
        # Simulate the C++ std::clamp to prevent mathematical overshoots
        return np.clip(y, 0.0, 1.0)

    # ==========================================
    # 2. Setup Data & Figure
    # ==========================================
    # X-axis data
    x_trans = np.linspace(0.0, 1.0, 500)                     # For Transfer Function
    x_steer = np.linspace(0.0, 2.5, 500)                     # For Bell Curve
    raw_load = x_steer * np.exp(1 - x_steer)                 # Simulated tire slip

    # Create figure and leave space at the bottom for sliders
    fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(16, 7))
    plt.subplots_adjust(bottom=0.35) # Make room for sliders

    # ==========================================
    # 3. Plot Fixed Reference Curves
    # ==========================================
    # Ref 1: Linear (No smoothing)
    ax1.plot(x_trans, x_trans, color='gray', linestyle='--', alpha=0.6, label='Ref: Linear (S0=1, S1=1)')
    ax2.plot(x_steer, raw_load, color='gray', linestyle='--', alpha=0.6, label='Ref: Linear (Sharp Peak)')

    # Ref 2: Standard Cubic (The original 1.5x - 0.5x^3)
    y_cub_t = generalized_spline(x_trans, S0=1.5, S1=0.0, E=2.0)
    y_cub_s = generalized_spline(raw_load, S0=1.5, S1=0.0, E=2.0)
    ax1.plot(x_trans, y_cub_t, color='dodgerblue', linestyle=':', lw=2, alpha=0.7, label='Ref: Standard Cubic (S0=1.5, S1=0, E=2)')
    ax2.plot(x_steer, y_cub_s, color='dodgerblue', linestyle=':', lw=2, alpha=0.7, label='Ref: Standard Cubic')

    # Ref 3: Ultra-Broad (Very gentle peak)
    y_brd_t = generalized_spline(x_trans, S0=1.0, S1=0.0, E=1.1)
    y_brd_s = generalized_spline(raw_load, S0=1.0, S1=0.0, E=1.1)
    ax1.plot(x_trans, y_brd_t, color='mediumseagreen', linestyle='-.', lw=2, alpha=0.7, label='Ref: Ultra-Broad (S0=1, S1=0, E=1.1)')
    ax2.plot(x_steer, y_brd_s, color='mediumseagreen', linestyle='-.', lw=2, alpha=0.7, label='Ref: Ultra-Broad')

    # ==========================================
    # 4. Plot the Interactive Line
    # ==========================================
    # Initial values
    init_S0 = 1.0
    init_S1 = 0.0
    init_E  = 2.0

    # Draw the initial interactive lines
    line_trans, = ax1.plot(x_trans, generalized_spline(x_trans, init_S0, init_S1, init_E), 
                        color='red', lw=3.5, label='Interactive Custom Curve')
    line_steer, = ax2.plot(x_steer, generalized_spline(raw_load, init_S0, init_S1, init_E), 
                        color='red', lw=3.5, label='Interactive Custom Curve')

    # Formatting Plot 1
    ax1.set_title('1. Transfer Function (Math)', fontsize=14, fontweight='bold')
    ax1.set_xlabel('Raw Normalized Load Transfer (Input)', fontsize=12)
    ax1.set_ylabel('Transformed FFB Output', fontsize=12)
    ax1.grid(True, linestyle=':', alpha=0.7)
    ax1.legend(fontsize=10, loc='lower right')
    ax1.set_xlim(0, 1.05)
    ax1.set_ylim(0, 1.05)

    # Formatting Plot 2
    ax2.set_title('2. FFB Force vs. Steering Angle (Physics)', fontsize=14, fontweight='bold')
    ax2.set_xlabel('Steering Angle / Slip Angle (1.0 = Limit of Grip)', fontsize=12)
    ax2.set_ylabel('Final FFB Output Force', fontsize=12)
    ax2.grid(True, linestyle=':', alpha=0.7)
    ax2.legend(fontsize=10, loc='lower right')
    ax2.set_xlim(0, 2.5)
    ax2.set_ylim(0, 1.05)

    # ==========================================
    # 5. Setup Sliders
    # ==========================================
    # Define axes for sliders[left, bottom, width, height]
    ax_S0 = plt.axes([0.15, 0.20, 0.7, 0.03], facecolor='lightgoldenrodyellow')
    ax_S1 = plt.axes([0.15, 0.13, 0.7, 0.03], facecolor='lightgoldenrodyellow')
    ax_E  = plt.axes([0.15, 0.06, 0.7, 0.03], facecolor='lightgoldenrodyellow')

    # Create sliders
    slider_S0 = Slider(ax_S0, 'S0 (Center Steepness)', 0.1, 3.0, valinit=init_S0, valstep=0.05)
    slider_S1 = Slider(ax_S1, 'S1 (Peak Notchiness)', 0.0, 1.0, valinit=init_S1, valstep=0.05)
    slider_E  = Slider(ax_E,  'E (Peak Broadness)', 1.01, 4.0, valinit=init_E, valstep=0.05)

    # Update function called when sliders move
    def update(val):
        S0 = slider_S0.val
        S1 = slider_S1.val
        E  = slider_E.val
        
        # Update Y data for both plots
        line_trans.set_ydata(generalized_spline(x_trans, S0, S1, E))
        line_steer.set_ydata(generalized_spline(raw_load, S0, S1, E))
        
        # Redraw the canvas
        fig.canvas.draw_idle()

    # Attach the update function to the sliders
    slider_S0.on_changed(update)
    slider_S1.on_changed(update)
    slider_E.on_changed(update)

    # ==========================================
    # 6. Show Plot
    # ==========================================
    plt.show()


        
def plot_interactive_locked_center_spline():

    # The proposed Locked-Center Hermite Spline formula
    def locked_center_spline(x, k):
        """
        x: Raw lateral load normalized [-1.0, 1.0]
        k: Rolloff parameter [0.0, 1.0]
        0.0 = Pure linear (sharp peak)
        1.0 = Maximum smoothing (flat peak, zero slope at extremes)
        """
        abs_x = np.abs(x)
        return x * (1.0 + k * abs_x - k * (abs_x * abs_x))

    # Generate x values from -1.0 to 1.0
    x = np.linspace(-1.0, 1.0, 500)

    # Initial parameter value
    k_init = 1.0

    # Create the figure and the line that we will manipulate
    fig, ax = plt.subplots(figsize=(9, 7))
    plt.subplots_adjust(bottom=0.25) # Make room for the slider

    # Plot a reference line (pure linear, k=0)
    ax.plot(x, x, 'r--', alpha=0.5, label='Linear Reference (k=0.0)')

    # Plot the initial transformed curve
    y = locked_center_spline(x, k_init)
    line, = ax.plot(x, y, 'b-', linewidth=2.5, label=f'Locked-Center Spline (k={k_init:.2f})')

    # Formatting the plot
    ax.set_xlim(-1.1, 1.1)
    ax.set_ylim(-1.1, 1.1)
    ax.set_title('Lateral Load Transformation: Locked-Center Hermite Spline', fontsize=14)
    ax.set_xlabel('Raw Lateral Load Input (x)', fontsize=12)
    ax.set_ylabel('Transformed FFB Output (y)', fontsize=12)
    ax.axhline(0, color='black', linewidth=1)
    ax.axvline(0, color='black', linewidth=1)
    ax.grid(True, linestyle=':', alpha=0.7)
    ax.legend(loc='upper left')

    # Define the slider axis and create the slider
    ax_k = plt.axes([0.15, 0.1, 0.7, 0.03])
    slider_k = Slider(
        ax=ax_k,
        label='Rolloff (k)',
        valmin=0.0,
        valmax=1.0,
        valinit=k_init,
        valstep=0.01,
        color='#0099ff'
    )

    # The function to be called anytime a slider's value changes
    def update(val):
        k = slider_k.val
        # Update the y-data of the plot
        line.set_ydata(locked_center_spline(x, k))
        # Update the legend to reflect the new k value
        line.set_label(f'Locked-Center Spline (k={k:.2f})')
        ax.legend(loc='upper left')
        # Redraw the canvas
        fig.canvas.draw_idle()

    # Register the update function with the slider
    slider_k.on_changed(update)

    plt.show()

import numpy as np
import matplotlib.pyplot as plt
from matplotlib.widgets import Slider

def plot_interactive_locked_center_spline2():
    
    # ==========================================
    # 1. The Math Function
    # ==========================================
    def locked_center_spline(x, k):
        """
        Calculates the Locked-Center Hermite Spline.
        k = Rolloff parameter (0.0 = Linear/Sharp, 1.0 = Smooth/Flat Peak)
        """
        abs_x = np.abs(x)
        # f(x) = x * (1 + k*|x| - k*x^2)
        y = x * (1.0 + k * abs_x - k * (abs_x * abs_x))
        
        # Simulate the C++ std::clamp to prevent mathematical overshoots
        return np.clip(y, -1.0, 1.0)

    # ==========================================
    # 2. Setup Data & Figure
    # ==========================================
    # X-axis data
    x_trans = np.linspace(0.0, 1.0, 500)                     # For Transfer Function
    x_steer = np.linspace(0.0, 2.5, 500)                     # For Bell Curve
    raw_load = x_steer * np.exp(1 - x_steer)                 # Simulated tire slip

    # Create figure and leave space at the bottom for the slider
    fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(16, 7))
    plt.subplots_adjust(bottom=0.25) # Make room for slider

    # ==========================================
    # 3. Plot Fixed Reference Curves
    # ==========================================
    # Ref 1: Linear (No smoothing, k=0.0)
    ax1.plot(x_trans, x_trans, color='gray', linestyle='--', alpha=0.6, label='Ref: Linear (k=0.0)')
    ax2.plot(x_steer, raw_load, color='gray', linestyle='--', alpha=0.6, label='Ref: Linear (Sharp Peak)')

    # Ref 2: Fully Smoothed (k=1.0)
    y_smooth_t = locked_center_spline(x_trans, k=1.0)
    y_smooth_s = locked_center_spline(raw_load, k=1.0)
    ax1.plot(x_trans, y_smooth_t, color='dodgerblue', linestyle=':', lw=2, alpha=0.7, label='Ref: Fully Smoothed (k=1.0)')
    ax2.plot(x_steer, y_smooth_s, color='dodgerblue', linestyle=':', lw=2, alpha=0.7, label='Ref: Fully Smoothed')

    # ==========================================
    # 4. Plot the Interactive Line
    # ==========================================
    # Initial value
    init_k = 0.5

    # Draw the initial interactive lines
    line_trans, = ax1.plot(x_trans, locked_center_spline(x_trans, init_k), 
                        color='red', lw=3.5, label=f'Interactive Curve (k={init_k:.2f})')
    line_steer, = ax2.plot(x_steer, locked_center_spline(raw_load, init_k), 
                        color='red', lw=3.5, label=f'Interactive Curve (k={init_k:.2f})')

    # Formatting Plot 1
    ax1.set_title('1. Transfer Function (Math)', fontsize=14, fontweight='bold')
    ax1.set_xlabel('Raw Normalized Load Transfer (Input)', fontsize=12)
    ax1.set_ylabel('Transformed FFB Output', fontsize=12)
    ax1.grid(True, linestyle=':', alpha=0.7)
    ax1.legend(fontsize=10, loc='lower right')
    ax1.set_xlim(0, 1.05)
    ax1.set_ylim(0, 1.05)

    # Formatting Plot 2
    ax2.set_title('2. FFB Force vs. Steering Angle (Physics)', fontsize=14, fontweight='bold')
    ax2.set_xlabel('Steering Angle / Slip Angle (1.0 = Limit of Grip)', fontsize=12)
    ax2.set_ylabel('Final FFB Output Force', fontsize=12)
    ax2.grid(True, linestyle=':', alpha=0.7)
    ax2.legend(fontsize=10, loc='lower right')
    ax2.set_xlim(0, 2.5)
    ax2.set_ylim(0, 1.05)

    # ==========================================
    # 5. Setup Slider
    # ==========================================
    # Define axis for slider[left, bottom, width, height]
    ax_k = plt.axes([0.15, 0.10, 0.7, 0.03], facecolor='lightgoldenrodyellow')

    # Create slider
    slider_k = Slider(ax_k, 'k (Rolloff)', 0.0, 1.0, valinit=init_k, valstep=0.01)

    # Update function called when slider moves
    def update(val):
        k = slider_k.val
        
        # Update Y data for both plots
        line_trans.set_ydata(locked_center_spline(x_trans, k))
        line_steer.set_ydata(locked_center_spline(raw_load, k))
        
        # Update labels to show current k value
        line_trans.set_label(f'Interactive Curve (k={k:.2f})')
        line_steer.set_label(f'Interactive Curve (k={k:.2f})')
        ax1.legend(fontsize=10, loc='lower right')
        ax2.legend(fontsize=10, loc='lower right')
        
        # Redraw the canvas
        fig.canvas.draw_idle()

    # Attach the update function to the slider
    slider_k.on_changed(update)

    # ==========================================
    # 6. Show Plot
    # ==========================================
    plt.show()


def main():
    plot_interactive_locked_center_spline2()

if __name__ == "__main__":
    main()
