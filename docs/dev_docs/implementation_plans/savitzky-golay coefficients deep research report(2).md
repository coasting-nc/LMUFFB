Advanced Signal Processing Architectures: Definitive Implementation of Savitzky-Golay Filters for Quadratic Differentiation with Adaptive Windowing1. Introduction and Historical ContextThe rigorous analysis of time-domain signals often necessitates the extraction of rate-of-change information—derivatives—from data streams corrupted by stochastic noise. In the landscape of digital signal processing (DSP) and chemometrics, the Savitzky-Golay (SG) filter stands as a preeminent algorithm, revered for its unique ability to smooth data and calculate derivatives while preserving the higher-order spectral moments of the underlying signal features, such as peak width and height. First introduced in the seminal 1964 Analytical Chemistry paper by Abraham Savitzky and Marcel J. E. Golay, the method fundamentally shifted the paradigm of data smoothing from simple moving averages to local least-squares polynomial fitting.The user's requirement to implement a Quadratic Fit ($N=2$) for the First Derivative with Variable Window Sizes touches upon the most sophisticated aspects of this algorithm. While the original formulation relied on static lookup tables for fixed window sizes, modern real-time applications demand dynamic adaptability. This report provides an exhaustive, expert-level deconstruction of the SG filter, moving from first-principles mathematical derivation to high-performance C++ implementation strategies. It addresses the subtle but critical mathematical equivalence between linear and quadratic fits on symmetric domains, the breakdown of this equivalence at signal boundaries, and the algorithmic imperative for dynamic coefficient generation over static storage.The necessity for such a filter arises because standard finite difference methods, such as the two-point central difference operator, act as high-pass filters. In the frequency domain, differentiation corresponds to multiplication by $j\omega$; consequently, high-frequency noise is amplified linearly with frequency, often drowning out the signal of interest in the derivative domain. The Savitzky-Golay filter effectively combines a low-pass smoothing filter with a differentiation kernel, optimizing the trade-off between noise suppression (variance reduction) and signal fidelity (bias minimization) through the least-squares criterion.2. Mathematical Derivation of the Least-Squares ConvolutionTo implement a robust feature for variable window sizes, one cannot rely on the pre-computed tables found in the original 1964 paper—which, notably, contained several numerical errors corrected in subsequent literature by Gorry and others. Instead, the implementation must utilize the generative mathematical model.2.1 The General Linear ModelThe core premise of the Savitzky-Golay filter is that valid signal trends within a narrow temporal window can be approximated by a polynomial of degree $N$. Let us consider a subset of $2M+1$ data points, denoted as the vector $\mathbf{x}$, centered at a time index $n=0$. The local index $k$ ranges from $-M$ to $M$. We aim to fit a polynomial $P(k)$ of degree $N$ to these points:$$P(k) = \sum_{j=0}^{N} a_j k^j$$The coefficients $a_j$ are determined by minimizing the sum of squared errors $\chi^2$ between the polynomial model and the observed data $x[k]$:$$\chi^2 = \sum_{k=-M}^{M} \left( \sum_{j=0}^{N} a_j k^j - x[k] \right)^2$$This minimization problem can be expressed in matrix notation. Let $\mathbf{J}$ be the Vandermonde design matrix of size $(2M+1) \times (N+1)$, where the element $J_{k,j} = k^j$. The system of linear equations to solve is:$$\mathbf{x} = \mathbf{J} \mathbf{a} + \mathbf{\epsilon}$$where $\mathbf{a} = [a_0, a_1, \dots, a_N]^T$ is the vector of polynomial coefficients. The optimal solution $\hat{\mathbf{a}}$ is found via the normal equations:$$\hat{\mathbf{a}} = (\mathbf{J}^T \mathbf{J})^{-1} \mathbf{J}^T \mathbf{x}$$The matrix $\mathbf{C} = (\mathbf{J}^T \mathbf{J})^{-1} \mathbf{J}^T$ is the projection matrix. The crucial insight of Savitzky and Golay was that because the indices $k$ are fixed relative to the window center (e.g., always $-2, -1, 0, 1, 2$ for $M=2$), the matrix $\mathbf{C}$ is constant and depends only on $M$ and $N$, not on the data $\mathbf{x}$.2.2 Deriving the First DerivativeThe polynomial $P(k)$ models the signal structure within the window.The smoothed value at the center ($k=0$) is $P(0) = a_0$.The first derivative at the center is evaluated analytically from the polynomial:$$\frac{d P(k)}{dk} \bigg|_{k=0} = \frac{d}{dk} (a_0 + a_1 k + a_2 k^2 + \dots) \bigg|_{k=0} = a_1$$The second derivative is $2a_2$, and so forth.Therefore, obtaining the smoothed first derivative reduces to calculating the coefficient $a_1$. From the matrix solution $\hat{\mathbf{a}} = \mathbf{C} \mathbf{x}$, the value of $a_1$ is simply the dot product of the second row of $\mathbf{C}$ (index 1) with the data vector $\mathbf{x}$.$$a_1 = \sum_{k=-M}^{M} C_{1,k} x[k]$$This linear combination forms the Finite Impulse Response (FIR) filter kernel for the derivative.3. The Quadratic-Linear Equivalence on Symmetric WindowsA critical theoretical nuance—often overlooked in general implementations but vital for understanding the "Quadratic" requirement—is the equivalence between Linear ($N=1$) and Quadratic ($N=2$) fits for determining the first derivative on symmetric windows.3.1 Orthogonality of Basis FunctionsThe matrix $\mathbf{J}^T \mathbf{J}$ consists of the moments of the index $k$. Let $S_p = \sum_{k=-M}^{M} k^p$.For a symmetric window $[-M, M]$, the sum of any odd power of $k$ is zero.$$S_{odd} = 0$$For a Linear Fit ($N=1$), the normal matrix $\mathbf{A}_{lin} = \mathbf{J}^T \mathbf{J}$ is:$$\mathbf{A}_{lin} = \begin{bmatrix} S_0 & S_1 \\ S_1 & S_2 \end{bmatrix} = \begin{bmatrix} 2M+1 & 0 \\ 0 & S_2 \end{bmatrix}$$This matrix is diagonal. The solution for $a_1$ is completely decoupled from $a_0$:$$a_1^{(lin)} = \frac{1}{S_2} \sum_{k=-M}^{M} k \cdot x[k]$$For a Quadratic Fit ($N=2$), the normal matrix $\mathbf{A}_{quad}$ expands to $3 \times 3$:$$\mathbf{A}_{quad} = \begin{bmatrix} S_0 & S_1 & S_2 \\ S_1 & S_2 & S_3 \\ S_2 & S_3 & S_4 \end{bmatrix} = \begin{bmatrix} S_0 & 0 & S_2 \\ 0 & S_2 & 0 \\ S_2 & 0 & S_4 \end{bmatrix}$$
Observe the "checkerboard" pattern of zeros. The equation for $a_1$ (the middle row) is effectively isolated from $a_0$ and $a_2$ because the off-diagonal terms $S_1$ and $S_3$ are zero. The inverse of this block-diagonal matrix retains the same structure, and the element corresponding to $a_1$ remains $1/S_2$.Consequently:$$a_1^{(quad)} = \frac{1}{S_2} \sum_{k=-M}^{M} k \cdot x[k]$$3.2 Implications for ImplementationThis mathematical identity leads to a profound simplification for the implementation. As long as the window is symmetric (i.e., not at the edges of the signal), the "Quadratic First Derivative" filter is computationally identical to the "Linear First Derivative" filter. The quadratic term $a_2$ captures the curvature (second derivative) and improves the estimate of the smoothed value $a_0$, but it does not alter the slope $a_1$ at the center of symmetry.Requirement Analysis: The user asked for a Quadratic Fit. The implementation must honor this. While the coefficients are identical to the linear case for the steady-state signal, the distinction becomes relevant if the user later requests the second derivative (which requires $N=2$) or processes the boundaries where symmetry is lost (discussed in Section 4).3.3 Comparison of Polynomial Orders for DerivativesIt is instructive to compare why one might choose $N=2$ over $N=3$ or $N=4$.Polynomial Order (N)Basis FunctionsFirst Derivative Character (a1​)NotesLinear ($N=1$)$1, k$Slope of regression line.Robust, high bias for curved signals.Quadratic ($N=2$)$1, k, k^2$Identical to Linear ($a_1^{(2)} = a_1^{(1)}$).Captures curvature ($a_2$) but slope at center is unchanged.Cubic ($N=3$)$1, k, k^2, k^3$Different. Slope accounts for inflection.Reduced bias, increased variance (noise sensitivity).Quartic ($N=4$)$1, \dots, k^4$Identical to Cubic ($a_1^{(4)} = a_1^{(3)}$).Adds $a_4$ term, center slope unchanged from Cubic.This table clarifies that for derivative estimation, polynomial orders pair up: $(1,2)$, $(3,4)$, $(5,6)$. The user's choice of Quadratic puts them in the lowest-variance (most stable) pair.4. The Universal Generating Function for Variable WindowsTo fulfill the requirement of Variable Window Sizes, the implementation cannot use hardcoded arrays. We must derive a function $f(M)$ that returns the convolution kernel.From the derivation in Section 3.1, the weight $w_k$ for the input $x[k]$ is:$$w_k = \frac{k}{S_2}$$The denominator $S_2$ is the sum of squares of integers from $-M$ to $M$.$$S_2 = \sum_{k=-M}^{M} k^2 = 2 \sum_{k=1}^{M} k^2 = 2 \frac{M(M+1)(2M+1)}{6} = \frac{M(M+1)(2M+1)}{3}$$Substituting this back into the expression for $w_k$:$$w_k = \frac{3k}{M(M+1)(2M+1)}$$This equation allows for the $O(1)$ generation of filter coefficients for any window half-width $M$. This is the definitive "definition" requested by the user.Physical Unit Scaling:The coefficient $a_1$ represents the change in signal amplitude per sample. To convert this to a physical rate of change (e.g., Volts/second), the result must be divided by the sampling interval $h$ (or $\Delta t$):$$\frac{dy}{dt} \approx \frac{1}{h} \sum_{k=-M}^{M} w_k x[n+k]$$
Failure to apply this $1/h$ scaling is a common implementation error noted in signal processing forums.5. Variable Window Architectures and Boundary HandlingThe term "Variable Window Sizes" in the user query implies two distinct engineering challenges that must be addressed: Adaptive Filtering (changing $M$ in response to signal statistics) and Boundary Handling (the edge cases where a full window does not fit).5.1 Adaptive Windowing based on Signal StatisticsIn a sophisticated implementation, $M$ is not just a user setting but a dynamic parameter.Stationary/Noisy Regions: If the local variance of the signal is high but the trend is flat, a larger $M$ is desirable to suppress noise.Transient/Fast Regions: If the signal exhibits rapid acceleration (high second derivative), a smaller $M$ is required to minimize the smoothing bias (distortion) of the peak.Implementation: The code structure should allow the half_width parameter to be updated per sample. Because the coefficient generation formula is computationally trivial, recalculating the kernel at every step (or caching a set of kernels for $M=2 \dots 20$) is feasible.5.2 The Boundary Problem: Asymmetry and InstabilityThe most critical aspect of "variable windows" is the start and end of the data stream. At index $n < M$, a full symmetric window extends into negative indices.Standard approaches include:Zero Padding: Assumes data is zero outside. Disastrous for derivatives, creating massive artificial spikes at the start.Mirror Padding: Reflects data. Better, but creates an artificial stationary point ($y'=0$) at the edge.Variable Asymmetric Window: Using only the available data points.The Asymmetric Quadratic Pitfall:If we fit a Quadratic polynomial ($N=2$) to an asymmetric window (e.g., indices $[0, 2M]$) to calculate the derivative at index 0, the equivalence derived in Section 3 breaks down.The matrix $\mathbf{J}^T \mathbf{J}$ is no longer block-diagonal. $S_{odd} \neq 0$.The slope $a_1$ becomes coupled to the curvature $a_2$.Runge's Phenomenon: At the edges of an interval, polynomial fits tend to oscillate ("flare") to minimize the least-squares error. A quadratic fit at the very first point of a noisy signal is highly unstable and will amplify noise significantly more than a linear fit.Recommended Strategy: Shrinking Symmetric WindowTo maintain the stability of the symmetric filters, the recommended approach for the boundaries is to dynamically shrink the window size $M$ to fit the available margin.At index $n$ (where $n < M_{target}$), set the effective window size $M_{eff} = n$.This results in a symmetric window $[-n, n]$ centered at $n$.Benefits: Maintains the Linear-Quadratic equivalence. Guarantees zero phase shift. Avoids the edge flare of asymmetric fitting.Trade-off: Reduced noise suppression at the very edges (since $N_{points}$ is smaller).5.3 Comparison of Boundary Handling StrategiesThe following table summarizes the behavior of different strategies for the start of the signal.StrategyPolynomial OrderStabilityDerivative QualityNotesAsymmetric FitQuadratic ($N=2$)LowVolatileProne to "flaring" (Runge-like). Physically fits a parabola to the start, often resulting in exaggerated initial slopes.Asymmetric FitLinear ($N=1$)MediumModerateFits a regression line to the first few points. More stable than quadratic but biased.Shrinking SymmetricQuad/Lin ($N=2/1$)HighStableReduces window size to stay centered. Example: At index 2, use $M=2$ (5 points). Preserves phase linearity.Forward DifferenceLinear ($N=1$)LowNoisyEquivalent to $M=1$ asymmetric. High noise amplification.For a robust "Quadratic First Derivative" implementation, the Shrinking Symmetric strategy is the expert recommendation.6. Implementation Strategies and Algorithms6.1 Advanced Algorithm: Gram PolynomialsWhile the closed-form solution derived in Section 4 is sufficient for $N=2$, a generalized implementation for "variable window sizes" that might support higher orders in the future should consider Gram Polynomials. This recursive method allows for the calculation of SG coefficients for any $N$ and $M$ without explicit matrix inversion, providing a numerically stable route for high-order polynomials.The recursion for the polynomial values $P_j(k)$ involves a three-term recurrence relation:$$P_{j+1}(k) = (k - \alpha_j) P_j(k) - \beta_j P_{j-1}(k)$$However, for the specific constraint of $N=2$, the closed-form approach remains superior in performance ($O(1)$ vs recursive $O(N)$). The Gram method is noted here as the rigorous path for extending the system to Cubic or Quartic fits.6.2 C++ Implementation ArchitectureThe provided C++ implementation follows a decoupled design. The CoeffGenerator is separated from the Filter to allow for the window_size to change independently of the data processing loop.6.2.1 Coefficient Generation (The "Definition")C++#include <vector>
#include <cmath>
#include <stdexcept>
#include <iostream>

/**
 * @class SGDerivKernel
 * @brief Implements the closed-form generation of Savitzky-Golay coefficients.
 *        Specialized for: Quadratic Fit (N=2), First Derivative.
 */
class SGDerivKernel {
public:
    /**
     * @brief Generates the convolution kernel.
     * @param half_width (M) The number of points on one side of the center.
     *                   Total Window Size = 2*M + 1.
     * @param h The sampling interval (delta t).
     * @return std::vector<double> containing the weights.
     */
    static std::vector<double> Create(int half_width, double h) {
        if (half_width < 1) {
            // Minimal window for derivative is 3 points (M=1)
            throw std::invalid_argument("Savitzky-Golay: Half-width must be >= 1");
        }
        if (h <= 0.0) {
            throw std::invalid_argument("Savitzky-Golay: Sampling interval must be positive");
        }

        // 1. Calculate the denominator S2 = Sum(k^2)
        // Formula: S2 = M(M+1)(2M+1)/3
        double M = static_cast<double>(half_width);
        double term = M * (M + 1.0) * (2.0 * M + 1.0);
        double S2 = term / 3.0;

        // 2. Allocate Kernel
        size_t window_size = 2 * half_width + 1;
        std::vector<double> kernel(window_size);

        // 3. Populate Weights
        // Formula: w_k = k / (S2 * h)
        // The weight vector is anti-symmetric: w_-k = -w_k
        for (int i = 0; i < window_size; ++i) {
            int k = i - half_width; // Local index ranges from -M to M
            kernel[i] = static_cast<double>(k) / (S2 * h);
        }

        return kernel;
    }
};
6.2.2 The Filter Processing Logic (The "Implementation")C++/**
 * @class AdaptiveSGFilter
 * @brief Manages the data buffer and applies the filter with variable window support.
 */
class AdaptiveSGFilter {
private:
    std::vector<double> buffer;
    double h;            // Sampling interval
    int target_M;        // Desired half-width for steady state

public:
    AdaptiveSGFilter(int M, double sampling_interval) 
        : target_M(M), h(sampling_interval) {}

    void Push(double value) {
        buffer.push_back(value);
    }

    /**
     * @brief Computes the derivative at a specific index using the
     *        Shrinking Symmetric Window strategy for boundaries.
     */
    double GetDerivativeAt(size_t index) {
        if (index >= buffer.size()) return 0.0;

        // 1. Determine available symmetric context
        int available_left = static_cast<int>(index);
        int available_right = static_cast<int>(buffer.size() - 1 - index);
        
        // 2. Variable Window Logic:
        // The effective M is limited by the distance to the nearest edge
        // or the user's target M.
        int effective_M = std::min(target_M, std::min(available_left, available_right));

        // 3. Edge Case: Not enough points for even a 3-point fit
        if (effective_M < 1) {
            return 0.0; // Or implement 2-point forward/backward difference here
        }

        // 4. Generate (or retrieve cached) coefficients
        // In production, cache these for M=1..target_M to avoid allocation
        std::vector<double> kernel = SGDerivKernel::Create(effective_M, h);

        // 5. Convolution
        double sum = 0.0;
        for (int k = -effective_M; k <= effective_M; ++k) {
            int data_idx = static_cast<int>(index) + k;
            // Kernel index centers at M
            sum += kernel[k + effective_M] * buffer[data_idx];
        }

        return sum;
    }
};
7. Performance Analysis and Frequency Domain Characteristics7.1 Frequency ResponseThe Savitzky-Golay differentiator is functionally a Low-Pass Differentiator.Ideal Differentiator: The magnitude response is $|\omega|$. It amplifies high frequencies infinitely.SG Differentiator: Follows the ideal $|\omega|$ slope in the low-frequency Passband but rolls off in the Stopband.The "Cutoff Frequency" ($f_c$) of the filter is determined by the window size $M$.$$f_c \propto \frac{1}{2M+1}$$As the window size increases, the cutoff frequency decreases, smoothing the signal more aggressively.7.2 Stopband LeakageA critical limitation of the SG filter compared to Gaussian derivatives is its stopband performance. The frequency response of the polynomial fit is not monotonic in the stopband; it exhibits sidelobes (ripples). This means that certain bands of high-frequency noise may pass through the filter with less attenuation than expected.Mitigation: If the signal contains narrowband high-frequency noise that aligns with an SG sidelobe, the window size $M$ should be adjusted to shift the nulls of the filter to cancel that frequency. This is a unique advantage of the "variable window" architecture—it allows for spectral tuning.7.3 Computational ComplexityThe optimized algorithm presented here operates with $O(M)$ complexity per sample.Symmetry Optimization: Since the derivative kernel is anti-symmetric ($c_{-k} = -c_k$), the convolution can be further optimized:$$y'[n] = \sum_{k=1}^{M} c_k (x[n+k] - x[n-k])$$
This reduces the number of multiplications by approximately 50%, a significant saving for high-throughput applications on embedded hardware.8. ConclusionThe implementation of a Quadratic First Derivative Savitzky-Golay filter with Variable Window Sizes requires a synthesis of robust mathematical theory and careful software architecture. By recognizing the Linear-Quadratic Equivalence on symmetric domains, the developer can employ the simplified Universal Generating Function derived in this report. However, the rigor of the implementation is defined by its handling of the "Variable Window" edge cases, where the Shrinking Symmetric Window strategy is recommended to prevent the instability inherent in asymmetric polynomial extrapolation.This report provides the definitions, derivations, and code structures necessary to deploy a production-grade differentiator that outperforms standard finite difference methods in noise suppression while preserving the fidelity of the underlying signal dynamics.References within text:.