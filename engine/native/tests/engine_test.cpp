/**
 * WisprFlex Native Engine - Test Suite
 * 
 * Verifies Phase 2.1 exit criteria:
 * - Native library builds successfully
 * - Clean startup and shutdown
 * - No crashes on repeated init/dispose
 * - Thread-safe by design
 */

#include "../include/wisprflex_engine.h"
#include <cstdio>
#include <cstring>
#include <thread>
#include <vector>
#include <atomic>

static int tests_passed = 0;
static int tests_failed = 0;

#define TEST(name) \
    printf("Testing: %s... ", name); \
    fflush(stdout);

#define PASS() \
    printf("PASS\n"); \
    tests_passed++;

#define FAIL(reason) \
    printf("FAIL (%s)\n", reason); \
    tests_failed++;

#define ASSERT(condition, reason) \
    if (!(condition)) { FAIL(reason); return; } 

#define ASSERT_EQ(actual, expected, reason) \
    if ((actual) != (expected)) { FAIL(reason); return; }

/* ============================================
 * Version Test
 * ============================================ */

void test_version() {
    TEST("Engine version")
    const char* version = wf_engine_get_version();
    ASSERT(version != nullptr, "version is null")
    ASSERT(strcmp(version, "0.1.0") == 0, "wrong version")
    PASS()
}

/* ============================================
 * Lifecycle Tests
 * ============================================ */

void test_init_success() {
    TEST("Init with valid config")
    WFEngineConfig config = {WF_DEVICE_CPU, WF_LOG_INFO};
    WFErrorCode result = wf_engine_init(&config);
    ASSERT_EQ(result, WF_OK, "init failed")
    ASSERT(wf_engine_is_initialized(), "not initialized")
    wf_engine_dispose();
    PASS()
}

void test_init_fails_without_config() {
    TEST("Init fails without config")
    WFErrorCode result = wf_engine_init(nullptr);
    ASSERT_EQ(result, WF_ERROR_INIT_FAILED, "should fail")
    PASS()
}

void test_init_fails_with_gpu() {
    TEST("Init fails with GPU (Phase 2.1)")
    WFEngineConfig config = {WF_DEVICE_GPU, WF_LOG_ERROR};
    WFErrorCode result = wf_engine_init(&config);
    ASSERT_EQ(result, WF_ERROR_DEVICE_NOT_SUPPORTED, "should fail with GPU")
    PASS()
}

void test_double_init() {
    TEST("Double init returns error")
    WFEngineConfig config = {WF_DEVICE_CPU, WF_LOG_ERROR};
    wf_engine_init(&config);
    WFErrorCode result = wf_engine_init(&config);
    ASSERT_EQ(result, WF_ERROR_ALREADY_INITIALIZED, "should fail")
    wf_engine_dispose();
    PASS()
}

void test_dispose_idempotent() {
    TEST("Dispose is idempotent")
    WFEngineConfig config = {WF_DEVICE_CPU, WF_LOG_ERROR};
    wf_engine_init(&config);
    WFErrorCode r1 = wf_engine_dispose();
    WFErrorCode r2 = wf_engine_dispose();
    ASSERT_EQ(r1, WF_OK, "first dispose failed")
    ASSERT_EQ(r2, WF_OK, "second dispose failed")
    PASS()
}

void test_repeated_init_dispose() {
    TEST("Repeated init/dispose cycles (10x)")
    WFEngineConfig config = {WF_DEVICE_CPU, WF_LOG_ERROR};
    for (int i = 0; i < 10; i++) {
        WFErrorCode r1 = wf_engine_init(&config);
        if (r1 != WF_OK) { FAIL("init failed"); return; }
        WFErrorCode r2 = wf_engine_dispose();
        if (r2 != WF_OK) { FAIL("dispose failed"); return; }
    }
    PASS()
}

/* ============================================
 * Model Tests
 * ============================================ */

void test_load_model_success() {
    TEST("Load model succeeds")
    WFEngineConfig config = {WF_DEVICE_CPU, WF_LOG_ERROR};
    wf_engine_init(&config);
    WFErrorCode result = wf_engine_load_model("base");
    ASSERT_EQ(result, WF_OK, "load failed")
    const char* loaded = wf_engine_get_loaded_model();
    ASSERT(loaded != nullptr, "no model loaded")
    ASSERT(strcmp(loaded, "base") == 0, "wrong model")
    wf_engine_dispose();
    PASS()
}

void test_load_model_fails_invalid() {
    TEST("Load model fails with invalid model")
    WFEngineConfig config = {WF_DEVICE_CPU, WF_LOG_ERROR};
    wf_engine_init(&config);
    WFErrorCode result = wf_engine_load_model("nonexistent");
    ASSERT_EQ(result, WF_ERROR_MODEL_NOT_FOUND, "should fail")
    wf_engine_dispose();
    PASS()
}

void test_unload_model() {
    TEST("Unload model clears state")
    WFEngineConfig config = {WF_DEVICE_CPU, WF_LOG_ERROR};
    wf_engine_init(&config);
    wf_engine_load_model("base");
    wf_engine_unload_model();
    const char* loaded = wf_engine_get_loaded_model();
    ASSERT(loaded == nullptr, "model still loaded")
    wf_engine_dispose();
    PASS()
}

/* ============================================
 * Session Tests
 * ============================================ */

