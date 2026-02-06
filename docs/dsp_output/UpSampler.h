#pragma once

#include <vector>
#include <array>
#include <cstdint>

namespace LMUFFB {

/**
 * Sample Rate Converter for FFB DSP Pipeline
 * Converts 400Hz input to 1000Hz output using 5/2 rational resampling
 * 
 * Algorithm: Upsample by 5, FIR lowpass filter, downsample by 2
 * Result: 400Hz * 5 / 2 = 1000Hz
 * 
 * Latency: Fixed group delay for consistency (8-16 taps configurable)
 * Precision: 32-bit float for calculations
 */
class SampleRateConverter {
public:
    // Configuration
    static constexpr int UPSAMPLE_FACTOR = 5;
    static constexpr int DOWNSAMPLE_FACTOR = 2;
    static constexpr int CONVERSION_RATIO = UPSAMPLE_FACTOR / DOWNSAMPLE_FACTOR; // 2.5 -> 1000Hz from 400Hz
    
    // FIR filter configuration
    static constexpr int MAX_TAPS = 16;
    static constexpr int DEFAULT_TAPS = 12; // Good balance of quality vs latency
    
    SampleRateConverter(int num_channels = 4, int fir_taps = DEFAULT_TAPS);
    ~SampleRateConverter() = default;
    
    // Disable copy/move for real-time safety
    SampleRateConverter(const SampleRateConverter&) = delete;
    SampleRateConverter& operator=(const SampleRateConverter&) = delete;
    
    /**
     * Process a block of samples
     * @param input Array of input samples (one per channel) at 400Hz
     * @param output Array to store output samples (one per channel) at 1000Hz
     * @param input_count Number of input samples to process
     * @param output_capacity Maximum output samples that can be stored
     * @return Number of output samples generated
     */
    int process(const float* const* input, float** output, int input_count, int output_capacity);

    int process(const double* const* input, float** output, int input_count, int output_capacity);

    // Overload for test compatibility (double arrays)
    int process(double (*input)[4], double (*output)[20], int input_count, int output_capacity);
    
    /**
     * Reset internal state (call when starting new session)
     */
    void reset();
    
    /**
     * Enable/disable bypass mode for testing
     */
    void setBypass(bool bypass) { m_bypass = bypass; }
    
    /**
     * Get current group delay in samples
     */
    int getGroupDelay() const { return m_fir_taps / 2; }
    
    /**
     * Get expected output samples for given input count
     */
    static int getExpectedOutputSamples(int input_samples) {
        return (input_samples * UPSAMPLE_FACTOR) / DOWNSAMPLE_FACTOR;
    }

private:
    int m_num_channels;
    int m_fir_taps;
    bool m_bypass;
    
    // FIR filter coefficients (designed for 400Hz -> 1000Hz conversion)
    std::vector<float> m_fir_coeffs;
    
    // Circular buffers for each channel
    std::vector<std::vector<float>> m_input_buffers;
    std::vector<int> m_buffer_indices;
    
    // Polyphase filter state
    int m_phase;
    
    // Design the FIR filter coefficients
    void designFIRFilter();
    
    // Apply FIR filter to a channel
    float applyFIR(const std::vector<float>& buffer, int channel, int delay);
};

} // namespace LMUFFB