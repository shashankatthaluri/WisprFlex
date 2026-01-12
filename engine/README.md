# WisprFlex Engine

The `engine/` directory contains the core WisprFlex transcription library. This is a **library**, not an application.

## Structure

```
engine/
├── native/     # C++ whisper.cpp wrapper
├── node/       # Node.js API layer
└── tests/      # Test suites
```

## Components

### Native Layer (`native/`)

- Wraps [whisper.cpp](https://github.com/ggerganov/whisper.cpp) for inference
- Provides C API for cross-platform use
- Handles model loading, session management, transcription
- **CPU-only** in current release

### Node Layer (`node/`)

- Exposes JavaScript API (`EngineController`)
- Handles async operations and event emission
- See [ENGINE_API_SPEC.md](../docs/phase-1/ENGINE_API_SPEC.md) for full API

## Performance Reality

WisprFlex runs on CPU only. Performance depends on hardware:

| Metric | Typical Value |
|--------|---------------|
| Model load | ~200ms |
| Memory (base) | ~220 MB |
| RTF (streaming) | 1.0–3.0 depending on CPU |

GPU acceleration is planned for Phase 3.

## Building

See `native/CMakeLists.txt` for build configuration.

Requirements:
- CMake 3.16+
- C++17 compiler
- whisper.cpp (git submodule)

## Documentation

- [Engine Architecture](../docs/phase-1/ENGINE_ARCHITECTURE.md)
- [API Specification](../docs/phase-1/ENGINE_API_SPEC.md)
- [Streaming Strategy](../docs/phase-1/STREAMING_STRATEGY.md)