void test_start_session_success() {
    TEST("Start session succeeds")
    WFEngineConfig config = {WF_DEVICE_CPU, WF_LOG_ERROR};
    wf_engine_init(&config);
    wf_engine_load_model("base");
    
    char session_id[64] = {0};
    WFErrorCode result = wf_engine_start_session(nullptr, session_id, sizeof(session_id));
    ASSERT_EQ(result, WF_OK, "start failed")
    ASSERT(strlen(session_id) > 0, "no session id")
    ASSERT(strncmp(session_id, "session_", 8) == 0, "bad session id format")
    
    wf_engine_dispose();
    PASS()
}

void test_start_session_before_load() {
    TEST("Start session before load fails")
    WFEngineConfig config = {WF_DEVICE_CPU, WF_LOG_ERROR};
    wf_engine_init(&config);
    
    char session_id[64] = {0};
    WFErrorCode result = wf_engine_start_session(nullptr, session_id, sizeof(session_id));
    ASSERT_EQ(result, WF_ERROR_MODEL_NOT_LOADED, "should fail")
    
    wf_engine_dispose();
    PASS()
}

void test_double_session() {
    TEST("Double session returns error")
    WFEngineConfig config = {WF_DEVICE_CPU, WF_LOG_ERROR};
    wf_engine_init(&config);
    wf_engine_load_model("base");
    
    char session_id[64] = {0};
    wf_engine_start_session(nullptr, session_id, sizeof(session_id));
    
    char session_id2[64] = {0};
    WFErrorCode result = wf_engine_start_session(nullptr, session_id2, sizeof(session_id2));
    ASSERT_EQ(result, WF_ERROR_SESSION_ALREADY_ACTIVE, "should fail")
    
    wf_engine_dispose();
    PASS()
}

void test_push_audio() {
    TEST("Push audio succeeds")
    WFEngineConfig config = {WF_DEVICE_CPU, WF_LOG_ERROR};
    wf_engine_init(&config);
    wf_engine_load_model("base");
    
    char session_id[64] = {0};
    wf_engine_start_session(nullptr, session_id, sizeof(session_id));
    
    float audio[1600] = {0};  // 100ms of 16kHz audio
    WFErrorCode result = wf_engine_push_audio(session_id, audio, 1600);
    ASSERT_EQ(result, WF_OK, "push failed")
    
    wf_engine_dispose();
    PASS()
}

void test_push_audio_wrong_session() {
    TEST("Push audio with wrong session fails")
    WFEngineConfig config = {WF_DEVICE_CPU, WF_LOG_ERROR};
    wf_engine_init(&config);
    wf_engine_load_model("base");
    
    char session_id[64] = {0};
    wf_engine_start_session(nullptr, session_id, sizeof(session_id));
    
    float audio[1600] = {0};
    WFErrorCode result = wf_engine_push_audio("wrong_session", audio, 1600);
    ASSERT_EQ(result, WF_ERROR_INVALID_SESSION, "should fail")
    
    wf_engine_dispose();
    PASS()
}

void test_end_session() {
    TEST("End session succeeds")
    WFEngineConfig config = {WF_DEVICE_CPU, WF_LOG_ERROR};
    wf_engine_init(&config);
    wf_engine_load_model("base");
    
    char session_id[64] = {0};
    wf_engine_start_session(nullptr, session_id, sizeof(session_id));
    
    WFErrorCode result = wf_engine_end_session(session_id);
    ASSERT_EQ(result, WF_OK, "end failed")
    
    const char* active = wf_engine_get_active_session();
    ASSERT(active == nullptr, "session still active")
    
    wf_engine_dispose();
    PASS()
}

/* ============================================
 * Thread Safety Test
 * ============================================ */

void test_concurrent_push_audio() {
    TEST("Concurrent push audio (thread safety)")
    WFEngineConfig config = {WF_DEVICE_CPU, WF_LOG_ERROR};
    wf_engine_init(&config);
    wf_engine_load_model("base");
    
    char session_id[64] = {0};
    wf_engine_start_session(nullptr, session_id, sizeof(session_id));
    
    // Launch multiple threads pushing audio
    std::vector<std::thread> threads;
    std::atomic<int> error_count{0};
    
    for (int i = 0; i < 4; i++) {
        threads.emplace_back([&session_id, &error_count]() {
            for (int j = 0; j < 5; j++) {
                float audio[160] = {0};  // 10ms chunks
                WFErrorCode result = wf_engine_push_audio(session_id, audio, 160);
                if (result != WF_OK && result != WF_ERROR_BACKPRESSURE_LIMIT) {
                    error_count++;
                }
            }
        });
    }
    
    for (auto& t : threads) {
        t.join();
    }
    
    ASSERT(error_count == 0, "unexpected errors in threads")
    
    wf_engine_dispose();
    PASS()
}

/* ============================================
 * Main
 * ============================================ */

int main() {
    printf("\n========================================\n");
    printf("WisprFlex Native Engine - Test Suite\n");
    printf("Phase 2.1 Verification\n");
    printf("========================================\n\n");
    
    // Version
    test_version();
    
    // Lifecycle
    test_init_success();
    test_init_fails_without_config();
    test_init_fails_with_gpu();
    test_double_init();
    test_dispose_idempotent();
    test_repeated_init_dispose();
    
    // Model
    test_load_model_success();
    test_load_model_fails_invalid();
    test_unload_model();
    
    // Session
    test_start_session_success();
    test_start_session_before_load();
    test_double_session();
    test_push_audio();
    test_push_audio_wrong_session();
    test_end_session();
    
    // Thread safety
    test_concurrent_push_audio();
    
    printf("\n========================================\n");
    printf("Results: %d passed, %d failed\n", tests_passed, tests_failed);
    printf("========================================\n\n");
    
    return tests_failed > 0 ? 1 : 0;
}
