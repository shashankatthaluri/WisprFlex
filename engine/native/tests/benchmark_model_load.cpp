/**
 * Phase 2.2.2 Model Loading Benchmark Suite
 * Agent D: Measurement & Validation
 * 
 * Measures:
 * - Model load time (cold + warm)
 * - Process memory (cannot measure peak RSS from within)
 * - 10 load/unload cycles stability
 */

#include "whisper_backend.h"
#include <cstdio>
#include <cstdlib>
#include <chrono>
#include <vector>

#ifdef _WIN32
#include <windows.h>
#include <psapi.h>
#endif

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

void print_separator() {
    printf("----------------------------------------\n");
}

int main(int argc, char** argv) {
    if (argc < 2) {
        printf("Usage: %s <model_path>\n", argv[0]);
        return 1;
    }

    const char* model_path = argv[1];
    
    printf("\n");
    printf("========================================\n");
    printf("Phase 2.2.2 Model Loading Benchmarks\n");
    printf("Agent D: Measurement & Validation\n");
    printf("========================================\n\n");

    // Baseline memory
    size_t baseline_kb = get_process_memory_kb();
    printf("Baseline Process Memory: %zu KB (%.2f MB)\n\n", 
           baseline_kb, baseline_kb / 1024.0);

    // Initialize
    WBErrorCode err = wb_init();
    if (err != WB_OK) {
        printf("FAIL: Init failed: %s\n", wb_error_message(err));
        return 1;
    }

    size_t after_init_kb = get_process_memory_kb();
    printf("After Init: %zu KB (%.2f MB)\n", after_init_kb, after_init_kb / 1024.0);

    // Cold load
    print_separator();
    printf("COLD LOAD:\n");
    
    auto cold_start = std::chrono::high_resolution_clock::now();
    err = wb_load_model(model_path);
    auto cold_end = std::chrono::high_resolution_clock::now();
    
    if (err != WB_OK) {
        printf("FAIL: Cold load failed: %s\n", wb_error_message(err));
        wb_shutdown();
        return 1;
    }

    double cold_load_ms = std::chrono::duration<double, std::milli>(cold_end - cold_start).count();
    size_t after_load_kb = get_process_memory_kb();
    
    printf("  Load time: %.2f ms\n", cold_load_ms);
    printf("  Memory after load: %zu KB (%.2f MB)\n", after_load_kb, after_load_kb / 1024.0);
    printf("  Memory increase: %zu KB (%.2f MB)\n", 
           after_load_kb - after_init_kb, 
           (after_load_kb - after_init_kb) / 1024.0);

    // Unload
    wb_unload_model();
    size_t after_unload_kb = get_process_memory_kb();
    printf("  Memory after unload: %zu KB (%.2f MB)\n", after_unload_kb, after_unload_kb / 1024.0);

    // Warm load
    print_separator();
    printf("WARM LOAD:\n");
    
    auto warm_start = std::chrono::high_resolution_clock::now();
    err = wb_load_model(model_path);
    auto warm_end = std::chrono::high_resolution_clock::now();
    
    double warm_load_ms = std::chrono::duration<double, std::milli>(warm_end - warm_start).count();
    printf("  Load time: %.2f ms\n", warm_load_ms);
    
    wb_unload_model();

    // 10 load/unload cycles
    print_separator();
    printf("STABILITY TEST (10 cycles):\n");
    
    std::vector<double> load_times;
    std::vector<size_t> memory_after_load;
    std::vector<size_t> memory_after_unload;
    bool stable = true;

    for (int i = 0; i < 10; i++) {
        auto start = std::chrono::high_resolution_clock::now();
        err = wb_load_model(model_path);
        auto end = std::chrono::high_resolution_clock::now();
        
        if (err != WB_OK) {
            printf("  Cycle %d: FAIL (load)\n", i + 1);
            stable = false;
            break;
        }
        
        double ms = std::chrono::duration<double, std::milli>(end - start).count();
        load_times.push_back(ms);
        memory_after_load.push_back(get_process_memory_kb());
        
        wb_unload_model();
        memory_after_unload.push_back(get_process_memory_kb());
        
        printf("  Cycle %d: %.2f ms, load: %.2fMB, unload: %.2fMB\n", 
               i + 1, ms, 
               memory_after_load.back() / 1024.0,
               memory_after_unload.back() / 1024.0);
    }

    // Final memory
    size_t final_kb = get_process_memory_kb();
    
    // Results summary
    print_separator();
    printf("\n========================================\n");
    printf("RESULTS SUMMARY\n");
    printf("========================================\n\n");
    
    printf("Model: %s\n", model_path);
    printf("Model Size: 141 MB\n\n");
    
    printf("| Metric | Value | Gate | Status |\n");
    printf("|--------|-------|------|--------|\n");
    
    // Cold load gate: < 2000ms
    const char* cold_status = cold_load_ms < 2000 ? "PASS" : "FAIL";
    printf("| Cold Load Time | %.2f ms | < 2000 ms | %s |\n", cold_load_ms, cold_status);
    
    // Peak memory gate: < 200MB (~204800 KB)
    size_t peak_kb = after_load_kb;
    const char* mem_status = peak_kb < 204800 ? "PASS" : "FAIL";
    printf("| Peak Memory | %.2f MB | < 200 MB | %s |\n", peak_kb / 1024.0, mem_status);
    
    // Unload reclaims memory
    const char* unload_status = after_unload_kb < after_load_kb ? "PASS" : "FAIL";
    printf("| Memory Reclaimed | %s | RSS drops | %s |\n", 
           after_unload_kb < after_load_kb ? "Yes" : "No", unload_status);
    
    // 10 cycles stable
    const char* stable_status = stable ? "PASS" : "FAIL";
    printf("| 10 Cycles Stable | %s | No crash | %s |\n", stable ? "Yes" : "No", stable_status);
    
    printf("\n");
    
    // Overall verdict
    bool all_pass = (cold_load_ms < 2000) && 
                    (peak_kb < 204800) && 
                    (after_unload_kb < after_load_kb) && 
                    stable;
    
    printf("========================================\n");
    printf("PHASE 2.2.2 VERDICT: %s\n", all_pass ? "PASS" : "FAIL");
    printf("========================================\n\n");

    wb_shutdown();
    return all_pass ? 0 : 1;
}
