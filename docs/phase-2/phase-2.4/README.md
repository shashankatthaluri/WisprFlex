⚙️ WisprFlex — Phase 2.4 README

Phase: 2.4 — Performance Enablement & App Integration
Status: ACTIVE
Strategy: Option B — Larger Chunk (CPU-Compatible)
Authority: Tech Co-Founder
Last Updated: 2026-01-11

1. Purpose of Phase 2.4

Phase 2.4 exists to make WisprFlex usable in the real application by:

Achieving acceptable dictation latency on CPU

Integrating the native engine into the app audio path

Removing Python Whisper completely

Preserving all architectural guarantees from previous phases

This phase does not introduce new architecture.
It operationalizes what already exists.

2. Strategy Chosen (Locked)
✅ Option B — Larger Chunk Strategy (CPU-Compatible)

Chunk duration: 3–5 seconds (default: 4s)

Streaming remains chunk-based

No overlapping chunks

No whisper.cpp state reuse

No GPU acceleration in this phase

Rationale:
CPU-only whisper.cpp cannot handle sub-second chunks in real time.
Larger chunks reduce per-call overhead and enable usable dictation latency.

This is an explicit, intentional tradeoff.

3. What Phase 2.4 Is (and Is Not)
Phase 2.4 IS

Performance enablement using larger chunks

App-level integration of native engine

Removal of Python Whisper path

End-to-end local dictation working

Measured and documented performance

Phase 2.4 IS NOT

True token-level real-time streaming

GPU / Metal / CUDA acceleration

UX redesign or feature expansion

Model switching

Advanced VAD or overlap heuristics

Tauri migration

If it’s not listed as “IS”, assume it is forbidden.

4. Performance Definition (Reframed for Phase 2.4)

In this phase, “real-time” means:

Metric	Target
First partial text	≤ 4 seconds
Partial update cadence	Per chunk (3–5s)
Final transcript latency	< 500 ms after EOS
Active RAM	< 400 MB
Memory growth	None over session

This is a honest, CPU-realistic definition.

5. App Integration Requirements
5.1 Audio Routing

audioManager must send PCM audio directly to EngineController

Chunking must follow Phase 2.4 size rules

Native engine is the default local transcription path

5.2 Python Removal (Hard Requirement)

The following must be fully removed:

whisper_bridge.py

Python detection logic

Python installer

Python-specific error handling

The app must:

Launch

Dictate

Transcribe

Without Python installed.

6. Cloud Fallback (Unchanged)

OpenAI Whisper API fallback remains intact

Selection logic unchanged

Native engine preferred when available

No redesign allowed here.

7. Validation & Evidence (Mandatory)

Phase 2.4 requires measured proof, not assumptions.

Required documentation:

docs/phase-2/phase-2.4/performance_validation.md


Must include:

First partial latency

Chunk cadence timing

Final transcript latency

Memory usage (idle + active)

Long-session test (≥ 5 minutes)

No document = phase not complete.

8. Exit Criteria (Strict)

Phase 2.4 closes only if all are true:

Native engine fully replaces Python path

App-level dictation works end-to-end

Larger chunk strategy implemented

Performance targets met or explicitly documented

No architectural violations

Safety verdict = SAFE

If any fail → Phase 2.4 remains open.

9. Relationship to Other Phases

Phase 2.3 validated correct streaming architecture

Phase 2.4 makes it usable

Phase 2.5 hardens performance and stability

Phase 3 may add GPU acceleration

Do not backport Phase 3 ideas here.

10. Final Rule (Read Carefully)

Phase 2.4 is about shipping something trustworthy on CPU — not chasing perfect real-time.

Clarity > cleverness
Correctness > speed
Measured facts > optimism