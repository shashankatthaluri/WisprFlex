cmake_minimum_required(VERSION 3.14)
project(wisprflex_engine VERSION 0.2.0 LANGUAGES CXX C)

# C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Compiler warnings
if(MSVC)
    add_compile_options(/W4)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# Thread library
find_package(Threads REQUIRED)

# ============================================
# whisper.cpp Integration (Phase 2.2)
# ============================================

# CPU-only flags (LOCKED for Phase 2.2)
set(WHISPER_NO_METAL ON CACHE BOOL "Disable Metal" FORCE)
set(WHISPER_NO_CUDA ON CACHE BOOL "Disable CUDA" FORCE)
set(WHISPER_NO_OPENCL ON CACHE BOOL "Disable OpenCL" FORCE)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build static" FORCE)
set(WHISPER_BUILD_TESTS OFF CACHE BOOL "No whisper tests" FORCE)
set(WHISPER_BUILD_EXAMPLES OFF CACHE BOOL "No whisper examples" FORCE)

# Add whisper.cpp as subdirectory (if exists)
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/third_party/whisper.cpp/CMakeLists.txt")
    message(STATUS "Found whisper.cpp, adding as subdirectory")
    add_subdirectory(third_party/whisper.cpp)
    set(WHISPER_AVAILABLE ON)
else()
    message(WARNING "whisper.cpp not found at third_party/whisper.cpp")
    message(WARNING "Run: git submodule add https://github.com/ggerganov/whisper.cpp third_party/whisper.cpp")
    set(WHISPER_AVAILABLE OFF)
endif()

# ============================================
# whisper_backend Library
# ============================================

if(WHISPER_AVAILABLE)
    add_library(whisper_backend STATIC
        whisper_backend/whisper_backend.cpp
    )

    target_include_directories(whisper_backend
        PUBLIC
            ${CMAKE_CURRENT_SOURCE_DIR}/whisper_backend
        PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/third_party/whisper.cpp
    )

    target_link_libraries(whisper_backend
        PRIVATE
            whisper
            Threads::Threads
    )
endif()

# ============================================
# Engine Library
# ============================================

add_library(wisprflex_engine STATIC
    src/engine.cpp
)

target_include_directories(wisprflex_engine
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(wisprflex_engine
    PRIVATE
        Threads::Threads
)

# Link whisper_backend if available
if(WHISPER_AVAILABLE)
    target_link_libraries(wisprflex_engine
        PRIVATE
            whisper_backend
    )
    target_compile_definitions(wisprflex_engine
        PRIVATE
            WISPRFLEX_HAS_WHISPER=1
    )
endif()

# ============================================
# Test Executables
# ============================================

# Engine skeleton test (Phase 2.1)
add_executable(engine_test
    tests/engine_test.cpp
)

target_link_libraries(engine_test
    PRIVATE
        wisprflex_engine
        Threads::Threads
)

# Whisper smoke test (Phase 2.2)
if(WHISPER_AVAILABLE)
    add_executable(whisper_smoke_test
        tests/whisper_smoke_test.cpp
    )

    target_link_libraries(whisper_smoke_test
        PRIVATE
            whisper_backend
            Threads::Threads
    )

    # Benchmark test (Phase 2.2.2)
    add_executable(benchmark_model_load
        tests/benchmark_model_load.cpp
    )

    target_link_libraries(benchmark_model_load
        PRIVATE
            whisper_backend
            Threads::Threads
    )

    if(WIN32)
        target_link_libraries(benchmark_model_load PRIVATE psapi)
    endif()

    # Transcription test (Phase 2.2.3)
    add_executable(transcription_test
        tests/transcription_test.cpp
    )

    target_link_libraries(transcription_test
        PRIVATE
            whisper_backend
            Threads::Threads
    )

    # Streaming test (Phase 2.3)
    add_executable(streaming_test
        tests/streaming_test.cpp
    )

    target_link_libraries(streaming_test
        PRIVATE
            whisper_backend
            Threads::Threads
    )

    if(WIN32)
        target_link_libraries(streaming_test PRIVATE psapi)
    endif()

    # Streaming test with real audio (Phase 2.3)
    add_executable(streaming_test_real
        tests/streaming_test_real.cpp
    )

    target_link_libraries(streaming_test_real
        PRIVATE
            whisper_backend
            Threads::Threads
    )

    if(WIN32)
        target_link_libraries(streaming_test_real PRIVATE psapi)
    endif()

    # Stability test (Phase 2.5)
    add_executable(stability_test
        tests/stability_test.cpp
    )

    target_link_libraries(stability_test
        PRIVATE
            whisper_backend
            Threads::Threads
    )

    if(WIN32)
        target_link_libraries(stability_test PRIVATE psapi)
    endif()
endif()

# Enable testing
enable_testing()
add_test(NAME engine_test COMMAND engine_test)
if(WHISPER_AVAILABLE)
    add_test(NAME whisper_smoke_test COMMAND whisper_smoke_test)
endif()

# ============================================
# Installation
# ============================================

install(TARGETS wisprflex_engine
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
)

install(FILES include/wisprflex_engine.h
    DESTINATION include
)

if(WHISPER_AVAILABLE)
    install(FILES whisper_backend/whisper_backend.h
        DESTINATION include
    )
endif()

# ============================================
# Configuration Summary
# ============================================

message(STATUS "")
message(STATUS "WisprFlex Engine Configuration:")
message(STATUS "  Version: ${PROJECT_VERSION}")
message(STATUS "  C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "  Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "  whisper.cpp: ${WHISPER_AVAILABLE}")
message(STATUS "")
