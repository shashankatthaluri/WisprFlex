/**
 * WisprFlex Native Engine - Main Implementation
 * 
 * Phase 2.1: Skeleton implementation (no-ops, no whisper.cpp)
 * 
 * From ENGINE_ARCHITECTURE.md:
 * - Section 4.3: Native Core is stateless across sessions
 * - Section 6: Worker threads, non-blocking, async
 * - Section 8: Failures contained, reported to controller
 * 
 * Thread Safety:
 * - All state access protected by g_engine_mutex
 * - Worker thread processes queue asynchronously
 * - No shared mutable globals (state is in g_state)
 */

#include "../include/wisprflex_engine.h"
#include "engine_state.h"

#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <chrono>
#include <sstream>
#include <random>

/* ============================================
 * Global Engine State (single instance)
 * ============================================ */

static std::mutex g_engine_mutex;
static EngineStateData* g_state = nullptr;

/* ============================================
 * Version
 * ============================================ */

static const char* ENGINE_VERSION = "0.1.0";

const char* wf_engine_get_version(void) {
    return ENGINE_VERSION;
}

/* ============================================
 * Error Messages
 * ============================================ */

static const char* ERROR_MESSAGES[] = {
    "Success",                              // WF_OK
    "Engine initialization failed",         // WF_ERROR_INIT_FAILED
    "Device not supported",                 // WF_ERROR_DEVICE_NOT_SUPPORTED
    "Model not found",                      // WF_ERROR_MODEL_NOT_FOUND
    "Model load failed",                    // WF_ERROR_MODEL_LOAD_FAILED
    "Model not loaded",                     // WF_ERROR_MODEL_NOT_LOADED
    "Out of memory",                        // WF_ERROR_OUT_OF_MEMORY
    "Session already active",               // WF_ERROR_SESSION_ALREADY_ACTIVE
    "Invalid session",                      // WF_ERROR_INVALID_SESSION
    "Session ended",                        // WF_ERROR_SESSION_ENDED
    "Backpressure limit reached",           // WF_ERROR_BACKPRESSURE_LIMIT
    "Audio stream error",                   // WF_ERROR_AUDIO_STREAM_ERROR
    "Internal engine error",                // WF_ERROR_INTERNAL
    "Engine already initialized",           // WF_ERROR_ALREADY_INITIALIZED
    "Engine not initialized",               // WF_ERROR_NOT_INITIALIZED
    "Engine disposed"                       // WF_ERROR_DISPOSED
};

const char* wf_engine_error_message(WFErrorCode code) {
    if (code >= 0 && code <= WF_ERROR_DISPOSED) {
        return ERROR_MESSAGES[code];
    }
    return "Unknown error";
}

/* ============================================
 * Logging
 * ============================================ */

static void log_message(int level, const char* message) {
    std::lock_guard<std::mutex> lock(g_engine_mutex);
    if (g_state && level <= g_state->log_level) {
        const char* level_str = (level == 0) ? "ERROR" : (level == 1) ? "WARN" : "INFO";
        printf("[WisprFlex:%s] %s\n", level_str, message);
    }
}

/* ============================================
 * Session ID Generation
 * ============================================ */

static std::string generate_session_id() {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<> dis(0, 35);
    
    auto now = std::chrono::system_clock::now();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()).count();
    
    std::stringstream ss;
    ss << "session_" << ms << "_";
    
    for (int i = 0; i < 9; i++) {
        int r = dis(gen);
        ss << (char)(r < 10 ? '0' + r : 'a' + r - 10);
    }
    
    return ss.str();
}

/* ============================================
 * Worker Thread
 * ============================================ */

static void worker_thread_func() {
    log_message(2, "Worker thread started");
    
    while (true) {
        WorkItem item;
        
        // Wait for work
        {
            std::unique_lock<std::mutex> lock(g_engine_mutex);
            if (!g_state) break;
            
            g_state->queue_cv.wait(lock, [&] {
                return !g_state || 
                       g_state->shutdown_requested || 
                       !g_state->work_queue.empty();
            });
            
            if (!g_state || (g_state->shutdown_requested && g_state->work_queue.empty())) {
                break;
            }
            
            if (!g_state->work_queue.empty()) {
                item = std::move(g_state->work_queue.front());
                g_state->work_queue.pop();
            } else {
                continue;
            }
        }
        
        // Process work item (Phase 2.1: no-ops with simulated delays)
        switch (item.type) {
            case WorkItem::Type::LOAD_MODEL:
                log_message(2, "Worker: Processing LOAD_MODEL (no-op)");
                // In Phase 2.2+, this would load whisper.cpp model
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                break;
                
            case WorkItem::Type::UNLOAD_MODEL:
                log_message(2, "Worker: Processing UNLOAD_MODEL (no-op)");
                break;
                
            case WorkItem::Type::PROCESS_AUDIO:
                log_message(2, "Worker: Processing PROCESS_AUDIO (no-op)");
                // In Phase 2.2+, this would run whisper inference
                break;
                
            case WorkItem::Type::END_SESSION:
                log_message(2, "Worker: Processing END_SESSION (no-op)");
                break;
                
            case WorkItem::Type::SHUTDOWN:
                log_message(2, "Worker: Shutdown requested");
                return;
        }
    }
    
    log_message(2, "Worker thread stopped");
}

