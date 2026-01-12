/**
 * Phase 2.2.3 Single-Shot Transcription Test
 * Agent D: Validation & Comparison
 * 
 * Runs transcription on test audio files and outputs results
 * for comparison with Python Whisper.
 */

#include "whisper_backend.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>
#include <fstream>
#include <chrono>

// Simple WAV header parser
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
    if (!file) {
        printf("Error: Cannot open file %s\n", path);
        return false;
    }

    WavHeader header;
    file.read(reinterpret_cast<char*>(&header), sizeof(WavHeader));

    // Find data chunk
    char chunkId[4];
    uint32_t chunkSize;
    
    while (file.read(chunkId, 4)) {
        file.read(reinterpret_cast<char*>(&chunkSize), 4);
        if (strncmp(chunkId, "data", 4) == 0) {
            break;
        }
        file.seekg(chunkSize, std::ios::cur);
    }

    sample_rate = header.sampleRate;
    
    // Read audio data
    if (header.bitsPerSample == 16) {
        std::vector<int16_t> raw(chunkSize / 2);
        file.read(reinterpret_cast<char*>(raw.data()), chunkSize);
        
        samples.resize(raw.size());
        for (size_t i = 0; i < raw.size(); i++) {
            samples[i] = raw[i] / 32768.0f;
        }
        
        // If stereo, convert to mono
        if (header.numChannels == 2) {
            std::vector<float> mono(samples.size() / 2);
            for (size_t i = 0; i < mono.size(); i++) {
                mono[i] = (samples[i * 2] + samples[i * 2 + 1]) / 2.0f;
            }
            samples = mono;
        }
        
        return true;
    }
    
    printf("Error: Unsupported format (bits=%d)\n", header.bitsPerSample);
    return false;
}

// Resample to 16kHz (simple linear interpolation)
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

int main(int argc, char** argv) {
    if (argc < 3) {
        printf("Usage: %s <model_path> <audio_file.wav> [audio2.wav ...]\n", argv[0]);
        return 1;
    }

    const char* model_path = argv[1];

    printf("\n");
    printf("========================================\n");
    printf("Phase 2.2.3 Transcription Validation\n");
    printf("Agent D: Single-Shot Test\n");
    printf("========================================\n\n");

    // Initialize
    WBErrorCode err = wb_init();
    if (err != WB_OK) {
        printf("FAIL: Init failed: %s\n", wb_error_message(err));
        return 1;
    }

    // Load model
    printf("Loading model: %s\n", model_path);
    err = wb_load_model(model_path);
    if (err != WB_OK) {
        printf("FAIL: Model load failed: %s\n", wb_error_message(err));
        wb_shutdown();
        return 1;
    }
    printf("Model loaded.\n\n");

    // Process each audio file
    for (int i = 2; i < argc; i++) {
        const char* audio_path = argv[i];
        
        printf("----------------------------------------\n");
        printf("File: %s\n", audio_path);
        printf("----------------------------------------\n");

        // Load WAV
        std::vector<float> samples;
        uint32_t sample_rate;
        
        if (!load_wav(audio_path, samples, sample_rate)) {
            printf("SKIP: Failed to load audio\n\n");
            continue;
        }
        
        printf("  Sample rate: %u Hz\n", sample_rate);
        printf("  Duration: %.2f seconds\n", (float)samples.size() / sample_rate);

        // Resample to 16kHz if needed
        if (sample_rate != 16000) {
            printf("  Resampling to 16kHz...\n");
            samples = resample_to_16k(samples, sample_rate);
        }

        // Transcribe
        char text_buffer[8192] = {0};
        WBTranscribeParams params = wb_default_params();
        params.language = "en";

        auto start = std::chrono::high_resolution_clock::now();
        err = wb_transcribe(samples.data(), samples.size(), &params, text_buffer, sizeof(text_buffer));
        auto end = std::chrono::high_resolution_clock::now();

        double ms = std::chrono::duration<double, std::milli>(end - start).count();

        if (err != WB_OK) {
            printf("FAIL: Transcription failed: %s\n\n", wb_error_message(err));
            continue;
        }

        printf("  Inference time: %.2f ms\n", ms);
        printf("\n  [TRANSCRIPT]\n");
        printf("  %s\n\n", text_buffer);
    }

    // Cleanup
    wb_unload_model();
    wb_shutdown();

    printf("========================================\n");
    printf("Transcription test complete.\n");
    printf("========================================\n");

    return 0;
}
