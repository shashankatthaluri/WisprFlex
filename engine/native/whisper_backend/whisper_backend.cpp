/**
 * WisprFlex whisper.cpp Backend - Implementation
 * 
 * Phase 2.2.1: Thin wrapper over whisper.cpp
 * 
 * This implementation:
 * - Wraps whisper.h API
 * - Manages context lifecycle
 * - Provides C interface for engine integration
 * 
 * CPU-only, no GPU flags.
 */

#include "whisper_backend.h"

// whisper.cpp header (from third_party/whisper.cpp)
#include "whisper.h"

#include <cstring>
#include <cstdio>
#include <chrono>
#include <mutex>
#include <string>

/* ============================================
 * Internal State
 * ============================================ */

static std::mutex g_mutex;
static bool g_initialized = false;
static struct whisper_context* g_ctx = nullptr;
static std::string g_model_path;
static WBMetrics g_metrics = {0};

/* ============================================
 * Error Messages
 * ============================================ */

static const char* ERROR_MESSAGES[] = {
    "Success",                      // WB_OK
    "Initialization failed",        // WB_ERROR_INIT_FAILED
    "Model file not found",         // WB_ERROR_MODEL_NOT_FOUND
    "Failed to load model",         // WB_ERROR_MODEL_LOAD_FAILED
    "Out of memory",                // WB_ERROR_OUT_OF_MEMORY
    "Inference failed",             // WB_ERROR_INFERENCE_FAILED
    "Invalid audio data",           // WB_ERROR_INVALID_AUDIO
    "Backend not initialized"       // WB_ERROR_NOT_INITIALIZED
};

const char* wb_error_message(WBErrorCode code) {
    if (code >= 0 && code <= WB_ERROR_NOT_INITIALIZED) {
        return ERROR_MESSAGES[code];
    }
    return "Unknown error";
}

/* ============================================
 * Backend Lifecycle
 * ============================================ */

WBErrorCode wb_init(void) {
    std::lock_guard<std::mutex> lock(g_mutex);
    
    if (g_initialized) {
        return WB_OK;  // Already initialized
    }
    
    // Reset metrics
    g_metrics = {0};
    g_initialized = true;
    
    printf("[whisper_backend] Initialized\n");
    return WB_OK;
}

int wb_is_initialized(void) {
    std::lock_guard<std::mutex> lock(g_mutex);
    return g_initialized ? 1 : 0;
}

WBErrorCode wb_shutdown(void) {
    std::lock_guard<std::mutex> lock(g_mutex);
    
    if (g_ctx) {
        whisper_free(g_ctx);
        g_ctx = nullptr;
    }
    
    g_model_path.clear();
    g_initialized = false;
    
    printf("[whisper_backend] Shutdown\n");
    return WB_OK;
}

/* ============================================
 * Model Management
 * ============================================ */

WBErrorCode wb_load_model(const char* model_path) {
    std::lock_guard<std::mutex> lock(g_mutex);
    
    if (!g_initialized) {
        return WB_ERROR_NOT_INITIALIZED;
    }
    
    if (!model_path || strlen(model_path) == 0) {
        return WB_ERROR_MODEL_NOT_FOUND;
    }
    
    // Unload previous model if any
    if (g_ctx) {
        whisper_free(g_ctx);
        g_ctx = nullptr;
    }
    
    printf("[whisper_backend] Loading model: %s\n", model_path);
    
    // Measure load time
    auto start = std::chrono::high_resolution_clock::now();
    
    // Load model using whisper.cpp
    g_ctx = whisper_init_from_file(model_path);
    
    auto end = std::chrono::high_resolution_clock::now();
    g_metrics.model_load_time_ms = 
        std::chrono::duration<double, std::milli>(end - start).count();
    
    if (!g_ctx) {
        printf("[whisper_backend] Failed to load model\n");
        return WB_ERROR_MODEL_LOAD_FAILED;
    }
    
    g_model_path = model_path;
    
    // Estimate memory usage (rough)
    // whisper.cpp doesn't expose exact memory, using file size as proxy
    // In real implementation, would use system memory APIs
    g_metrics.model_memory_bytes = 0;  // TODO: Measure actual memory
    
    printf("[whisper_backend] Model loaded in %.2f ms\n", 
           g_metrics.model_load_time_ms);
    
    return WB_OK;
}

WBErrorCode wb_unload_model(void) {
    std::lock_guard<std::mutex> lock(g_mutex);
    
    if (g_ctx) {
        whisper_free(g_ctx);
        g_ctx = nullptr;
        g_model_path.clear();
        printf("[whisper_backend] Model unloaded\n");
    }
    
    return WB_OK;
}

int wb_is_model_loaded(void) {
    std::lock_guard<std::mutex> lock(g_mutex);
    return g_ctx != nullptr ? 1 : 0;
}

WBErrorCode wb_get_model_info(char* out_info, size_t info_size) {
    std::lock_guard<std::mutex> lock(g_mutex);
    
    if (!g_ctx) {
        snprintf(out_info, info_size, "No model loaded");
        return WB_OK;
    }
    
    snprintf(out_info, info_size, 
             "Model: %s\n"
             "Load time: %.2f ms",
             g_model_path.c_str(),
             g_metrics.model_load_time_ms);
    
    return WB_OK;
}

