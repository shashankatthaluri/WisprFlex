/**
 * WisprFlex whisper.cpp Backend - Header
 * 
 * Phase 2.2.1: Thin wrapper over whisper.cpp
 * 
 * Purpose:
 * - Initialize whisper context
 * - Load gguf models
 * - Run single-shot inference
 * - Return transcription text
 * 
 * Constraints:
 * - CPU-only (no GPU flags)
 * - No streaming (Phase 2.3)
 * - No audio chunking
 */

#ifndef WISPRFLEX_WHISPER_BACKEND_H
#define WISPRFLEX_WHISPER_BACKEND_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================
 * Error Codes
 * ============================================ */

typedef enum WBErrorCode {
    WB_OK = 0,
    WB_ERROR_INIT_FAILED = 1,
    WB_ERROR_MODEL_NOT_FOUND = 2,
    WB_ERROR_MODEL_LOAD_FAILED = 3,
    WB_ERROR_OUT_OF_MEMORY = 4,
    WB_ERROR_INFERENCE_FAILED = 5,
    WB_ERROR_INVALID_AUDIO = 6,
    WB_ERROR_NOT_INITIALIZED = 7
} WBErrorCode;

/**
 * Get error message for error code
 */
const char* wb_error_message(WBErrorCode code);

/* ============================================
 * Backend Lifecycle
 * ============================================ */

/**
 * Initialize the whisper backend
 * Must be called before any other operations.
 * 
 * @return WB_OK on success
 */
WBErrorCode wb_init(void);

/**
 * Check if backend is initialized
 */
int wb_is_initialized(void);

/**
 * Shutdown and free all resources
 * Idempotent.
 */
WBErrorCode wb_shutdown(void);

/* ============================================
 * Model Management
 * ============================================ */

/**
 * Load a whisper model from file path
 * 
 * @param model_path Absolute path to .gguf model file
 * @return WB_OK on success
 */
WBErrorCode wb_load_model(const char* model_path);

/**
 * Unload the currently loaded model
 */
WBErrorCode wb_unload_model(void);

/**
 * Check if a model is loaded
 */
int wb_is_model_loaded(void);

/**
 * Get model info (for debugging)
 * 
 * @param out_info Buffer to receive info string
 * @param info_size Size of buffer
 */
WBErrorCode wb_get_model_info(char* out_info, size_t info_size);

/* ============================================
 * Single-Shot Transcription
 * ============================================ */

/**
 * Transcription parameters
 */
typedef struct WBTranscribeParams {
    const char* language;   /* NULL for auto-detect */
    int translate;          /* 1 = translate to English */
    int n_threads;          /* 0 = auto */
} WBTranscribeParams;

/**
 * Default transcription parameters
 */
WBTranscribeParams wb_default_params(void);

/**
 * Run single-shot transcription on PCM audio
 * 
 * @param pcm_data PCM Float32 audio (16kHz, mono)
 * @param n_samples Number of samples
 * @param params Transcription parameters
 * @param out_text Buffer to receive transcription
 * @param text_size Size of output buffer
 * @return WB_OK on success
 */
WBErrorCode wb_transcribe(
    const float* pcm_data,
    size_t n_samples,
    const WBTranscribeParams* params,
    char* out_text,
    size_t text_size
);

/* ============================================
 * Performance Metrics (for validation)
 * ============================================ */

typedef struct WBMetrics {
    double model_load_time_ms;
    double last_inference_time_ms;
    size_t model_memory_bytes;
    size_t peak_memory_bytes;
} WBMetrics;

/**
 * Get current performance metrics
 */
WBMetrics wb_get_metrics(void);

/* ============================================
 * Phase 2.3: Streaming Session APIs
 * ============================================ */

/**
 * Streaming session callback for partial transcripts
 */
typedef void (*WBPartialCallback)(const char* partial_text, void* user_data);

/**
 * Start a new streaming session
 * Only one session active at a time.
 * 
 * @param callback Called for each partial transcript
 * @param user_data Passed to callback
 * @return Session ID (0 on failure)
 */
uint32_t wb_start_session(WBPartialCallback callback, void* user_data);

/**
 * Process an audio chunk in the current session
 * 
 * Chunk size: 800ms = 12800 samples @ 16kHz
 * Each chunk is processed independently (no state reuse).
 * Transient buffers are freed after processing.
 * 
 * @param session_id Session from wb_start_session
 * @param pcm_data PCM Float32 audio (16kHz, mono)
 * @param n_samples Number of samples (should be ~12800 for 800ms)
 * @return WB_OK on success
 */
WBErrorCode wb_process_chunk(
    uint32_t session_id,
    const float* pcm_data,
    size_t n_samples
);

/**
 * Check if audio chunk is silent (energy-based)
 * Used for EOS detection (silence > 700ms)
 * 
 * @param pcm_data PCM Float32 audio
 * @param n_samples Number of samples
 * @return 1 if silent, 0 if speech detected
 */
int wb_is_silent(const float* pcm_data, size_t n_samples);

/**
 * Finalize session and get final transcript
 * Merges all partial transcripts.
 * Exactly one final transcript per session.
 * 
 * @param session_id Session to finalize
 * @param out_text Buffer for final transcript
 * @param text_size Size of buffer
 * @return WB_OK on success
 */
WBErrorCode wb_finalize_session(
    uint32_t session_id,
    char* out_text,
    size_t text_size
);

/**
 * Check if a session is active
 */
int wb_is_session_active(uint32_t session_id);

/**
 * Abort session without finalizing
 * Used for error recovery.
 */
WBErrorCode wb_abort_session(uint32_t session_id);

#ifdef __cplusplus
}
#endif

#endif /* WISPRFLEX_WHISPER_BACKEND_H */
