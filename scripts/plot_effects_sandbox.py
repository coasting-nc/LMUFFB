
import numpy as np
import matplotlib.pyplot as plt


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

def plot_lateral_load():
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


def main():
    plot_lateral_load_bellcurve()

if __name__ == "__main__":
    main()
