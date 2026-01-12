# WisprFlex — Transcription Engine Architecture

**Phase**: 1
**Status**: LOCKED
**Date**: 2026-01-09

---

## 1. Purpose

This document defines the final architecture of the WisprFlex transcription engine.

It answers:

- What components exist
- What each component owns
- How data flows between them
- Where boundaries are enforced
- What is explicitly not allowed

This architecture is designed to:

- Replace Python Whisper entirely
- Enable low-latency dictation
- Enforce memory and performance budgets
- Allow future expansion without rewrites

---

## 2. Architectural Overview

### High-Level Shape

```
┌──────────────────────────────┐
│          App Shell           │
│  (Electron Renderer + Main)  │
│                              │
│  - UI                        │
│  - Hotkeys                   │
│  - Clipboard Injection       │
│  - Settings                  │
└───────────────┬──────────────┘
                │ IPC (thin, typed)
┌───────────────▼──────────────┐
│      Engine Controller       │
│        (Node Layer)          │
│                              │
│  - Session lifecycle         │
│  - Backpressure handling     │
│  - Error translation         │
│  - Thread orchestration      │
└───────────────┬──────────────┘
                │ Native API (N-API / FFI)
┌───────────────▼──────────────┐
│     Transcription Engine     │
│        (Native Core)         │
│                              │
│  - Audio ingestion           │
│  - VAD                       │
│  - Chunking                  │
│  - Decode loop               │
└───────────────┬──────────────┘
                │
┌───────────────▼──────────────┐
│        Model Runtime         │
│    (whisper.cpp backend)     │
│                              │
│  - ggml / gguf models        │
│  - CPU / GPU execution       │
│  - Memory management         │
└──────────────────────────────┘
```

---

## 3. Core Architectural Principles (Non-Negotiable)

**Strict Layering**
- UI → Controller → Engine → Model
- No upward dependencies

**Headless Engine**
- Engine has no knowledge of UI, OS, or Electron

**Deterministic Resource Usage**
- No unbounded memory growth
- Explicit lifecycle control

**Single Responsibility per Layer**
- Each layer does one thing well

**Replaceability**
- App shell can change without engine changes
- Engine can evolve without UI changes

---

## 4. Component Breakdown

### 4.1 App Shell (Existing / Mostly Kept)

**Responsibilities**
- Capture audio (MediaRecorder / OS APIs)
- Handle hotkeys
- Display UI
- Inject text into target application
- Manage user preferences

**Forbidden**
- Any transcription logic
- Any model logic
- Any performance-sensitive work

**Communicates with**
- Engine Controller only

---

### 4.2 Engine Controller (Node Layer)

This is the boundary enforcer.

**Responsibilities**
- Own transcription sessions
- Validate inputs
- Queue and forward audio chunks
- Apply backpressure (drop / delay audio if needed)
- Translate native errors into structured JS errors
- Emit partial/final text events

**Allowed**
- Minimal state
- Lightweight buffering
- Thread coordination

**Forbidden**
- Audio decoding
- ML logic
- Model loading
- Text post-processing

**Why this layer exists**
- Shields UI from native complexity
- Allows future engine swaps
- Centralizes error handling

---

### 4.3 Transcription Engine (Native Core)

This is the performance-critical core.

**Responsibilities**
- Accept raw PCM audio
- Run voice activity detection
- Chunk audio deterministically
- Perform incremental transcription
- Emit partial and final segments

**Characteristics**
- Stateless across sessions (except model)
- Deterministic execution
- No filesystem access (except models)

**Forbidden**
- UI callbacks
- Clipboard access
- Hotkey awareness
- Network access

---

### 4.4 Model Runtime (whisper.cpp)

**Responsibilities**
- Load model into memory
- Execute inference
- Manage CPU/GPU backend
- Control memory footprint

**Rules**
- One model loaded at a time
- Explicit load / unload
- No hidden global caches
- No background downloads

---

## 5. Audio Data Flow (Conceptual)

```
Microphone
   ↓
Audio Capture (UI)
   ↓
PCM Frames
   ↓
Engine Controller
   ↓
Native Engine
   ↓
Chunked Decode
   ↓
Partial Text Events
   ↓
Final Text Event
   ↓
UI decides how to inject
```

Key property:
👉 Audio always flows downward. Text always flows upward.

---

## 6. Concurrency Model

### Threads / Execution Contexts

- **UI thread**: UI only
- **Node main thread**: Session coordination
- **Native worker threads**:
  - Audio processing
  - Whisper inference

### Rules

- No blocking on UI thread
- No blocking IPC calls
- Engine work must be off main thread

### Inter-Thread Communication

Communication between the Node Engine Controller and native worker threads must be:
- Asynchronous
- Non-blocking to the Node event loop

Allowed mechanisms (implementation choice):
- Message passing via N-API thread-safe functions
- Shared memory buffers (e.g., ring buffer) with explicit synchronization

The specific mechanism (postMessage-style vs shared memory) is an implementation detail
and will be selected in Phase 2 based on performance and complexity trade-offs.

Direct blocking calls from Node into native inference threads are forbidden.

---

## 7. Memory Ownership Rules

| Component | Owns Memory |
|-----------|-------------|
| UI | Audio buffers (short-lived) |
| Engine Controller | Small queues only |
| Transcription Engine | Audio chunks + inference buffers |
| Model Runtime | Model weights |

Memory must be:
- Explicitly allocated
- Explicitly freed
- Measurable

---

## 8. Failure Boundaries

Failures must not crash the app.

| Failure | Contained Where |
|---------|-----------------|
| Model load failure | Engine Controller |
| Out-of-memory | Engine Controller |
| Invalid audio | Engine |
| Device not supported | Engine |

Engine reports → Controller translates → UI decides.

---

## 9. Explicit Non-Goals (Phase 1)

This architecture explicitly does not include:

- Token-level streaming
- Multi-model concurrency
- Cloud synchronization
- Plugin systems
- Multi-user profiles

These are future phases.

---

## 10. Architecture Lock Conditions

This architecture is considered LOCKED when:

- ENGINE_API_SPEC.md aligns with it
- STREAMING_STRATEGY.md does not contradict it
- MODEL_MANAGEMENT_SPEC.md fits cleanly
- Performance targets are achievable

Any later change requires explicit architectural revision.

---

## 11. Summary (One Sentence)

WisprFlex's engine is a headless, native, chunk-streaming transcription core, isolated behind a thin controller, designed for low latency, low memory, and zero UI coupling.