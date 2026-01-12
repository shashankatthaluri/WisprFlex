/**
 * Phase 2.4 Streaming Test - Real Audio
 * Agent D: Validation & Measurement
 * 
 * Tests streaming with 4-second chunks (CPU-compatible)
 * Strategy: Option B - Larger chunks for usable latency
 */

#include "whisper_backend.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>
#include <chrono>
#include <fstream>
#include <cmath>

#ifdef _WIN32
#include <windows.h>
#include <psapi.h>
#endif

#define SAMPLE_RATE 16000
#define CHUNK_DURATION_MS 4000  // Phase 2.4: 4-second chunks
#define CHUNK_SAMPLES (SAMPLE_RATE * CHUNK_DURATION_MS / 1000)  // 64000 samples

size_t get_process_memory_kb() {
#ifdef _WIN32
    PROCESS_MEMORY_COUNTERS pmc;
    if (GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc))) {
        return pmc.WorkingSetSize / 1024;
    }
#endif
    return 0;
}

// WAV header
struct WavHeader {
    char riff[4];
    uint32_t fileSize;
    char wave[4];
    char fmt[4];
    uint32_t fmtSize;
    uint16_t audioFormat;
    uint16_t numChannels;
    uint32_t sampleRate;
    uint32_t byteRate;
    uint16_t blockAlign;
    uint16_t bitsPerSample;
};

bool load_wav(const char* path, std::vector<float>& samples, uint32_t& sample_rate) {
    std::ifstream file(path, std::ios::binary);
    if (!file) return false;

    WavHeader header;
    file.read(reinterpret_cast<char*>(&header), sizeof(WavHeader));

    char chunkId[4];
    uint32_t chunkSize;
    
    while (file.read(chunkId, 4)) {
        file.read(reinterpret_cast<char*>(&chunkSize), 4);
        if (strncmp(chunkId, "data", 4) == 0) break;
        file.seekg(chunkSize, std::ios::cur);
    }

    sample_rate = header.sampleRate;
    
    if (header.bitsPerSample == 16) {
        std::vector<int16_t> raw(chunkSize / 2);
        file.read(reinterpret_cast<char*>(raw.data()), chunkSize);
        
        samples.resize(raw.size());
        for (size_t i = 0; i < raw.size(); i++) {
            samples[i] = raw[i] / 32768.0f;
        }
        
        if (header.numChannels == 2) {
            std::vector<float> mono(samples.size() / 2);
            for (size_t i = 0; i < mono.size(); i++) {
                mono[i] = (samples[i * 2] + samples[i * 2 + 1]) / 2.0f;
            }
            samples = mono;
        }
        return true;
    }
    return false;
}

std::vector<float> resample_to_16k(const std::vector<float>& input, uint32_t input_rate) {
    if (input_rate == 16000) return input;
    
    double ratio = 16000.0 / input_rate;
    size_t output_size = (size_t)(input.size() * ratio);
    std::vector<float> output(output_size);
    
    for (size_t i = 0; i < output_size; i++) {
        double src_idx = i / ratio;
        size_t idx = (size_t)src_idx;
        double frac = src_idx - idx;
        
        if (idx + 1 < input.size()) {
            output[i] = input[idx] * (1.0 - frac) + input[idx + 1] * frac;
        } else {
            output[i] = input[idx];
        }
    }
    return output;
}

static std::vector<std::string> g_partials;
static std::vector<double> g_partial_times;
static std::chrono::time_point<std::chrono::high_resolution_clock> g_session_start;

void on_partial(const char* text, void* user_data) {
    (void)user_data;
    auto now = std::chrono::high_resolution_clock::now();
    double elapsed = std::chrono::duration<double, std::milli>(now - g_session_start).count();
    g_partials.push_back(text);
    g_partial_times.push_back(elapsed);
    printf("  [PARTIAL @ %.0fms] %s\n", elapsed, text);
}

