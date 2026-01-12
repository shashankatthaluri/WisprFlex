/**
 * WisprFlex whisper.cpp Smoke Test
 * 
 * Phase 2.2.1: Minimal CLI test to verify whisper.cpp integration
 * 
 * This is NOT part of the engine - just proof of life.
 * 
 * Usage:
 *   whisper_smoke_test <model_path> [audio_file.wav]
 * 
 * If no audio file provided, generates silent test audio.
 */

#include "whisper_backend.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>
#include <cmath>

#define SAMPLE_RATE 16000

/**
 * Generate silent audio for testing
 */
std::vector<float> generate_test_audio(float duration_sec) {
    size_t n_samples = (size_t)(duration_sec * SAMPLE_RATE);
    std::vector<float> audio(n_samples);
    
    // Generate a simple tone (440Hz) for testing
    for (size_t i = 0; i < n_samples; i++) {
        audio[i] = 0.1f * sinf(2.0f * 3.14159f * 440.0f * i / SAMPLE_RATE);
    }
    
    return audio;
}

/**
 * Print metrics
 */
void print_metrics(const WBMetrics& metrics) {
    printf("\n--- Performance Metrics ---\n");
    printf("Model load time: %.2f ms\n", metrics.model_load_time_ms);
    printf("Inference time:  %.2f ms\n", metrics.last_inference_time_ms);
    printf("Model memory:    %zu bytes\n", metrics.model_memory_bytes);
    printf("Peak memory:     %zu bytes\n", metrics.peak_memory_bytes);
    printf("---------------------------\n");
}

int main(int argc, char** argv) {
    printf("========================================\n");
    printf("WisprFlex whisper.cpp Smoke Test\n");
    printf("Phase 2.2.1 - Proof of Life\n");
    printf("========================================\n\n");

    if (argc < 2) {
        printf("Usage: %s <model_path> [audio_file.wav]\n", argv[0]);
        printf("\nExample:\n");
        printf("  %s models/ggml-base.bin\n", argv[0]);
        return 1;
    }

    const char* model_path = argv[1];
    bool use_test_audio = (argc < 3);

    // Step 1: Initialize backend
    printf("[1/4] Initializing whisper backend...\n");
    WBErrorCode err = wb_init();
    if (err != WB_OK) {
        printf("FAILED: %s\n", wb_error_message(err));
        return 1;
    }
    printf("      OK\n");

    // Step 2: Load model
    printf("[2/4] Loading model: %s\n", model_path);
    err = wb_load_model(model_path);
    if (err != WB_OK) {
        printf("FAILED: %s\n", wb_error_message(err));
        wb_shutdown();
        return 1;
    }
    
    WBMetrics metrics = wb_get_metrics();
    printf("      OK (%.2f ms)\n", metrics.model_load_time_ms);

    // Step 3: Prepare audio
    printf("[3/4] Preparing audio...\n");
    std::vector<float> audio;
    
    if (use_test_audio) {
        printf("      Using generated test audio (5 seconds)\n");
        audio = generate_test_audio(5.0f);
    } else {
        // TODO: Load WAV file
        printf("      WAV loading not implemented, using test audio\n");
        audio = generate_test_audio(5.0f);
    }
    printf("      OK (%zu samples)\n", audio.size());

    // Step 4: Run transcription
    printf("[4/4] Running transcription...\n");
    
    char text_buffer[4096] = {0};
    WBTranscribeParams params = wb_default_params();
    params.language = "en";
    
    err = wb_transcribe(
        audio.data(),
        audio.size(),
        &params,
        text_buffer,
        sizeof(text_buffer)
    );
    
    if (err != WB_OK) {
        printf("FAILED: %s\n", wb_error_message(err));
        wb_shutdown();
        return 1;
    }
    
    printf("      OK\n");

    // Results
    printf("\n========================================\n");
    printf("TRANSCRIPTION RESULT:\n");
    printf("========================================\n");
    printf("%s\n", strlen(text_buffer) > 0 ? text_buffer : "(empty - expected for test audio)");
    printf("========================================\n");

    // Print metrics
    metrics = wb_get_metrics();
    print_metrics(metrics);

    // Cleanup
    wb_unload_model();
    wb_shutdown();

    printf("\n[PASS] Smoke test completed successfully\n\n");
    return 0;
}
