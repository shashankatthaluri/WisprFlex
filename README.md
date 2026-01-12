# WisprFlex

> **v0.1.0 — Engine Preview**

WisprFlex is a cross-platform, **offline-first voice-to-text transcription engine** designed for desktop applications that value **privacy, stability, and low system resource usage**.

WisprFlex runs **entirely locally**, with no cloud dependency by default, and is built to be embedded into real desktop apps — not demos.

**Note**: This is an engine library release, not a finished end-user application.

---

## What This Repository Contains

This repository includes:

- ✅ A **native C++ transcription engine** built on `whisper.cpp`
- ✅ A **Node.js bridge layer** for desktop app integration
- ✅ A complete **streaming transcription pipeline**
- ✅ Extensive **architecture, audit, and validation documentation**
- 📦 A future **WisprFlex application layer** (not yet implemented)

This project is open-sourced as **WisprFlex Core (CPU)**.

---

## Project Status

**Current Phase:** ✅ **Phase 2 — COMPLETE (Implementation & Stability)**

Completed milestones:
- Phase 0 — Audit & Decomposition
- Phase 1 — Engine Architecture & API Design
- Phase 2.1–2.5 — Native Engine, Streaming, Integration, Stability Hardening

The engine is **stable, memory-safe, and production-ready** on CPU.

---

## Known Limitations (Important)

- CPU-only inference (no GPU acceleration yet)
- Streaming on CPU is **not real-time** for Whisper base model
- Optimized GPU backends (Metal / CUDA) are planned for a future phase

These limitations are **documented, measured, and intentional**.

---

## Repository Structure
```
wisprflex/  
├── docs/ # Source of truth: architecture, phases, validation
├── engine/ # WisprFlex Core transcription engine (C++ + Node)
├── research/ # Read-only reference implementations
│ └── openwhispr-reference/
├── wisprflex/ # Future application layer (currently empty)
└── README.md # You are here
```

---

## Getting Started

Depending on your role:

- **New contributors** → start with `docs/PUBLIC_OVERVIEW.md`
- **Engine contributors** → see `engine/README.md`
- **Architecture readers** → start with `docs/phase-1/`
- **Auditors / evaluators** → see `PHASE0_EXECUTIVE_BRIEF.md`

---

## Contribution Philosophy

WisprFlex follows a **disciplined, phase-driven development model**.

Contributions are welcome, but:
- Architecture changes require discussion
- Performance claims must be measured
- Stability and correctness are non-negotiable

See `CONTRIBUTING.md` for details.

---

## License

WisprFlex Core is released under the **Apache 2.0 License**.

This allows commercial use, modification, and closed-source extensions, while protecting contributors.

---

## Final Note

WisprFlex is built with a focus on **clarity, correctness, and long-term maintainability**.

If you are looking for hype, this may not be the project for you.  
If you care about building solid systems, welcome.
