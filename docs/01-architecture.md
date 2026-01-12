Architecture Overview
High-Level Architecture
┌──────────────────────────┐
│        UI Layer          │
│  (Electron initially)    │
└──────────┬───────────────┘
           │
┌──────────▼──────────┐
│  App Controller      │
│  - State             │
│  - Hotkeys           │
│  - Settings          │
│  - Licensing (later) │
└──────────┬──────────┘
           │
┌──────────▼──────────┐
│ Transcription Engine │
│  - Audio capture     │
│  - VAD               │
│  - Chunking          │
│  - Streaming STT     │
└──────────┬──────────┘
           │
┌──────────▼──────────┐
│ Model Runtime        │
│  - whisper.cpp       │
│  - CPU / GPU         │
└──────────────────────┘

Key Architectural Decisions

UI must be thin

Transcription engine must be headless

Model runtime must be replaceable

No UI logic inside engine

No business logic inside UI

OpenWhispr Relationship

OpenWhispr is used as:

Reference implementation

Bootstrap scaffold

OpenWhispr code will be selectively reused

Python Whisper pipeline is planned for removal

Non-Negotiable Constraints

Offline capability must remain functional

Idle RAM target: <150MB

Active dictation RAM: <500MB

Cold start <2s (target)

MIT-compatible licensing