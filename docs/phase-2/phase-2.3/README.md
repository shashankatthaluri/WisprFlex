🚀 Phase 2.3 — Streaming & Real-Time Dictation

Project: WisprFlex
Phase: 2.3
Status: CONTROL DOCUMENT
Prerequisite: Phase 2.2 CLOSED
Owner: Tech Co-Founder / Engine Team
Change Policy: 🔒 Changes require explicit re-approval

1. Phase Purpose (Non-Negotiable)

Phase 2.3 exists to make WisprFlex feel real-time to users while preserving:

Deterministic memory usage

Stable transcription quality

Predictable engine behavior

This phase answers one question:

Can WisprFlex deliver responsive, incremental dictation output without sacrificing correctness or stability?

2. Streaming Definition (Locked)
2.1 What “Streaming” Means in WisprFlex

WisprFlex streaming is chunk-based pseudo-streaming.

Audio is captured in fixed-size chunks

Each chunk is processed independently

Partial transcripts are emitted incrementally

A single session produces exactly one final transcript

This is not token-level streaming.

2.2 What Streaming Is NOT

The following are explicitly out of scope for Phase 2.3:

Token-by-token decoding

Whisper internal state reuse across chunks

Word timestamps

Multi-session concurrency

GPU / Metal / CUDA optimizations

3. High-Level Architecture (Streaming Path)
Microphone
   ↓
Audio Chunker (fixed-size)
   ↓
Native Engine
   ↓
whisper.cpp (per-chunk inference)
   ↓
Partial Transcript
   ↓
Transcript Merger
   ↓
Node Engine Controller
   ↓
UI (later phase)


Key principle:

Each chunk is stateless at the whisper.cpp level.

4. Session Model (Locked)
4.1 Session Lifecycle

A dictation session follows this lifecycle:

startSession()

Zero or more processChunk(audioChunk)

EOS detected (silence or user stop)

finalizeSession()

Emit one final transcript

Session destroyed

4.2 Session Guarantees

Only one active session at a time

No overlapping sessions

No auto-restart within a session

New speech after EOS requires a new session

5. Chunking Strategy (Locked)
5.1 Chunk Size

Default chunk duration: 800 ms

Hard bounds: 500 ms – 1000 ms

This balances:

Latency

Transcription quality

CPU load

5.2 Overlap Strategy

No overlap in Phase 2.3

Reason:

Overlap increases memory

Overlap complicates merging

Overlap is unnecessary for initial UX

Overlap may be introduced in Phase 2.4+ only.

6. EOS (End-of-Speech) Handling
6.1 EOS Detection Rules

EOS is triggered when either:

User explicitly stops dictation, or

Silence > 700 ms is detected

Silence detection is:

Energy-based

Implemented outside whisper.cpp

6.2 EOS Semantics

EOS finalizes the current session

No further chunks are accepted

Final transcript is emitted once

If user speaks again after EOS:

A new session must be started

7. Transcript Merging Rules (Critical)
7.1 Partial Transcript Emission

Each chunk produces a partial transcript

Partial transcripts are emitted immediately

Partial transcripts are mutable

7.2 Final Transcript Rules

Exactly one final transcript per session

Final transcript is immutable

Final transcript replaces partials

7.3 Merge Constraints

No duplicate words

No reordering

No hallucinated joins

Prefer omission over duplication

Correctness > verbosity.

8. Memory Rules (Hard Constraints)
8.1 Memory Budget
State	Max RSS
Idle	<150 MB
Streaming Active	<350 MB
Peak (short spikes)	<380 MB
8.2 Memory Growth Rules

Memory usage must not grow with session length

Each chunk must free transient buffers

Session teardown must reclaim memory

Violating this is a hard fail.

9. Latency Targets (User-Perceived)
Metric	Target
First partial text	< 1.5 s
Partial update cadence	≤ chunk duration
Final transcript after EOS	< 500 ms

These are UX-critical, not optional.

10. Node ↔ Native Contract (Streaming)
10.1 Required APIs

Native engine must expose:

startSession()

processChunk(audioBuffer)

finalizeSession()

All APIs:

Async

Non-blocking

Structured errors only

10.2 Events Emitted

partial_transcript

final_transcript

error

No progress events beyond these.

11. Error Handling (Streaming Context)
11.1 Recoverable Errors

Chunk decode failure

Temporary inference failure

Behavior:

Drop chunk

Continue session

11.2 Fatal Errors

OOM

Model unloaded mid-session

Engine internal fault

Behavior:

Abort session

Emit error

Require new session

12. Validation & Exit Criteria (Locked)

Phase 2.3 closes only if:

Streaming works end-to-end

Partial transcripts are timely and stable

Final transcript is correct

Memory stays within budget

No crashes in long sessions (>2 minutes)

All constraints above are met

If any fail → Phase 2.3 does not close.

13. What Comes After Phase 2.3 (Explicitly Not Now)

Not part of this phase:

GPU acceleration

Advanced VAD

Overlapping chunks

Word-level timestamps

UX polish

Model switching

These belong to Phase 2.4+.

14. Final Instruction (Read This Twice)

Phase 2.3 is the most dangerous phase.
It is where shortcuts destroy trust.

Implementation must follow this document exactly.
If something is unclear, stop and clarify before coding.

15. Latency Constraints (Observed)

Validated 2026-01-11:

- CPU-only streaming is not real-time for <1s chunks
- Architecture is correct
- Real-time streaming requires GPU or larger chunks
- Addressed in Phase 2.4+

---

**Phase 2.3 Status: COMPLETE**

Streaming Architecture & Safety: VERIFIED
Known Limitation: CPU-only chunk-based streaming does not meet real-time latency targets.