/* ============================================
 * Engine Lifecycle API Implementation
 * ============================================ */

WFErrorCode wf_engine_init(const WFEngineConfig* config) {
    std::lock_guard<std::mutex> lock(g_engine_mutex);
    
    // Check if already initialized
    if (g_state && g_state->state != EngineState::DISPOSED) {
        return WF_ERROR_ALREADY_INITIALIZED;
    }
    
    // Validate config
    if (!config) {
        return WF_ERROR_INIT_FAILED;
    }
    
    // Phase 2.1: Only CPU supported
    if (config->device != WF_DEVICE_CPU) {
        return WF_ERROR_DEVICE_NOT_SUPPORTED;
    }
    
    // Create new state
    g_state = new (std::nothrow) EngineStateData();
    if (!g_state) {
        return WF_ERROR_OUT_OF_MEMORY;
    }
    
    // Initialize state
    g_state->state = EngineState::INITIALIZED;
    g_state->device = config->device;
    g_state->log_level = config->log_level;
    g_state->shutdown_requested = false;
    
    // Start worker thread
    g_state->worker_thread = std::thread(worker_thread_func);
    
    log_message(2, "Engine initialized");
    return WF_OK;
}

WFErrorCode wf_engine_set_callback(WFEventCallback callback, void* user_data) {
    std::lock_guard<std::mutex> lock(g_engine_mutex);
    
    if (!g_state || g_state->state == EngineState::DISPOSED) {
        return WF_ERROR_DISPOSED;
    }
    if (g_state->state == EngineState::UNINITIALIZED) {
        return WF_ERROR_NOT_INITIALIZED;
    }
    
    g_state->callback = (void*)callback;
    g_state->callback_user_data = user_data;
    
    return WF_OK;
}

WFErrorCode wf_engine_load_model(const char* model_id) {
    std::lock_guard<std::mutex> lock(g_engine_mutex);
    
    // Validate state
    if (!g_state || g_state->state == EngineState::DISPOSED) {
        return WF_ERROR_DISPOSED;
    }
    if (g_state->state == EngineState::UNINITIALIZED) {
        return WF_ERROR_NOT_INITIALIZED;
    }
    if (g_state->state == EngineState::SESSION_ACTIVE) {
        return WF_ERROR_SESSION_ALREADY_ACTIVE;
    }
    
    // Validate model_id
    if (!model_id || strlen(model_id) == 0) {
        return WF_ERROR_MODEL_NOT_FOUND;
    }
    
    // Check supported models (Phase 2.1: dummy validation)
    const char* supported[] = {"tiny", "base", "small", "medium"};
    bool found = false;
    for (const char* m : supported) {
        if (strcmp(model_id, m) == 0) {
            found = true;
            break;
        }
    }
    if (!found) {
        return WF_ERROR_MODEL_NOT_FOUND;
    }
    
    // Queue load work
    WorkItem item;
    item.type = WorkItem::Type::LOAD_MODEL;
    item.data = model_id;
    g_state->work_queue.push(std::move(item));
    g_state->queue_cv.notify_one();
    
    // Update state synchronously for Phase 2.1
    g_state->loaded_model_id = model_id;
    g_state->state = EngineState::MODEL_LOADED;
    
    log_message(2, "Model load requested");
    return WF_OK;
}

WFErrorCode wf_engine_unload_model(void) {
    std::lock_guard<std::mutex> lock(g_engine_mutex);
    
    if (!g_state || g_state->state == EngineState::DISPOSED) {
        return WF_ERROR_DISPOSED;
    }
    if (g_state->state == EngineState::SESSION_ACTIVE) {
        return WF_ERROR_SESSION_ALREADY_ACTIVE;
    }
    
    if (!g_state->loaded_model_id.empty()) {
        WorkItem item;
        item.type = WorkItem::Type::UNLOAD_MODEL;
        g_state->work_queue.push(std::move(item));
        g_state->queue_cv.notify_one();
        
        g_state->loaded_model_id.clear();
    }
    
    if (g_state->state == EngineState::MODEL_LOADED) {
        g_state->state = EngineState::INITIALIZED;
    }
    
    log_message(2, "Model unloaded");
    return WF_OK;
}

