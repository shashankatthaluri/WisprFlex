# WisprFlex — Public Overview

## What is WisprFlex?

WisprFlex is a **local-first speech-to-text engine** that runs entirely on your machine. It provides:

- **Privacy-first dictation** — No cloud required
- **Native whisper.cpp integration** — Fast, portable inference
- **Clean Node.js API** — Easy integration with apps

## Why WisprFlex Exists

WisprFlex was born from a comprehensive audit of [OpenWhispr](https://github.com/openwhispr/open-whispr), with the goal of building a production-grade, purely local transcription engine.

The key improvements over the reference implementation:

| Aspect | OpenWhispr | WisprFlex |
|--------|------------|-----------|
| Transcription | Python Whisper subprocess | Native whisper.cpp |
| Architecture | Electron-coupled | Library-first |
| Memory | Unpredictable | Bounded & measured |
| Streaming | N/A | Chunk-based |

## Known Limitations

- **CPU-only** — GPU acceleration is planned for Phase 3
- **Base model only** — Model switching is future work
- **RTF > 1.0 on slow CPUs** — Real-time requires faster hardware

## Repository Structure

```
wisprflex/
├── docs/           # All project documentation
├── engine/         # Core transcription library
│   ├── native/     # C++ whisper.cpp wrapper
│   └── node/       # Node.js API layer
├── research/       # Reference materials (read-only)
└── wisprflex/      # Application layer (future)
```

## Where to Start

| Role | Start Here |
|------|------------|
| **Contributors** | [Phase 1 Architecture](phase-1/ENGINE_ARCHITECTURE.md) |
| **Users** | [Engine README](../engine/README.md) |
| **Curious** | [Phase 0 Audit](phase-0/) |

## Phase Status

| Phase | Focus | Status |
|-------|-------|--------|
| 0 | Codebase audit | ✅ Complete |
| 1 | Architecture design | ✅ Complete |
| 2 | Engine implementation | ✅ Complete |
| 3 | GPU & optimization | 📋 Planned |

## License

See [LICENSE](../LICENSE) in the repository root.
