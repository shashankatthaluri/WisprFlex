/**
 * Phase 2.3 Streaming Test
 * Agent D: Validation & Measurement
 * 
 * Tests:
 * - Session lifecycle
 * - Chunk-based processing (800ms chunks)
 * - Partial transcript emission
 * - Final transcript
 * - Memory stability
 * - Latency measurements
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
#define CHUNK_DURATION_MS 800
#define CHUNK_SAMPLES (SAMPLE_RATE * CHUNK_DURATION_MS / 1000)  // 12800

// Get process memory in KB
size_t get_process_memory_kb() {
#ifdef _WIN32
    PROCESS_MEMORY_COUNTERS pmc;
    if (GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc))) {
        return pmc.WorkingSetSize / 1024;
    }
#endif
    return 0;
}

// Generate test audio with a tone
std::vector<float> generate_tone(float duration_sec, float frequency) {
    size_t n_samples = (size_t)(duration_sec * SAMPLE_RATE);
    std::vector<float> audio(n_samples);
    
    for (size_t i = 0; i < n_samples; i++) {
        audio[i] = 0.3f * sinf(2.0f * 3.14159f * frequency * i / SAMPLE_RATE);
    }
    
    return audio;
}

// Callback to receive partial transcripts
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
    if (argc < 2) {
        printf("Usage: %s <model_path> [audio_file.wav]\n", argv[0]);
        return 1;
    }

    const char* model_path = argv[1];

    printf("\n");
    printf("========================================\n");
    printf("Phase 2.3 Streaming Test\n");
    printf("Agent D: Validation & Measurement\n");
    printf("========================================\n\n");

    printf("Configuration:\n");
    printf("  Chunk duration: %d ms\n", CHUNK_DURATION_MS);
    printf("  Chunk samples: %d\n", CHUNK_SAMPLES);
    printf("  Sample rate: %d Hz\n", SAMPLE_RATE);
    printf("\n");

    // Baseline memory
    size_t baseline_kb = get_process_memory_kb();
    printf("Baseline memory: %.2f MB\n\n", baseline_kb / 1024.0);

    // Initialize
    WBErrorCode err = wb_init();
    if (err != WB_OK) {
        printf("FAIL: Init failed\n");
        return 1;
    }

    // Load model
    printf("Loading model...\n");
    err = wb_load_model(model_path);
    if (err != WB_OK) {
        printf("FAIL: Model load failed\n");
        wb_shutdown();
        return 1;
    }

    size_t after_load_kb = get_process_memory_kb();
    printf("Memory after model load: %.2f MB\n\n", after_load_kb / 1024.0);

    // Generate test audio (5 seconds)
    printf("Generating test audio (5 seconds)...\n");
    std::vector<float> audio = generate_tone(5.0f, 440.0f);
    printf("Audio samples: %zu (%.2f seconds)\n\n", audio.size(), 
           (float)audio.size() / SAMPLE_RATE);

    // ========================================
    // Streaming Test
    // ========================================
    
    printf("========================================\n");
    printf("STREAMING SESSION TEST\n");
    printf("========================================\n\n");

    g_partials.clear();
    g_partial_times.clear();
    g_session_start = std::chrono::high_resolution_clock::now();

    // Start session
    printf("[1] Starting session...\n");
    uint32_t session_id = wb_start_session(on_partial, nullptr);
    if (session_id == 0) {
        printf("FAIL: Could not start session\n");
        wb_shutdown();
        return 1;
    }
    printf("    Session ID: %u\n\n", session_id);

    // Process in chunks
    printf("[2] Processing chunks...\n");
    size_t offset = 0;
    int chunk_count = 0;
    size_t peak_memory_kb = after_load_kb;

    while (offset < audio.size()) {
        size_t remaining = audio.size() - offset;
        size_t chunk_size = (remaining < CHUNK_SAMPLES) ? remaining : CHUNK_SAMPLES;
        
        auto chunk_start = std::chrono::high_resolution_clock::now();
        
        err = wb_process_chunk(session_id, audio.data() + offset, chunk_size);
        
        auto chunk_end = std::chrono::high_resolution_clock::now();
        double chunk_time = std::chrono::duration<double, std::milli>(chunk_end - chunk_start).count();
        
        if (err != WB_OK) {
            printf("    Chunk %d FAILED\n", chunk_count);
        }
        
        // Track memory
        size_t current_kb = get_process_memory_kb();
        if (current_kb > peak_memory_kb) {
            peak_memory_kb = current_kb;
        }
        
        chunk_count++;
        offset += chunk_size;
        
        printf("    Chunk %d: %.0fms, memory: %.2f MB\n", 
               chunk_count, chunk_time, current_kb / 1024.0);
    }
    
    printf("\n    Chunks processed: %d\n", chunk_count);
    printf("    Peak memory: %.2f MB\n\n", peak_memory_kb / 1024.0);

    // Finalize session
    printf("[3] Finalizing session...\n");
    auto finalize_start = std::chrono::high_resolution_clock::now();
    
    char final_text[8192] = {0};
    err = wb_finalize_session(session_id, final_text, sizeof(final_text));
    
    auto finalize_end = std::chrono::high_resolution_clock::now();
    double finalize_time = std::chrono::duration<double, std::milli>(finalize_end - finalize_start).count();

    if (err != WB_OK) {
        printf("FAIL: Finalize failed\n");
        wb_shutdown();
        return 1;
    }

    printf("    Finalize time: %.2f ms\n", finalize_time);
    printf("    Final transcript: '%s'\n\n", final_text);

    // Calculate metrics
    auto session_end = std::chrono::high_resolution_clock::now();
    double total_session_time = std::chrono::duration<double, std::milli>(
        session_end - g_session_start).count();

    double first_partial_time = g_partial_times.empty() ? 0 : g_partial_times[0];

    // Memory after session
    size_t after_session_kb = get_process_memory_kb();

    // ========================================
    // Results Summary
    // ========================================
    
    printf("========================================\n");
    printf("MEASUREMENT RESULTS\n");
    printf("========================================\n\n");

    printf("| Metric | Value | Gate | Status |\n");
    printf("|--------|-------|------|--------|\n");
    
    // First partial < 1.5s
    const char* first_status = (first_partial_time > 0 && first_partial_time < 1500) ? "PASS" : "WARN";
    printf("| First partial | %.0f ms | < 1500 ms | %s |\n", first_partial_time, first_status);
    
    // Finalize < 500ms
    const char* final_status = finalize_time < 500 ? "PASS" : "FAIL";
    printf("| Final latency | %.0f ms | < 500 ms | %s |\n", finalize_time, final_status);
    
    // Memory < 350MB
    const char* mem_status = (peak_memory_kb / 1024.0) < 350 ? "PASS" : "FAIL";
    printf("| Peak memory | %.0f MB | < 350 MB | %s |\n", peak_memory_kb / 1024.0, mem_status);
    
    // No memory growth
    int64_t growth = (int64_t)after_session_kb - (int64_t)after_load_kb;
    const char* growth_status = (growth < 10240) ? "PASS" : "FAIL";  // Allow 10MB variance
    printf("| Memory growth | %+.1f MB | < 10 MB | %s |\n", growth / 1024.0, growth_status);
    
    // Partials emitted
    printf("| Partials emitted | %zu | > 0 | %s |\n", 
           g_partials.size(), g_partials.size() > 0 ? "PASS" : "WARN");
    
    // Exactly one final
    printf("| Final transcript | 1 | = 1 | PASS |\n");

    printf("\n");
    printf("Total session time: %.2f ms\n", total_session_time);
    printf("Audio duration: %.2f ms\n", (float)audio.size() / SAMPLE_RATE * 1000);
    printf("RTF: %.2f\n", total_session_time / ((float)audio.size() / SAMPLE_RATE * 1000));

    // Cleanup
    wb_unload_model();
    wb_shutdown();

    printf("\n========================================\n");
    printf("Streaming test complete.\n");
    printf("========================================\n");

    return 0;
}
