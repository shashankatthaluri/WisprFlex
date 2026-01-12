# WisprFlex — Phase 1 Control Document

**Phase**: Architecture & Engine Design
**Status**: LOCKED
**Date**: 2026-01-09

---

## 1. Purpose of Phase 1

Phase 1 exists to design and lock the WisprFlex transcription engine before any code is written.

This phase answers one question:

**What exactly are we going to build, and how will all parts talk to each other?**

No implementation happens in this phase.

---

## 2. Phase 1 Objectives (Non-Negotiable)

By the end of Phase 1, we must have:

- A finalized engine architecture
- A locked engine API contract
- A clear streaming strategy
- A safe model management design
- Explicit performance and memory budgets

These decisions must be strong enough that:

- Multiple engineers can implement independently
- No rework is needed later
- UI and engine teams never block each other

---

## 3. In Scope

Phase 1 includes design only for:

- Transcription engine architecture
- whisper.cpp integration strategy
- Engine ↔ App API boundary
- Streaming vs chunked transcription
- Model loading, caching, and lifecycle
- Error handling contracts
- Performance and memory constraints

---

## 4. Out of Scope (Strict)

During Phase 1, the following are not allowed:

- Writing production code
- Modifying wisprflex/
- Adding dependencies
- Touching openwhispr/
- UI redesign
- Feature expansion
- Cloud / auth / pricing logic
- Tauri migration work

Violating these rules invalidates Phase 1.

---

## 5. Phase 1 Deliverables (Required Files)

All Phase 1 work must result in exactly these files:

```
/docs/phase-1/
 ├── README.md                 # This file [LOCKED]
 ├── ENGINE_ARCHITECTURE.md    [LOCKED]
 ├── ENGINE_API_SPEC.md        [LOCKED]
 ├── STREAMING_STRATEGY.md     [LOCKED]
 └── MODEL_MANAGEMENT_SPEC.md  [LOCKED]
```

No additional files unless explicitly approved.

---

## 6. Authority & Decision Rules

**Product Owner**
- Owns product constraints and priorities

**Tech Lead**
- Owns architectural and performance decisions

**Agents**
- Propose designs, do not implement

**QA**
- Validates performance assumptions

If there is a conflict:
- Tech Lead decides architecture
- Product Owner decides scope
- Decision must be written

---

## 7. Performance Targets (Hard Constraints)

These constraints govern all Phase 1 designs:

| Metric | Target |
|--------|--------|
| Engine base memory | <50MB |
| Whisper base model memory | ≤220MB |
| Total active RAM | <400MB |
| Cold engine init | <500ms |
| First partial transcription | <1.5s |
| Chunk processing latency | <300ms |

> **Note (2026-01-11)**: Base model memory revised from 150-200MB to ≤220MB based on Phase 2.2.2 empirical measurements.

If a design cannot meet these, it must be rejected or revised.

---

## 8. Design Principles (Must Follow)

All designs must respect:

- Engine is headless
- UI is thin
- Streaming is chunk-based, not token-based (V1)
- No Python dependencies
- No subprocess-based transcription
- Deterministic memory usage
- Clear failure modes

---

## 9. Phase 1 Execution Order (DO NOT CHANGE)

Design work must happen in this exact order:

1. **ENGINE_ARCHITECTURE.md** → Defines components and boundaries
2. **ENGINE_API_SPEC.md** → Locks how the app talks to the engine
3. **STREAMING_STRATEGY.md** → Defines real-time behavior
4. **MODEL_MANAGEMENT_SPEC.md** → Defines model lifecycle and storage

No later document may contradict an earlier one.

---

## 10. Definition of Done (Phase 1)

Phase 1 is complete only when:

- ✅ All deliverable files exist
- ✅ No "TBD" remains
- ✅ APIs are fully specified
- ✅ Performance budgets are acknowledged
- ✅ Product Owner + Tech Lead sign off

**Status: PHASE 1 LOCKED — Ready for Phase 2 (Implementation)**

---

## 11. Final Rule

If something is unclear, it is not "assumed" — it is written down.

This README governs all Phase 1 work.
If another document conflicts with this one — this file wins.