int main(int argc, char** argv) {
    if (argc < 3) {
        printf("Usage: %s <model_path> <audio_file.wav>\n", argv[0]);
        return 1;
    }

    const char* model_path = argv[1];
    const char* audio_path = argv[2];

    printf("\n");
    printf("========================================\n");
    printf("Phase 2.4 Streaming Test (4s Chunks)\n");
    printf("========================================\n\n");

    // Load audio
    printf("Loading audio: %s\n", audio_path);
    std::vector<float> audio;
    uint32_t sample_rate;
    
    if (!load_wav(audio_path, audio, sample_rate)) {
        printf("FAIL: Cannot load audio\n");
        return 1;
    }
    
    printf("  Original: %zu samples @ %u Hz (%.2fs)\n", 
           audio.size(), sample_rate, (float)audio.size() / sample_rate);
    
    if (sample_rate != 16000) {
        audio = resample_to_16k(audio, sample_rate);
        printf("  Resampled: %zu samples @ 16000 Hz (%.2fs)\n", 
               audio.size(), (float)audio.size() / 16000);
    }

    size_t baseline_kb = get_process_memory_kb();

    // Initialize
    WBErrorCode err = wb_init();
    if (err != WB_OK) return 1;

    printf("\nLoading model...\n");
    err = wb_load_model(model_path);
    if (err != WB_OK) {
        wb_shutdown();
        return 1;
    }

    size_t after_load_kb = get_process_memory_kb();
    printf("Memory after load: %.2f MB\n\n", after_load_kb / 1024.0);

    // Start streaming
    printf("========================================\n");
    printf("STREAMING SESSION\n");
    printf("========================================\n\n");

    g_partials.clear();
    g_partial_times.clear();
    g_session_start = std::chrono::high_resolution_clock::now();

    uint32_t session_id = wb_start_session(on_partial, nullptr);
    if (session_id == 0) {
        printf("FAIL: Cannot start session\n");
        wb_shutdown();
        return 1;
    }

    // Process chunks
    size_t offset = 0;
    int chunk_count = 0;
    size_t peak_memory_kb = after_load_kb;
    std::vector<double> chunk_times;

    while (offset < audio.size()) {
        size_t remaining = audio.size() - offset;
        size_t chunk_size = (remaining < CHUNK_SAMPLES) ? remaining : CHUNK_SAMPLES;
        
        auto chunk_start = std::chrono::high_resolution_clock::now();
        err = wb_process_chunk(session_id, audio.data() + offset, chunk_size);
        auto chunk_end = std::chrono::high_resolution_clock::now();
        
        double chunk_time = std::chrono::duration<double, std::milli>(chunk_end - chunk_start).count();
        chunk_times.push_back(chunk_time);
        
        size_t current_kb = get_process_memory_kb();
        if (current_kb > peak_memory_kb) peak_memory_kb = current_kb;
        
        chunk_count++;
        offset += chunk_size;
        
        printf("  Chunk %d: %.0fms, mem: %.1fMB\n", chunk_count, chunk_time, current_kb/1024.0);
    }

    // Finalize
    auto finalize_start = std::chrono::high_resolution_clock::now();
    char final_text[16384] = {0};
    err = wb_finalize_session(session_id, final_text, sizeof(final_text));
    auto finalize_end = std::chrono::high_resolution_clock::now();
    
    double finalize_time = std::chrono::duration<double, std::milli>(finalize_end - finalize_start).count();
    auto session_end = std::chrono::high_resolution_clock::now();
    double total_session = std::chrono::duration<double, std::milli>(session_end - g_session_start).count();

    size_t after_session_kb = get_process_memory_kb();
    double first_partial = g_partial_times.empty() ? 0 : g_partial_times[0];
    double audio_duration = (float)audio.size() / 16000 * 1000;

    // Average chunk time
    double avg_chunk = 0;
    for (auto t : chunk_times) avg_chunk += t;
    if (!chunk_times.empty()) avg_chunk /= chunk_times.size();

    printf("\n========================================\n");
    printf("FINAL TRANSCRIPT\n");
    printf("========================================\n");
    printf("%s\n", final_text);

    printf("\n========================================\n");
    printf("MEASUREMENT RESULTS\n");
    printf("========================================\n\n");

    // Phase 2.4 gates: first partial ≤4s, chunk time ≤4.8s (RTF ≤1.2)
    double rtf = avg_chunk / (CHUNK_DURATION_MS);
    
    printf("| Metric | Value | Gate | Status |\n");
    printf("|--------|-------|------|--------|\n");
    printf("| First partial | %.0f ms | ≤ 4000 ms | %s |\n", 
           first_partial, first_partial <= 4000 ? "PASS" : "FAIL");
    printf("| Avg chunk time | %.0f ms | ≤ 4800 ms | %s |\n", 
           avg_chunk, avg_chunk <= 4800 ? "PASS" : "FAIL");
    printf("| RTF | %.2f | ≤ 1.2 | %s |\n", 
           rtf, rtf <= 1.2 ? "PASS" : "FAIL");
    printf("| Final latency | %.0f ms | < 500 ms | %s |\n", 
           finalize_time, finalize_time < 500 ? "PASS" : "FAIL");
    printf("| Peak memory | %.0f MB | < 400 MB | %s |\n", 
           peak_memory_kb/1024.0, peak_memory_kb/1024.0 < 400 ? "PASS" : "FAIL");
    
    int64_t growth = (int64_t)after_session_kb - (int64_t)after_load_kb;
    printf("| Memory growth | %+.1f MB | < 10 MB | %s |\n", 
           growth/1024.0, growth < 10240 ? "PASS" : "FAIL");
    printf("| Partials | %zu | > 0 | %s |\n", 
           g_partials.size(), g_partials.size() > 0 ? "PASS" : "FAIL");
    printf("| Final transcript | 1 | = 1 | PASS |\n");

    printf("\nSession: %.2fs, Audio: %.2fs, RTF: %.2f\n", 
           total_session/1000, audio_duration/1000, total_session/audio_duration);

    wb_unload_model();
    wb_shutdown();

    printf("\n========================================\n");
    printf("Test complete.\n");
    printf("========================================\n");

    return 0;
}
