/**
 * WisprFlex Native Engine - Public C API Header
 * 
 * This header defines the public interface for the WisprFlex transcription engine.
 * Designed for cross-language compatibility (Node.js N-API, other FFI bindings).
 * 
 * Phase 2.1: Skeleton implementation (no-ops, no whisper.cpp)
 * 
 * From ENGINE_ARCHITECTURE.md Section 4.3:
 * - Accept raw PCM audio
 * - Stateless across sessions (except model)
 * - Deterministic execution
 * - No UI callbacks, clipboard, hotkeys, network
 */

#ifndef WISPRFLEX_ENGINE_H
#define WISPRFLEX_ENGINE_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================
 * Version
 * ============================================ */

#define WF_ENGINE_VERSION_MAJOR 0
#define WF_ENGINE_VERSION_MINOR 1
#define WF_ENGINE_VERSION_PATCH 0

/**
 * Get engine version string
 * @return Version string (e.g., "0.1.0")
 */
const char* wf_engine_get_version(void);

/* ============================================
 * Error Codes
 * ============================================ */

typedef enum WFErrorCode {
    WF_OK = 0,
    WF_ERROR_INIT_FAILED = 1,
    WF_ERROR_DEVICE_NOT_SUPPORTED = 2,
    WF_ERROR_MODEL_NOT_FOUND = 3,
    WF_ERROR_MODEL_LOAD_FAILED = 4,
    WF_ERROR_MODEL_NOT_LOADED = 5,
    WF_ERROR_OUT_OF_MEMORY = 6,
    WF_ERROR_SESSION_ALREADY_ACTIVE = 7,
    WF_ERROR_INVALID_SESSION = 8,
    WF_ERROR_SESSION_ENDED = 9,
    WF_ERROR_BACKPRESSURE_LIMIT = 10,
    WF_ERROR_AUDIO_STREAM_ERROR = 11,
    WF_ERROR_INTERNAL = 12,
    WF_ERROR_ALREADY_INITIALIZED = 13,
    WF_ERROR_NOT_INITIALIZED = 14,
    WF_ERROR_DISPOSED = 15
} WFErrorCode;

/**
 * Get error message for error code
 * @param code Error code
 * @return Human-readable error message
 */
const char* wf_engine_error_message(WFErrorCode code);

/* ============================================
 * Configuration Types
 * ============================================ */

typedef enum WFDeviceType {
    WF_DEVICE_CPU = 0,
    WF_DEVICE_GPU = 1
} WFDeviceType;

typedef enum WFLogLevel {
    WF_LOG_ERROR = 0,
    WF_LOG_WARN = 1,
    WF_LOG_INFO = 2
} WFLogLevel;

typedef struct WFEngineConfig {
    WFDeviceType device;
    WFLogLevel log_level;
} WFEngineConfig;

typedef struct WFSessionConfig {
    const char* language;   /* NULL for auto */
    int vad_enabled;        /* 1 = enabled (default), 0 = disabled */
} WFSessionConfig;

/* ============================================
 * Event Types
 * ============================================ */

typedef enum WFEventType {
    WF_EVENT_PARTIAL_TRANSCRIPT = 0,
    WF_EVENT_FINAL_TRANSCRIPT = 1,
    WF_EVENT_ERROR = 2,
    WF_EVENT_MODEL_PROGRESS = 3,
    WF_EVENT_BACKPRESSURE_WARNING = 4
} WFEventType;

typedef struct WFEvent {
    WFEventType type;
    const char* session_id;   /* May be NULL for non-session events */
    
    union {
        struct {
            const char* text;
            int is_stable;
        } partial_transcript;
        
        struct {
            const char* text;
        } final_transcript;
        
        struct {
            WFErrorCode code;
            const char* message;
            int recoverable;
        } error;
        
        struct {
            const char* model_id;
            int progress;   /* 0-100 */
        } model_progress;
        
        struct {
            int dropped_chunks;
        } backpressure_warning;
    } data;
} WFEvent;

/**
 * Event callback function type
 * Called from worker thread - must be thread-safe
 */
typedef void (*WFEventCallback)(const WFEvent* event, void* user_data);

/* ============================================
 * Engine Lifecycle API
 * ============================================ */

/**
 * Initialize the engine runtime
 * Must be called exactly once before any other operations.
 * 
 * @param config Engine configuration
 * @return WF_OK on success, error code on failure
 */
WFErrorCode wf_engine_init(const WFEngineConfig* config);

/**
 * Set event callback
 * Must be called after init, before any operations that emit events.
 * 
 * @param callback Event callback function
 * @param user_data User data passed to callback
 * @return WF_OK on success
 */
WFErrorCode wf_engine_set_callback(WFEventCallback callback, void* user_data);

/**
 * Load a transcription model
 * Only one model loaded at a time - automatically unloads previous.
 * 
 * @param model_id Model identifier (e.g., "base", "small")
 * @return WF_OK on success, error code on failure
 */
WFErrorCode wf_engine_load_model(const char* model_id);

/**
 * Unload the currently loaded model
 * Safe to call even if no model is loaded.
 * 
 * @return WF_OK on success
 */
WFErrorCode wf_engine_unload_model(void);

/**
 * Start a new transcription session
 * Requires model to be loaded. One active session at a time.
 * 
 * @param config Session configuration (may be NULL for defaults)
 * @param session_id_out Buffer to receive session ID (min 64 bytes)
 * @param session_id_size Size of session_id_out buffer
 * @return WF_OK on success, error code on failure
 */
WFErrorCode wf_engine_start_session(
    const WFSessionConfig* config,
    char* session_id_out,
    size_t session_id_size
);

/**
 * Push audio data into an active session
 * Non-blocking. Engine may apply backpressure.
 * 
 * @param session_id Session identifier
 * @param pcm_data PCM Float32 audio data (16kHz, mono)
 * @param sample_count Number of samples in pcm_data
 * @return WF_OK on success, error code on failure
 */
WFErrorCode wf_engine_push_audio(
    const char* session_id,
    const float* pcm_data,
    size_t sample_count
);

/**
 * End a transcription session
 * Flushes remaining buffers and triggers final transcription.
 * 
 * @param session_id Session identifier
 * @return WF_OK on success, error code on failure
 */
WFErrorCode wf_engine_end_session(const char* session_id);

/**
 * Shut down engine and free all resources
 * Idempotent - safe to call multiple times.
 * 
 * @return WF_OK on success
 */
WFErrorCode wf_engine_dispose(void);

/* ============================================
 * Utility Functions
 * ============================================ */

/**
 * Check if engine is initialized
 * @return 1 if initialized, 0 otherwise
 */
int wf_engine_is_initialized(void);

/**
 * Get currently loaded model ID
 * @return Model ID or NULL if no model loaded
 */
const char* wf_engine_get_loaded_model(void);

/**
 * Get active session ID
 * @return Session ID or NULL if no active session
 */
const char* wf_engine_get_active_session(void);

#ifdef __cplusplus
}
#endif

#endif /* WISPRFLEX_ENGINE_H */
