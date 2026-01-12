WisprFlex — Phase 2.2 Control Document
Phase: 2.2 — whisper.cpp Integration
Status: Planning
Date: 2026-01-09

1. Purpose of Phase 2.2

Phase 2.2 exists to integrate whisper.cpp into the native engine and prove:

Native transcription works

Memory stays within budget

Latency targets are achievable

Model loading is deterministic

This phase is about truth, not polish.

2. Scope (Strict)
In Scope

Build whisper.cpp as native dependency

Load gguf models

Single-shot transcription (no streaming yet)

Memory + latency measurement

Error handling validation

Out of Scope

Streaming pipeline

UI changes

Model downloads UX

Performance optimizations

GPU acceleration tuning

3. Phase 2.2 Hard Gates (Non-Negotiable)

If any of these fail, Phase 2.3 does not start.

Gate	Requirement
Build	whisper.cpp builds on macOS + Windows
Load	Base model loads < 2s
Memory	Base model ≤ 220MB RAM
Stability	No crashes across 10 runs
Accuracy	Comparable to Python Whisper (base)

> **Note (2026-01-11)**: Base model memory gate revised from 200 MB to 220 MB based on empirical measurements in Phase 2.2.2. whisper.cpp compute buffers require ~60 MB additional overhead beyond the 147 MB model weights.

4. Integration Strategy (High Level)
Layer Interaction
Node Engine Controller
        ↓
Native Engine Core
        ↓
whisper.cpp Runtime
        ↓
gguf Model


Key rule:

whisper.cpp is never called directly from Node.

5. Phase 2.2 Work Breakdown
🔧 Phase 2.2.1 — whisper.cpp Build Readiness

Goal: Get whisper.cpp compiling cleanly.

Tasks:

Decide build method (submodule vs vendored)

Build static library

Verify:

CPU-only inference

No GPU flags yet

Produce minimal CLI test

Exit:

whisper.cpp builds without hacks

🧠 Phase 2.2.2 — Model Loading Integration

Goal: Load a gguf model inside native engine.

Tasks:

Load base model

Measure:

Load time

Memory usage

Fail fast on OOM

Expose load status to Node

Exit:

loadModel() works with real model

🧪 Phase 2.2.3 — Single-Shot Transcription

Goal: Prove inference correctness.

Tasks:

Feed WAV / PCM buffer

Run one transcription

Return final text only

Compare output vs Python Whisper

Exit:

Transcription is correct and stable

📊 Phase 2.2.4 — Measurement & Validation

Goal: Establish real baseline numbers.

Metrics to capture:

Engine init time

Model load time

Peak RAM

CPU usage

Transcription latency

Exit:

Metrics documented

Pass/fail against gates declared

6. Phase 2.2 Agent Assignment (Preview)
Agent	Focus
Agent B	whisper.cpp build + native integration
Agent C	Memory & threading safety
Agent D	Measurement & validation
Agent A	Node exposure (minimal)

Assignments will be locked after readiness checklist.

7. Phase 2.2 Risks (Known)
Risk	Mitigation
Build complexity	Start CPU-only
Memory blow-up	Base model only
Platform variance	macOS first, Windows next
Accuracy mismatch	Compare against Python
8. Definition of Done (Phase 2.2)

Phase 2.2 is DONE when:

whisper.cpp integrated

Base model loads & transcribes

Performance gates met or failed explicitly

Decision made: Proceed / Adjust / Abort

No optimism allowed.