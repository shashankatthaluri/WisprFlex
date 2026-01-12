WisprFlex — Phase 2 Control Document
Phase: 2 — Implementation
Status: Active
Date: 2026-01-09

1. Purpose of Phase 2

Phase 2 exists to implement the WisprFlex transcription engine exactly as designed in Phase 1.

This phase converts:

Architecture → code

APIs → bindings

Strategy → working system

No new design decisions are allowed unless explicitly approved.

2. Phase 2 Core Principle

Follow the design. Do not reinterpret it.

If something feels unclear:

Stop

Refer to Phase 1 docs

Escalate in writing

Guessing is forbidden.

3. In Scope

Phase 2 includes:

Native engine implementation

whisper.cpp integration

Engine Controller (Node layer)

Streaming pipeline

Model management

Replacing Python Whisper path

Performance hardening

4. Out of Scope (Strict)

During Phase 2, the following are not allowed:

UI redesign

Feature expansion

API changes

Token-level streaming

Multi-session concurrency

Tauri migration

Cloud features

Violations invalidate Phase 2 work.

5. Phase 2 Deliverables

By the end of Phase 2, we must have:

A working native transcription engine

No Python dependency remaining

Feature parity with OpenWhispr (local transcription path)

Streaming dictation working end-to-end

Performance targets met or measured

6. Performance Targets (Hard Gates)
Metric	Target
Idle RAM	<150MB
Active RAM	<400MB
Engine init	<500ms
First partial text	<1.5s
Final transcript latency	<500ms after EOS

If targets are not met:

Phase 2 does not close

Optimization continues

7. Phase 2 Execution Rules

Build in small, verifiable steps

Each sub-phase must compile and run

No “big bang” merges

Replace Python path only after native path is stable

Performance is tested continuously, not at the end

8. Phase 2 Roadmap (Authoritative)
🧱 Phase 2.1 — Engine Skeleton & Repo Setup

Goal: Working scaffold, no ML yet

Tasks

Create wisprflex-engine/ module

Set up build system (CMake / node-gyp / napi-rs)

Implement dummy engine API

Emit fake partial/final events

Wire ENGINE_API_SPEC to Node

Exit Criteria

Engine initializes

Sessions start/end

Events flow correctly

No transcription yet

🔗 Phase 2.2 — whisper.cpp Integration (Single-Shot)

Goal: Prove whisper.cpp works natively

Tasks

Compile whisper.cpp for target platforms

Load gguf model

Run single-shot transcription on WAV

Validate memory usage

Validate accuracy vs Python Whisper

Exit Criteria

One-shot transcription works

Memory within model budget

No crashes

🌊 Phase 2.3 — Streaming Pipeline

Goal: Real-time dictation works

Tasks

PCM chunk ingestion

Chunk overlap handling

Incremental decoding

Partial transcript emission

Stability heuristic (2-chunk rule)

Exit Criteria

Partial text appears <1.5s

Revisions behave correctly

No unbounded memory growth

🔌 Phase 2.4 — App Integration & Python Removal

Goal: Replace old path cleanly

Tasks

Route audioManager → Engine API

Remove Python Whisper code paths

Remove Python installer

Keep OpenAI API fallback intact

Ensure clipboard injection unchanged

Exit Criteria

Local transcription works end-to-end

Python is no longer required

App behavior unchanged from user POV

⚙️ Phase 2.5 — Performance & Stability Hardening

Goal: Production readiness

Tasks

Memory profiling

CPU profiling

Long-session testing

Error injection testing

Graceful failure handling

Exit Criteria

Performance targets met

No crashes under stress

Clean shutdowns

Deterministic memory release

9. Authority & Sign-Off

Tech Lead: owns correctness, performance

Product Owner: owns scope and parity

QA: can block Phase 2 completion

Phase 2 is complete only when all exit criteria are met.

10. Definition of Done (Phase 2)

Phase 2 is DONE when:

Native engine replaces Python fully

Streaming dictation works

Performance targets met or documented

No architectural violations

Phase 3 can start without refactors

11. Final Rule

If Phase 1 told us what to build, Phase 2 is about building it without ego.

This README governs all Phase 2 work.