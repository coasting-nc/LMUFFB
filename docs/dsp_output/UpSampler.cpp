#include "upsampler.h"
#include <cmath>
#include <algorithm>
#include <numeric>

#ifndef PI
constexpr float M_PI = 3.14159265358979f;
#else
constexpr float M_PI = PI;
#endif

namespace LMUFFB {

SampleRateConverter::SampleRateConverter(int num_channels, int fir_taps)
    : m_num_channels(num_channels)
    , m_fir_taps(std::min(fir_taps, MAX_TAPS))
    , m_bypass(false)
    , m_phase(0)
{
    // Ensure fir_taps is even for symmetric filter
    if (m_fir_taps % 2 != 0) {
        m_fir_taps++;
    }
    
    designFIRFilter();
    
    // Initialize circular buffers (size = fir_taps + some margin)
    int buffer_size = m_fir_taps + UPSAMPLE_FACTOR;
    m_input_buffers.resize(m_num_channels, std::vector<float>(buffer_size, 0.0f));
    m_buffer_indices.resize(m_num_channels, 0);
}

void SampleRateConverter::reset() {
    m_phase = 0;
    for (auto& buffer : m_input_buffers) {
        std::fill(buffer.begin(), buffer.end(), 0.0f);
    }
    std::fill(m_buffer_indices.begin(), m_buffer_indices.end(), 0);
}

void SampleRateConverter::designFIRFilter() {
    // Design a low-pass FIR filter for 400Hz -> 1000Hz conversion
    // Cutoff frequency: 400Hz / 2 = 200Hz (Nyquist for 400Hz input)
    // But since we upsample by 5, the effective cutoff is 200Hz * 5 = 1000Hz
    // We want to filter out frequencies above 200Hz in the final 1000Hz output
    
    m_fir_coeffs.resize(m_fir_taps);
    
    // Use windowed sinc function for FIR design
    float cutoff_norm = 200.0f / (1000.0f * UPSAMPLE_FACTOR / 2.0f); // Normalized cutoff
    float window_sum = 0.0f;
    
    for (int i = 0; i < m_fir_taps; ++i) {
        int n = i - m_fir_taps / 2;
        if (n == 0) {
            m_fir_coeffs[i] = 2.0f * M_PI * cutoff_norm;
        } else {
            m_fir_coeffs[i] = std::sin(2.0f * M_PI * cutoff_norm * n) / n;
        }
        
        // Apply Blackman window for better stopband attenuation
        float window = 0.42f - 0.5f * std::cos(2.0f * M_PI * i / (m_fir_taps - 1)) + 
                       0.08f * std::cos(4.0f * M_PI * i / (m_fir_taps - 1));
        m_fir_coeffs[i] *= window;
        window_sum += window;
    }
    
    // Normalize to unity gain
    for (float& coeff : m_fir_coeffs) {
        coeff /= window_sum;
    }
}

int SampleRateConverter::process(const float* const* input, float** output, int input_count, int output_capacity) {
    if (m_bypass) {
        // Bypass mode: copy input to output (for testing)
        int copy_count = std::min(input_count, output_capacity);
        for (int ch = 0; ch < m_num_channels; ++ch) {
            std::copy_n(input[ch], copy_count, output[ch]);
        }
        return copy_count;
    }
    
    int output_samples = 0;
    
    for (int input_idx = 0; input_idx < input_count; ++input_idx) {
        // Upsample: insert zeros between input samples
        for (int up = 0; up < UPSAMPLE_FACTOR; ++up) {
            // Store in circular buffers for each channel
            for (int ch = 0; ch < m_num_channels; ++ch) {
                auto& buffer = m_input_buffers[ch];
                int& idx = m_buffer_indices[ch];
                
                // Add input sample or zero for upsampling
                float sample = (up == 0) ? input[ch][input_idx] : 0.0f;
                buffer[idx] = sample;
                idx = (idx + 1) % buffer.size();
            }
            
            // Apply polyphase filtering and downsampling
            if (m_phase == 0) {
                // Generate output sample
                if (output_samples < output_capacity) {
                    for (int ch = 0; ch < m_num_channels; ++ch) {
                        output[ch][output_samples] = applyFIR(m_input_buffers[ch], ch, getGroupDelay());
                    }
                    output_samples++;
                }
            }
            
            m_phase = (m_phase + 1) % DOWNSAMPLE_FACTOR;
        }
    }
    
    return output_samples;
}

int SampleRateConverter::process(const double* const* input, float** output, int input_count, int output_capacity) {
    // Convert to float buffers and delegate to float version
    std::vector<std::vector<float>> temp_in(m_num_channels, std::vector<float>(input_count, 0.0f));
    for (int ch = 0; ch < m_num_channels; ++ch) {
        for (int i = 0; i < input_count; ++i) {
            temp_in[ch][i] = static_cast<float>(input[ch][i]);
        }
    }
    std::vector<const float*> input_ptrs(m_num_channels);
    for (int ch = 0; ch < m_num_channels; ++ch) {
        input_ptrs[ch] = temp_in[ch].data();
    }
    return process(input_ptrs.data(), output, input_count, output_capacity);
}

float SampleRateConverter::applyFIR(const std::vector<float>& buffer, int channel, int delay) {
    float sum = 0.0f;
    int buffer_size = buffer.size();
    int idx = m_buffer_indices[channel];

    // Apply FIR filter with circular buffer access
    for (int i = 0; i < m_fir_taps; ++i) {
        int sample_idx = (idx - delay - i + buffer_size) % buffer_size;
        sum += buffer[sample_idx] * m_fir_coeffs[i];
    }

    return sum;
}

int SampleRateConverter::process(double (*input)[4], double (*output)[20], int input_count, int output_capacity) {
    // Convert array of arrays to array of pointers for the double overload
    const double* input_ptrs[4];
    for (int ch = 0; ch < 4; ++ch) {
        input_ptrs[ch] = input[ch];
    }
    // Call the double overload and copy back to double output
    //float* output_ptrs[4];
    //for (int ch = 0; ch < 4; ++ch) {
    //    output_ptrs[ch] = reinterpret_cast<float*>(output[ch]); // Cast double to float for output
    //}
    // Call the double overload, but since output is float**, and we need to cast back
    // Actually, the double overload outputs to float**, so we need to copy back to double
    std::vector<std::vector<float>> temp_output(4, std::vector<float>(output_capacity));
    float* temp_ptrs[4];
    for (int ch = 0; ch < 4; ++ch) {
        temp_ptrs[ch] = temp_output[ch].data();
    }
    int samples = process(input_ptrs, temp_ptrs, input_count, output_capacity);
    // Copy back to double output
    for (int ch = 0; ch < 4; ++ch) {
        for (int i = 0; i < samples; ++i) {
            output[ch][i] = static_cast<double>(temp_output[ch][i]);
        }
    }
    return samples;
}

} // namespace LMUFFB
