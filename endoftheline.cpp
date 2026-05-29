#include <vector>
#include <cmath>
#include <thread>
#include <algorithm>

void process_block_vectorized(const std::vector<double>& input, std::vector<double>& output, int start, int end) {
    #pragma clang loop vectorize(enable) interleave(enable)
    for (int i = start; i < end; ++i) {
        output[i] = std::tanh(input[i] * 10.0);
    }
}

std::vector<double> execute_analog_pipeline_simd(const std::vector<double>& input_signals) {
    int N = input_signals.size();
    std::vector<double> output_signals(N);
    
    if (N <= 10000) {
        if (N > 0) {
            #pragma clang loop vectorize(enable)
            for (int i = 0; i < N; ++i) {
                output_signals[i] = std::tanh(input_signals[i] * 10.0);
            }
        }
        return output_signals;
    }

    unsigned int cores = std::thread::hardware_concurrency();
    if (cores == 0) cores = 4;
    
    cores = std::min(cores, static_cast<unsigned int>(N / 1000));
    if (cores == 0) cores = 1;

    std::vector<std::thread> threads;
    double block_size_exact = static_cast<double>(N) / cores;

    int current_start = 0;
    for (unsigned int i = 0; i < cores; ++i) {
        int current_end = static_cast<int>(std::round((i + 1) * block_size_exact));
        
        if (i < cores - 1 && current_end < N) {
            current_end = std::min(N, static_cast<int>((current_end + 7) & ~7));
        } else {
            current_end = N;
        }
        
        if (current_start < current_end) {
            threads.push_back(std::thread(process_block_vectorized, std::ref(input_signals), std::ref(output_signals), current_start, current_end));
        }
        
        current_start = current_end;
    }
    
    for (auto& t : threads) {
        if (t.joinable()) t.join();
    }
    
    return output_signals;
}
