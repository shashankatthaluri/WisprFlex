# RISKS_AND_CONSTRAINTS.md — Technical Risk Assessment

**Phase 0 Audit Document**
**Date**: 2026-01-09

---

## Technical Risks

### 1. Python Dependency Complexity
- **Description**: OpenWhispr requires Python 3.8-3.11 with openai-whisper package. Users must install Python or app must bundle it.
- **Impact**: **HIGH** — Installation friction, support burden, cross-platform complexity
- **Likelihood**: **HIGH** — Already documented as common issue in TROUBLESHOOTING.md
- **Mitigation**: Remove Python-based transcription dependency

### 2. FFmpeg Binary Bundling
- **Description**: ffmpeg-static bundles ~100MB FFmpeg binary for audio conversion
- **Impact**: **MEDIUM** — App size bloat, GPL license concerns
- **Likelihood**: **HIGH** — Currently bundled in every build
- **Mitigation**: Use system FFmpeg or investigate WebM direct support

### 3. Model Download Size
- **Description**: Whisper models range from 74MB (base) to 1.5GB (large)
- **Impact**: **MEDIUM** — Long download times, storage usage
- **Likelihood**: **HIGH** — Users must download at least one model
- **Mitigation**: Default to smaller model, progressive download UI, clear size warnings

### 4. Native Module Rebuilds
- **Description**: better-sqlite3 requires native compilation per platform/arch
- **Impact**: **LOW** — Build complexity, potential ARM64 issues
- **Likelihood**: **MEDIUM** — Standard issue for native Node modules
- **Mitigation**: Use electron-rebuild, proper build matrix

---

## Performance Risks

### 1. Cold Start Latency
- **Description**: Python subprocess spawn + Whisper model load on first transcription
- **Impact**: **HIGH** — 3-7 second delay on first use, poor user experience
- **Likelihood**: **HIGH** — Inherent to current architecture
- **Mitigation**: Preload model at app startup, remove subprocess overhead

### 2. Memory Usage During Transcription
- **Description**: Python process + Whisper model can consume 500MB-2GB RAM
- **Impact**: **HIGH** — Exceeds WisprFlex target of <500MB active
- **Likelihood**: **HIGH** — Measured behavior of current implementation
- **Mitigation**: Remove Python runtime overhead from transcription pipeline

### 3. Batch-Only Transcription
- **Description**: Audio is processed only after recording completes, not streamed
- **Impact**: **MEDIUM** — Latency proportional to recording length
- **Likelihood**: **HIGH** — Current architecture limitation
- **Mitigation**: Investigate streaming transcription options

### 4. Electron Resource Overhead
- **Description**: Electron runtime adds ~150-200MB baseline RAM
- **Impact**: **LOW** — Acceptable for desktop app, conflicts with <150MB idle target
- **Likelihood**: **HIGH** — Inherent to Electron
- **Mitigation**: Evaluate lighter-weight runtime alternatives

---

## Licensing Risks

### 1. FFmpeg GPL License
- **Description**: Bundled FFmpeg is GPL-3.0 licensed. Bundling may trigger copyleft.
- **Impact**: **HIGH** — Potential requirement to open-source entire application
- **Likelihood**: **MEDIUM** — Depends on distribution method and bundling strategy
- **Mitigation**:
  - Option A: Require user to install FFmpeg separately
  - Option B: Use LGPL-licensed FFmpeg build
  - Option C: Investigate audio formats Whisper supports natively

### 2. Whisper Model License
- **Description**: OpenAI Whisper models are MIT licensed
- **Impact**: **NONE** — MIT is permissive
- **Likelihood**: **LOW** — No licensing concerns
- **Mitigation**: None needed

### 3. Torch Dependency License
- **Description**: PyTorch is BSD-3-Clause (via Python Whisper)
- **Impact**: **NONE** — BSD is permissive
- **Likelihood**: **LOW** — No licensing concerns
- **Mitigation**: Eliminated when Python pipeline is removed

---

## Platform Risks

### 1. macOS Accessibility Permissions
- **Description**: Text injection requires Accessibility permission grant
- **Impact**: **HIGH** — Core feature unusable without permission
- **Likelihood**: **HIGH** — Required on every macOS installation
- **Mitigation**: Clear onboarding flow, permission request dialog, troubleshooting docs

### 2. Windows Python PATH Issues
- **Description**: Python often not in PATH on Windows, requires registry lookup
- **Impact**: **MEDIUM** — Installation/detection complexity
- **Likelihood**: **HIGH** — Documented in WINDOWS_TROUBLESHOOTING.md
- **Mitigation**: Remove Python dependency from pipeline

### 3. Globe Key macOS-Only
- **Description**: Globe key hotkey only works on macOS with specific hardware
- **Impact**: **LOW** — Optional feature, fallback hotkeys exist
- **Likelihood**: **LOW** — Feature scope is limited
- **Mitigation**: Document as macOS-only, provide alternative hotkeys

### 4. Text Injection Reliability
- **Description**: Clipboard + keystroke simulation may fail in some apps
- **Impact**: **MEDIUM** — Core feature may not work in all contexts
- **Likelihood**: **MEDIUM** — Known edge cases (secure input fields, VMs)
- **Mitigation**: Document limitations, consider direct typing alternative

---

## Architectural Risks

### 1. Tight UI-Engine Coupling
- **Description**: audioManager.js contains both recording and transcription logic
- **Impact**: **MEDIUM** — Harder to test, replace, or modularize
- **Likelihood**: **HIGH** — Current code structure
- **Mitigation**: Refactor into separate recording and transcription modules

### 2. Subprocess Communication Fragility
- **Description**: Python communication via JSON over stdout is error-prone
- **Impact**: **MEDIUM** — Parsing errors, encoding issues, timeout handling
- **Likelihood**: **MEDIUM** — Edge cases in production
- **Mitigation**: Remove subprocess boundary from critical path

### 3. Settings in localStorage
- **Description**: User settings stored in browser localStorage, not persisted database
- **Impact**: **LOW** — Settings lost on data clear, not synced between windows
- **Likelihood**: **LOW** — Mostly works for single-user desktop app
- **Mitigation**: Migrate critical settings to SQLite database

---

## Unknowns

| Unknown | Impact if True | Investigation Needed |
|---------|----------------|----------------------|
| whisper.cpp Node bindings maturity | May block native migration | Evaluate existing bindings |
| WebM audio direct Whisper support | Eliminates FFmpeg need | Test whisper.cpp with WebM |
| Tauri text injection capability | May block Tauri migration | Test Tauri clipboard + keypress |
| Streaming transcription quality | May require architecture redesign | Benchmark streaming vs batch |
| ARM64 Windows support | Limits user base | Test on ARM64 Windows device |

---

## Risk Priority Matrix

| Risk | Impact | Likelihood | Priority |
|------|--------|------------|----------|
| Python dependency | HIGH | HIGH | **P0** |
| Cold start latency | HIGH | HIGH | **P0** |
| Memory usage | HIGH | HIGH | **P0** |
| FFmpeg GPL license | HIGH | MEDIUM | **P1** |
| macOS Accessibility | HIGH | HIGH | **P1** |
| Batch-only transcription | MEDIUM | HIGH | **P2** |
| Electron overhead | LOW | HIGH | **P3** |
| Text injection reliability | MEDIUM | MEDIUM | **P3** |

**P0** = Must address in Phase 1
**P1** = Should address in Phase 1
**P2** = Address in Phase 2
**P3** = Monitor, address if becomes problem
