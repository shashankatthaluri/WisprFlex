/**
 * WisprFlex Native Engine - Internal State Definitions
 * 
 * Internal header - not part of public API.
 */

#ifndef WISPRFLEX_ENGINE_STATE_H
#define WISPRFLEX_ENGINE_STATE_H

#include <mutex>
#include <atomic>
#include <thread>
#include <queue>
#include <condition_variable>
#include <string>
#include <functional>

/**
 * Engine state enum - matches Node layer exactly
 */
enum class EngineState {
    UNINITIALIZED,
    INITIALIZED,
    MODEL_LOADED,
    SESSION_ACTIVE,
    DISPOSED
};

/**
 * Work item for the worker thread queue
 */
struct WorkItem {
    enum class Type {
        LOAD_MODEL,
        UNLOAD_MODEL,
        PROCESS_AUDIO,
        END_SESSION,
        SHUTDOWN
    };
    
    Type type;
    std::string data;           // Model ID or session ID
    std::vector<float> audio;   // Audio data for PROCESS_AUDIO
};

/**
 * Internal engine state - all access protected by mutex
 */
struct EngineStateData {
    // Core state
    EngineState state = EngineState::UNINITIALIZED;
    int device = 0;             // 0 = CPU, 1 = GPU
    int log_level = 0;          // 0 = error, 1 = warn, 2 = info
    
    // Model state
    std::string loaded_model_id;
    
    // Session state
    std::string active_session_id;
    std::string session_language;
    bool session_vad_enabled = true;
    int chunk_count = 0;
    
    // Callback
    void* callback = nullptr;
    void* callback_user_data = nullptr;
    
    // Worker thread
    std::thread worker_thread;
    std::queue<WorkItem> work_queue;
    std::mutex queue_mutex;
    std::condition_variable queue_cv;
    std::atomic<bool> shutdown_requested{false};
};

#endif /* WISPRFLEX_ENGINE_STATE_H */
