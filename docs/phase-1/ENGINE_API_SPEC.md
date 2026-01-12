# WisprFlex — Engine API Specification

**Phase**: 1
**Status**: LOCKED
**Date**: 2026-01-09

---

## 1. Purpose

This document defines the public API contract between:

- WisprFlex App Shell (Electron / Node)
- WisprFlex Transcription Engine (native core)

This API is:

- Stable
- Versioned
- Asynchronous
- Event-driven

Once locked, no breaking changes are allowed without explicit revision.

---

## 2. Design Principles

- Minimal surface area
- Asynchronous by default
- Session-based
- Typed errors
- No UI assumptions
- No global mutable state exposed

---

## 3. Engine Lifecycle API

### 3.1 engine.init(config)

Initializes the engine runtime.

```typescript
engine.init(config: EngineConfig): Promise<void>
```

**Rules**
- Must be called exactly once
- No model loaded yet
- Allocates base engine resources

**Errors**
- `ENGINE_INIT_FAILED`
- `DEVICE_NOT_SUPPORTED`

---

### 3.2 engine.loadModel(modelId)

Loads a transcription model into memory.

```typescript
engine.loadModel(modelId: string): Promise<void>
```

**Rules**
- Only one model loaded at a time
- Automatically unloads previous model
- Blocking until model is ready

**Load behavior**
- Model loading is blocking at the engine level
- Load timeout is fixed and implementation-defined in V1
- If loading exceeds acceptable duration or fails, `MODEL_LOAD_FAILED` is emitted

Model load timeout is not configurable via API in V1.

**Errors**
- `MODEL_NOT_FOUND`
- `MODEL_LOAD_FAILED`
- `OUT_OF_MEMORY`

---

### 3.3 engine.unloadModel()

Unloads the currently loaded model.

```typescript
engine.unloadModel(): Promise<void>
```

**Rules**
- Safe to call even if no model loaded
- Frees model memory

---

### 3.4 engine.dispose()

Shuts down engine and frees all resources.

```typescript
engine.dispose(): Promise<void>
```

**Rules**
- Idempotent
- After dispose, engine cannot be reused

---

## 4. Session API

### 4.1 engine.startSession(sessionConfig)

Creates a new transcription session.

```typescript
engine.startSession(config: SessionConfig): Promise<SessionId>
```

**Rules**
- Requires model to be loaded
- One active session at a time (V1)
- Returns opaque session ID

**Errors**
- `MODEL_NOT_LOADED`
- `SESSION_ALREADY_ACTIVE`

---

### 4.2 engine.pushAudio(sessionId, pcmChunk)

Pushes audio data into an active session.

```typescript
engine.pushAudio(
  sessionId: SessionId,
  pcmChunk: Float32Array
): Promise<void>
```

**Rules**
- Non-blocking
- Engine may apply backpressure
- Chunk duration defined by streaming strategy

**Errors**
- `INVALID_SESSION`
- `SESSION_ENDED`
- `BACKPRESSURE_LIMIT`

---

### 4.3 engine.endSession(sessionId)

Signals end of audio stream.

```typescript
engine.endSession(sessionId: SessionId): Promise<void>
```

**Rules**
- Flushes remaining buffers
- Triggers final transcription

---

## 5. Event API (Engine → App)

Events are emitted via an event emitter interface.

### 5.1 partial_transcript

```typescript
{
  type: "partial_transcript",
  sessionId: SessionId,
  text: string,
  isStable: boolean
}
```

- Emitted incrementally
- `isStable = false` may change later
- UI may display optimistically

---

### 5.2 final_transcript

```typescript
{
  type: "final_transcript",
  sessionId: SessionId,
  text: string
}
```

- Emitted once per session
- Text will not change

---

### 5.3 error

```typescript
{
  type: "error",
  sessionId?: SessionId,
  error: EngineError
}
```

---

### 5.4 model_progress

```typescript
{
  type: "model_progress",
  modelId: string,
  progress: number // 0–100
}
```

---

### 5.5 backpressure_warning

```typescript
{
  type: "backpressure_warning",
  sessionId: SessionId,
  droppedChunks: number
}
```

- Emitted when audio chunks are dropped but session continues
- Allows UI to display warning without terminating session

---

## 6. Error Model

All errors conform to:

```typescript
type EngineError = {
  code: EngineErrorCode
  message: string
  recoverable: boolean
}
```

### Error Codes (Non-Exhaustive)

```
ENGINE_INIT_FAILED
DEVICE_NOT_SUPPORTED
MODEL_NOT_FOUND
MODEL_LOAD_FAILED
MODEL_NOT_LOADED
OUT_OF_MEMORY
SESSION_ALREADY_ACTIVE
INVALID_SESSION
SESSION_ENDED
BACKPRESSURE_LIMIT
AUDIO_STREAM_ERROR
INTERNAL_ENGINE_ERROR
```

### BACKPRESSURE_LIMIT Behavior

`BACKPRESSURE_LIMIT` may be reported in two ways:

- As a warning event when audio chunks are dropped but session continues
- As an error when engine cannot recover and session must terminate

The error object will indicate recoverability via the `recoverable` flag.

---

## 7. Configuration Objects

### 7.1 EngineConfig

```typescript
type EngineConfig = {
  device: "cpu" | "gpu"
  logLevel?: "error" | "warn" | "info"
}
```

### 7.2 SessionConfig

```typescript
type SessionConfig = {
  language?: string // default: auto
  vadEnabled?: boolean // default: true
}
```

---

## 8. Versioning & Compatibility

- API version is semver
- Breaking changes require:
  - Major version bump
  - Migration notes

Engine exposes:

```typescript
engine.getVersion(): string
```

---

## 9. Explicit Non-Goals (API Level)

The API does not expose:

- Model internals
- Token-level streaming
- Confidence scores (V1)
- Word-level timestamps (V1)
- Multi-session concurrency (V1)

---

## 10. Invariants (Must Always Hold)

- Engine never blocks UI thread
- Engine never throws uncaught exceptions
- All memory is released on dispose
- Events are emitted in order per session

---

## 11. API Lock Conditions

This API is considered LOCKED when:

- STREAMING_STRATEGY.md aligns with it
- MODEL_MANAGEMENT_SPEC.md aligns with it
- Product Owner + Tech Lead sign off

After lock:
- Implementation may begin
- Only additive changes allowed in V1

---

## 12. Summary

This API defines a stable, session-based, event-driven contract that cleanly separates WisprFlex's UI from its transcription engine.