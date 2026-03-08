
import numpy as np
import matplotlib.pyplot as plt


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

def main():
    plot_lateral_load_Cubic_Hermite_Spline()

if __name__ == "__main__":
    main()
