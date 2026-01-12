# WisprFlex — Streaming & Incremental Transcription Strategy

**Phase**: 1
**Status**: LOCKED
**Date**: 2026-01-09

---

## 1. Purpose

This document defines how audio is processed incrementally and how transcription results are emitted in near real time.

It answers:

- What "streaming" means in WisprFlex V1
- How audio is chunked
- When partial text is emitted
- When text is considered final
- How latency and accuracy are balanced

This strategy is designed to:

- Feel real-time to users
- Stay within performance budgets
- Align with Whisper / whisper.cpp constraints

---

## 2. Definition of "Streaming" (V1)

WisprFlex V1 does NOT implement token-level streaming.

Instead, it uses:

**Chunked pseudo-streaming with incremental partial results**

This means:

- Audio is continuously captured
- Audio is processed in short, fixed-duration chunks
- Transcription results are emitted incrementally
- Earlier partial text may be revised until finalized

This matches Whisper's strengths and avoids unstable behavior.

---

## 3. Audio Chunking Strategy

### 3.1 Audio Format (Input to Engine)

- **Format**: PCM Float32
- **Sample rate**: 16 kHz
- **Channels**: Mono

Conversion (if needed) happens before audio enters the engine.

### 3.2 Chunk Size (Hard Decision)

| Parameter | Value |
|-----------|-------|
| Chunk duration | 1.5 seconds |
| Chunk overlap | 0.3 seconds |
| Effective new audio per chunk | ~1.2 seconds |

**Why 1.5s?**

- Short enough to feel responsive
- Long enough for Whisper to maintain context
- Stable across CPU-only systems

### 3.3 Chunk Overlap Rationale

Overlap exists to:

- Prevent word truncation
- Improve accuracy at boundaries
- Allow smoother partial updates

The engine is responsible for:

- De-duplicating overlapping text
- Emitting clean partial output

---

## 4. Voice Activity Detection (VAD)

### 4.1 VAD Role

VAD is used to:

- Suppress silence
- Detect speech start/end
- Trigger finalization earlier

VAD does not replace chunking — it complements it.

### 4.2 VAD Rules (V1)

- VAD enabled by default
- Silence threshold: configurable (engine-level)
- End-of-speech detected after: **~700 ms continuous silence**

When end-of-speech is detected:

- Engine may finalize the current segment
- Final transcript is emitted earlier

---

## 5. Partial vs Final Transcription

### 5.1 Partial Transcripts

Partial transcripts:

- Are emitted after each chunk
- May change over time
- Are marked `isStable = false`

Rules:

- UI may display partial text optimistically
- UI must expect revisions
- Engine may re-emit updated partial text

### 5.2 Stability Heuristic

A partial transcript becomes stable when:

- It has survived 2 consecutive chunks without change

When this happens:

```
isStable = true
```

Stable partials are unlikely to change further.

### 5.3 Final Transcript

A final transcript is emitted when:

- `engine.endSession()` is called
  OR
- VAD detects end-of-speech

Rules:

- Final transcript is immutable
- Exactly one final transcript per session
- Emitted after all pending chunks are processed

### Session Finalization Rule (V1)

- A transcription session produces exactly one final transcript.
- When VAD detects end-of-speech, the session is finalized.
- Any subsequent speech requires the app to start a new session.

Automatic session restart within a single session is explicitly not supported in V1.

---

## 6. Latency Targets (Streaming-Level)

| Stage | Target |
|-------|--------|
| Audio capture → engine | <50 ms |
| Chunk ready → partial text | <300 ms |
| First partial text | <1.5 s |
| Final text after speech end | <500 ms |

If these targets cannot be met, chunk size must be revisited.

---

## 7. Backpressure & Overload Handling

### 7.1 Backpressure Conditions

Backpressure is triggered when:

- Engine processing lags behind audio input
- Chunk queue exceeds safe depth

### 7.2 Backpressure Policy (V1)

When overloaded:

- Drop oldest non-finalized chunk
- Preserve most recent audio
- Emit warning event (`BACKPRESSURE_LIMIT`)

This prioritizes recency over completeness.

---

## 8. Error Scenarios

| Scenario | Engine Behavior |
|----------|-----------------|
| Audio gap | Skip chunk, continue |
| Corrupt chunk | Drop and report recoverable error |
| Engine overload | Apply backpressure |
| Model error mid-session | Emit error, end session |

Errors are reported via the standard error event.

---

## 9. Explicit Non-Goals (Streaming)

This strategy explicitly excludes:

- Token-level streaming
- Word-level timestamps
- Speaker diarization
- Live punctuation correction
- Continuous dictation across sessions

These are future-phase considerations.

Long-running continuous dictation (>60s) is not optimized in V1.
Sessions are expected to be bounded in duration to ensure stable memory usage.

---

## 10. Validation Criteria

This streaming strategy is considered valid if:

- First partial text appears <1.5s
- Text updates feel continuous to user
- Accuracy is comparable to batch mode
- Memory usage remains within budget

---

## 11. Summary (One Sentence)

WisprFlex V1 delivers "real-time" dictation using overlapping audio chunks with incremental, revisable partial transcripts, optimized for stability and low latency.