/* ============================================
 * Transcription
 * ============================================ */

WBTranscribeParams wb_default_params(void) {
    WBTranscribeParams params;
    params.language = nullptr;  // Auto-detect
    params.translate = 0;       // No translation
    params.n_threads = 0;       // Auto
    return params;
}

WBErrorCode wb_transcribe(
    const float* pcm_data,
    size_t n_samples,
    const WBTranscribeParams* params,
    char* out_text,
    size_t text_size
) {
    std::lock_guard<std::mutex> lock(g_mutex);
    
    if (!g_initialized) {
        return WB_ERROR_NOT_INITIALIZED;
    }
    
    if (!g_ctx) {
        return WB_ERROR_MODEL_LOAD_FAILED;
    }
    
    if (!pcm_data || n_samples == 0) {
        return WB_ERROR_INVALID_AUDIO;
    }
    
    if (!out_text || text_size == 0) {
        return WB_ERROR_INFERENCE_FAILED;
    }
    
    // Set up whisper parameters
    struct whisper_full_params wparams = 
        whisper_full_default_params(WHISPER_SAMPLING_GREEDY);
    
    wparams.print_progress = false;
    wparams.print_special = false;
    wparams.print_realtime = false;
    wparams.print_timestamps = false;
    wparams.translate = params ? params->translate : 0;
    wparams.single_segment = false;
    wparams.no_context = true;
    
    if (params && params->language) {
        wparams.language = params->language;
    }
    
    if (params && params->n_threads > 0) {
        wparams.n_threads = params->n_threads;
    }
    
    printf("[whisper_backend] Running inference on %zu samples...\n", n_samples);
    
    // Measure inference time
    auto start = std::chrono::high_resolution_clock::now();
    
    // Run inference
    int result = whisper_full(g_ctx, wparams, pcm_data, (int)n_samples);
    
    auto end = std::chrono::high_resolution_clock::now();
    g_metrics.last_inference_time_ms = 
        std::chrono::duration<double, std::milli>(end - start).count();
    
    if (result != 0) {
        printf("[whisper_backend] Inference failed with code %d\n", result);
        return WB_ERROR_INFERENCE_FAILED;
    }
    
    // Extract text from all segments
    std::string full_text;
    int n_segments = whisper_full_n_segments(g_ctx);
    
    for (int i = 0; i < n_segments; i++) {
        const char* segment_text = whisper_full_get_segment_text(g_ctx, i);
        if (segment_text) {
            full_text += segment_text;
        }
    }
    
    // Copy to output buffer
    strncpy(out_text, full_text.c_str(), text_size - 1);
    out_text[text_size - 1] = '\0';
    
    printf("[whisper_backend] Inference completed in %.2f ms, %d segments\n",
           g_metrics.last_inference_time_ms, n_segments);
    
    return WB_OK;
}

/* ============================================
 * Metrics
 * ============================================ */

WBMetrics wb_get_metrics(void) {
    std::lock_guard<std::mutex> lock(g_mutex);
    return g_metrics;
}

/* ============================================
 * Phase 2.3: Streaming Session Implementation
 * ============================================ */

#include <vector>
#include <atomic>

// Silence detection threshold (energy-based)
static const float SILENCE_THRESHOLD = 0.001f;

// Session state
struct StreamingSession {
    uint32_t id;
    bool active;
    WBPartialCallback callback;
    void* user_data;
    std::vector<std::string> partial_transcripts;
    std::chrono::time_point<std::chrono::high_resolution_clock> start_time;
};

static StreamingSession g_session = {0};
static std::atomic<uint32_t> g_next_session_id{1};

uint32_t wb_start_session(WBPartialCallback callback, void* user_data) {
    std::lock_guard<std::mutex> lock(g_mutex);
    
    if (!g_initialized) {
        return 0;
    }
    
    if (!g_ctx) {
        printf("[whisper_backend] Cannot start session: no model loaded\n");
        return 0;
    }
    
    if (g_session.active) {
        printf("[whisper_backend] Cannot start session: one already active\n");
        return 0;
    }
    
    // Initialize session
    g_session.id = g_next_session_id++;
    g_session.active = true;
    g_session.callback = callback;
    g_session.user_data = user_data;
    g_session.partial_transcripts.clear();
    g_session.start_time = std::chrono::high_resolution_clock::now();
    
    printf("[whisper_backend] Session %u started\n", g_session.id);
    return g_session.id;
}

int wb_is_session_active(uint32_t session_id) {
    std::lock_guard<std::mutex> lock(g_mutex);
    return (g_session.active && g_session.id == session_id) ? 1 : 0;
}

int wb_is_silent(const float* pcm_data, size_t n_samples) {
    if (!pcm_data || n_samples == 0) {
        return 1;  // Treat invalid input as silent
    }
    
    // Calculate RMS energy
    float energy = 0.0f;
    for (size_t i = 0; i < n_samples; i++) {
        energy += pcm_data[i] * pcm_data[i];
    }
    energy = energy / (float)n_samples;
    
    return (energy < SILENCE_THRESHOLD) ? 1 : 0;
}