WFErrorCode wf_engine_start_session(
    const WFSessionConfig* config,
    char* session_id_out,
    size_t session_id_size
) {
    std::lock_guard<std::mutex> lock(g_engine_mutex);
    
    // Validate state
    if (!g_state || g_state->state == EngineState::DISPOSED) {
        return WF_ERROR_DISPOSED;
    }
    if (g_state->state == EngineState::UNINITIALIZED) {
        return WF_ERROR_NOT_INITIALIZED;
    }
    if (g_state->loaded_model_id.empty()) {
        return WF_ERROR_MODEL_NOT_LOADED;
    }
    if (!g_state->active_session_id.empty()) {
        return WF_ERROR_SESSION_ALREADY_ACTIVE;
    }
    
    // Validate output buffer
    if (!session_id_out || session_id_size < 64) {
        return WF_ERROR_INTERNAL;
    }
    
    // Generate session ID
    std::string session_id = generate_session_id();
    
    // Copy to output
    strncpy(session_id_out, session_id.c_str(), session_id_size - 1);
    session_id_out[session_id_size - 1] = '\0';
    
    // Update state
    g_state->active_session_id = session_id;
    g_state->session_language = config && config->language ? config->language : "auto";
    g_state->session_vad_enabled = config ? config->vad_enabled : true;
    g_state->chunk_count = 0;
    g_state->state = EngineState::SESSION_ACTIVE;
    
    log_message(2, "Session started");
    return WF_OK;
}

WFErrorCode wf_engine_push_audio(
    const char* session_id,
    const float* pcm_data,
    size_t sample_count
) {
    std::lock_guard<std::mutex> lock(g_engine_mutex);
    
    // Validate state
    if (!g_state || g_state->state == EngineState::DISPOSED) {
        return WF_ERROR_DISPOSED;
    }
    if (g_state->active_session_id.empty()) {
        return WF_ERROR_SESSION_ENDED;
    }
    if (!session_id || g_state->active_session_id != session_id) {
        return WF_ERROR_INVALID_SESSION;
    }
    
    // Validate audio data
    if (!pcm_data || sample_count == 0) {
        return WF_ERROR_AUDIO_STREAM_ERROR;
    }
    
    // Check backpressure (max 10 items in queue)
    if (g_state->work_queue.size() >= 10) {
        return WF_ERROR_BACKPRESSURE_LIMIT;
    }
    
    // Queue audio for processing
    WorkItem item;
    item.type = WorkItem::Type::PROCESS_AUDIO;
    item.data = session_id;
    item.audio.assign(pcm_data, pcm_data + sample_count);
    g_state->work_queue.push(std::move(item));
    g_state->queue_cv.notify_one();
    
    g_state->chunk_count++;
    
    log_message(2, "Audio pushed to queue");
    return WF_OK;
}

WFErrorCode wf_engine_end_session(const char* session_id) {
    std::lock_guard<std::mutex> lock(g_engine_mutex);
    
    // Validate state
    if (!g_state || g_state->state == EngineState::DISPOSED) {
        return WF_ERROR_DISPOSED;
    }
    if (g_state->active_session_id.empty()) {
        return WF_ERROR_SESSION_ENDED;
    }
    if (!session_id || g_state->active_session_id != session_id) {
        return WF_ERROR_INVALID_SESSION;
    }
    
    // Queue end session
    WorkItem item;
    item.type = WorkItem::Type::END_SESSION;
    item.data = session_id;
    g_state->work_queue.push(std::move(item));
    g_state->queue_cv.notify_one();
    
    // Clear session state
    g_state->active_session_id.clear();
    g_state->chunk_count = 0;
    g_state->state = EngineState::MODEL_LOADED;
    
    log_message(2, "Session ended");
    return WF_OK;
}

WFErrorCode wf_engine_dispose(void) {
    std::unique_lock<std::mutex> lock(g_engine_mutex);
    
    if (!g_state) {
        return WF_OK;  // Already disposed, idempotent
    }
    
    if (g_state->state == EngineState::DISPOSED) {
        return WF_OK;
    }
    
    // Signal shutdown
    g_state->shutdown_requested = true;
    
    // Queue shutdown work item
    WorkItem item;
    item.type = WorkItem::Type::SHUTDOWN;
    g_state->work_queue.push(std::move(item));
    g_state->queue_cv.notify_one();
    
    // Release lock before joining thread
    std::thread worker = std::move(g_state->worker_thread);
    lock.unlock();
    
    // Wait for worker thread
    if (worker.joinable()) {
        worker.join();
    }
    
    // Clean up state
    lock.lock();
    g_state->state = EngineState::DISPOSED;
    delete g_state;
    g_state = nullptr;
    
    log_message(2, "Engine disposed");
    return WF_OK;
}

/* ============================================
 * Utility Functions
 * ============================================ */

int wf_engine_is_initialized(void) {
    std::lock_guard<std::mutex> lock(g_engine_mutex);
    return g_state && g_state->state != EngineState::UNINITIALIZED 
                   && g_state->state != EngineState::DISPOSED;
}

const char* wf_engine_get_loaded_model(void) {
    std::lock_guard<std::mutex> lock(g_engine_mutex);
    if (g_state && !g_state->loaded_model_id.empty()) {
        return g_state->loaded_model_id.c_str();
    }
    return nullptr;
}

const char* wf_engine_get_active_session(void) {
    std::lock_guard<std::mutex> lock(g_engine_mutex);
    if (g_state && !g_state->active_session_id.empty()) {
        return g_state->active_session_id.c_str();
    }
    return nullptr;
}
