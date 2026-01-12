# WisprFlex — Model Management Specification

**Phase**: 1
**Status**: LOCKED
**Date**: 2026-01-09

---

## 1. Purpose

This document defines how transcription models are stored, loaded, unloaded, and managed in WisprFlex.

It ensures:

- Predictable memory usage
- Fast startup and switching
- Clear user expectations
- No hidden caches or leaks

Model management is treated as a first-class system, not an implementation detail.

---

## 2. Model Technology Choice

### Model Backend

- whisper.cpp inference engine
- Models in ggml / gguf format

### Explicit Constraints

- No Python-based models
- No Torch runtime
- No dynamic model compilation

---

## 3. Supported Models (V1)

WisprFlex V1 officially supports:

| Model | Approx Size | Target Use |
|-------|-------------|------------|
| tiny | ~39MB | Fast, low-end devices |
| base | ~74MB | Default, balanced |
| small | ~244MB | Higher accuracy |
| medium | ~769MB | Optional (power users) |

`large` is not supported by default in V1 due to memory constraints.

---

## 4. Model Storage Layout

### Directory Location

Models are stored per-user:

```
~/.wisprflex/models/
```

Platform-specific examples:

- macOS: `/Users/<user>/.wisprflex/models/`
- Windows: `C:\Users\<user>\.wisprflex\models\`

### File Structure

```
models/
 ├── base/
 │    ├── model.gguf
 │    └── metadata.json
 ├── small/
 │    ├── model.gguf
 │    └── metadata.json
```

### Metadata Schema (metadata.json)

```json
{
  "modelId": "base",
  "format": "gguf",
  "sizeBytes": 77400000,
  "languageSupport": ["auto", "en", "multi"],
  "checksum": "sha256:...",
  "createdAt": "2026-01-09"
}
```

---

## 5. Model Lifecycle

### 5.1 Download

- Models are downloaded explicitly by user action
- No background or silent downloads
- Progress reported via `model_progress` events

**Rules:**

- Downloads are resumable
- Partial downloads are cleaned up on failure
- Checksum verified before marking usable

**Resumable downloads are implemented via:**

- Partial file persistence on disk
- Resume from last verified byte offset on retry

The specific transport mechanism (e.g., HTTP Range requests) is an implementation detail
and will be defined in Phase 2.

### 5.2 Load

- Exactly one model loaded at a time
- Load is blocking (engine-level)
- Previous model is unloaded before new load

**Rules:**

- Load must fail fast if insufficient memory
- Load time must be <2s for base on target hardware

### 5.3 Unload

Explicit unload frees:

- Model weights
- Inference buffers

- Unload must be deterministic
- Safe to call multiple times

### 5.4 Cache Policy

There is no hidden model cache.

**Rules:**

- Loaded model = in memory
- Unloaded model = not in memory
- No background retention

This guarantees predictable RAM usage.

---

## 6. Memory Budget Enforcement

### Per-Model Memory Targets

| Model | Target RAM |
|-------|------------|
| tiny | ≤120MB |
| base | ≤220MB |
| small | <300MB |
| medium | <500MB |

> **Note (2026-01-11)**: Base model target revised to ≤220MB based on empirical measurements in Phase 2.2.2. whisper.cpp allocates ~60MB compute buffers beyond the 147MB model weights.

If a model exceeds budget:

- Load fails with `OUT_OF_MEMORY`
- UI must prompt user to choose smaller model

---

## 7. Model Switching Rules

- Model switching requires no active session
- Active session must be ended first
- Switching emits lifecycle events: `unload → load → ready`

No hot-swapping during transcription in V1.

---

## 8. Error Handling

### Common Model Errors

| Error Code | Meaning |
|------------|---------|
| MODEL_NOT_FOUND | Model files missing |
| MODEL_CORRUPTED | Checksum mismatch |
| MODEL_LOAD_FAILED | Engine failed to load |
| OUT_OF_MEMORY | Insufficient RAM |
| MODEL_IN_USE | Active session present |

All errors are:

- Structured
- Typed
- Non-fatal to app shell

---

## 9. UI Responsibilities (Explicit)

The UI is responsible for:

- Showing model sizes before download
- Asking for confirmation on large models
- Displaying download progress
- Preventing model switch during active session

The engine does not handle UX decisions.

---

## 10. Explicit Non-Goals (Model Management)

This spec does not include:

- Automatic model upgrades
- Cloud model sync
- Model fine-tuning
- Multi-model loading
- Background preloading

These are future-phase features.

---

## 11. Validation Criteria

Model management is considered correct if:

- Memory usage is predictable
- Models load/unload deterministically
- Switching models never crashes engine
- Corrupt models are detected early
- Disk usage is transparent to user

---

## 12. Summary (One Sentence)

WisprFlex manages transcription models explicitly, deterministically, and transparently, ensuring predictable memory usage and zero hidden behavior.