WBErrorCode wb_process_chunk(
    uint32_t session_id,
    const float* pcm_data,
    size_t n_samples
) {
    std::lock_guard<std::mutex> lock(g_mutex);
    
    if (!g_initialized) {
        return WB_ERROR_NOT_INITIALIZED;
    }
    
    if (!g_session.active || g_session.id != session_id) {
        printf("[whisper_backend] Invalid session ID: %u\n", session_id);
        return WB_ERROR_INFERENCE_FAILED;
    }
    
    if (!pcm_data || n_samples == 0) {
        return WB_ERROR_INVALID_AUDIO;
    }
    
    if (!g_ctx) {
        return WB_ERROR_MODEL_LOAD_FAILED;
    }
    
    // Per-chunk inference (stateless at whisper.cpp level)
    struct whisper_full_params wparams = 
        whisper_full_default_params(WHISPER_SAMPLING_GREEDY);
    
    wparams.print_progress = false;
    wparams.print_special = false;
    wparams.print_realtime = false;
    wparams.print_timestamps = false;
    wparams.translate = 0;
    wparams.single_segment = true;  // Force single segment for chunk
    wparams.no_context = true;      // Stateless - no state reuse
    wparams.language = "en";
    
    // Run inference on chunk
    auto start = std::chrono::high_resolution_clock::now();
    int result = whisper_full(g_ctx, wparams, pcm_data, (int)n_samples);
    auto end = std::chrono::high_resolution_clock::now();
    
    double chunk_time = std::chrono::duration<double, std::milli>(end - start).count();
    
    if (result != 0) {
        // Recoverable error: drop chunk, continue session
        printf("[whisper_backend] Chunk inference failed (dropped)\n");
        return WB_OK;  // Continue session
    }
    
    // Extract partial transcript
    std::string partial;
    int n_segments = whisper_full_n_segments(g_ctx);
    for (int i = 0; i < n_segments; i++) {
        const char* text = whisper_full_get_segment_text(g_ctx, i);
        if (text) {
            partial += text;
        }
    }
    
    // Store partial transcript (for final merge)
    if (!partial.empty()) {
        g_session.partial_transcripts.push_back(partial);
        
        // Emit partial via callback
        if (g_session.callback) {
            g_session.callback(partial.c_str(), g_session.user_data);
        }
    }
    
    printf("[whisper_backend] Chunk processed: %.2fms, text: '%s'\n", 
           chunk_time, partial.c_str());
    
    // Transient buffers freed automatically by whisper_full
    
    return WB_OK;
}

WBErrorCode wb_finalize_session(
    uint32_t session_id,
    char* out_text,
    size_t text_size
) {
    std::lock_guard<std::mutex> lock(g_mutex);
    
    if (!g_session.active || g_session.id != session_id) {
        return WB_ERROR_INFERENCE_FAILED;
    }
    
    if (!out_text || text_size == 0) {
        return WB_ERROR_INVALID_AUDIO;
    }
    
    // Calculate session duration
    auto end = std::chrono::high_resolution_clock::now();
    double duration = std::chrono::duration<double, std::milli>(
        end - g_session.start_time).count();
    
    // Merge partial transcripts
    // Strategy: Simple concatenation with space deduplication
    // Phase 2.3: No overlap, so simple join works
    std::string final_text;
    
    for (const auto& partial : g_session.partial_transcripts) {
        if (!partial.empty()) {
            // Avoid leading space duplication
            if (!final_text.empty() && 
                final_text.back() != ' ' && 
                partial.front() != ' ') {
                final_text += ' ';
            }
            final_text += partial;
        }
    }
    
    // Trim leading/trailing whitespace
    size_t start = final_text.find_first_not_of(" \t\n");
    size_t end_pos = final_text.find_last_not_of(" \t\n");
    if (start != std::string::npos && end_pos != std::string::npos) {
        final_text = final_text.substr(start, end_pos - start + 1);
    }
    
    // Copy to output
    strncpy(out_text, final_text.c_str(), text_size - 1);
    out_text[text_size - 1] = '\0';
    
    printf("[whisper_backend] Session %u finalized: %.2fms, %zu partials, final: '%s'\n",
           session_id, duration, g_session.partial_transcripts.size(), 
           final_text.substr(0, 50).c_str());
    
    // Session destroyed
    g_session.active = false;
    g_session.partial_transcripts.clear();
    g_session.callback = nullptr;
    g_session.user_data = nullptr;
    
    return WB_OK;
}

WBErrorCode wb_abort_session(uint32_t session_id) {
    std::lock_guard<std::mutex> lock(g_mutex);
    
    if (!g_session.active || g_session.id != session_id) {
        return WB_OK;  // Already inactive
    }
    
    printf("[whisper_backend] Session %u aborted\n", session_id);
    
    g_session.active = false;
    g_session.partial_transcripts.clear();
    g_session.callback = nullptr;
    g_session.user_data = nullptr;
    
    return WB_OK;
}

