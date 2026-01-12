# KEEP_DROP_REWRITE.md — Component Classification for WisprFlex

**Phase 0 Audit Document**
**Date**: 2026-01-09

---

## Classification Key

- **KEEP**: Reuse mostly as-is in WisprFlex
- **DROP**: Remove entirely, not needed
- **REWRITE**: Concept needed, implementation must change

---

## Core Engine Components

| Component | Classification | Reason | Risk |
|-----------|---------------|--------|------|
| whisper_bridge.py | **REWRITE** | Python subprocess-based implementation conflicts with memory and startup targets. | High |
| whisper.js | **REWRITE** | Current implementation relies on Python subprocess with temp file passing. | High |
| audioManager.js | **KEEP** | Core audio capture logic is sound and well-isolated from transcription backend. | Low |
| MediaRecorder usage | **KEEP** | Web API is appropriate for audio capture. | Low |

---

## Application Shell

| Component | Classification | Reason | Risk |
|-----------|---------------|--------|------|
| main.js | **KEEP** | Well-structured entry point. | Low |
| preload.js | **KEEP** | Clean IPC abstraction. Pattern works well. | Low |
| windowManager.js | **KEEP** | Window management logic is reusable. | Low |
| hotkeyManager.js | **KEEP** | Hotkey logic is clean. Globe key is macOS-specific. | Low |
| tray.js | **KEEP** | Tray management works. | Low |

---

## Data Layer

| Component | Classification | Reason | Risk |
|-----------|---------------|--------|------|
| database.js | **KEEP** | SQLite for history is appropriate. | Low |
| better-sqlite3 | **KEEP** | Performant, synchronous SQLite. | Low |
| clipboard.js | **KEEP** | Text injection logic is essential and well-implemented. | Low |

---

## UI Components

| Component | Classification | Reason | Risk |
|-----------|---------------|--------|------|
| App.jsx | **REWRITE** | Too much logic embedded in component. | Medium |
| ControlPanel.tsx | **KEEP** | Settings UI is functional. | Low |
| SettingsPage.tsx | **KEEP** | Comprehensive settings. | Low |
| OnboardingFlow.tsx | **KEEP** | Onboarding is valuable for first-run experience. | Low |
| Radix UI components | **KEEP** | Clean, accessible component library. | Low |
| Tailwind CSS | **KEEP** | Styling approach is efficient. | Low |

---

## Reasoning / AI Features

| Component | Classification | Reason | Risk |
|-----------|---------------|--------|------|
| ReasoningService.ts | **DROP** | Out of scope for V1 MVP. Text cleanup not core feature. | Low |
| LocalReasoningService.ts | **DROP** | Out of scope for V1 MVP. | Low |
| AIModelSelectorEnhanced.tsx | **DROP** | AI model selection not needed for V1. | Low |
| llama.cpp integration | **DROP** | Not core to dictation. Adds complexity. | Low |
| Anthropic/Gemini APIs | **DROP** | Not core to dictation. | Low |

---

## Infrastructure

| Component | Classification | Reason | Risk |
|-----------|---------------|--------|------|
| Electron | **KEEP** | Current desktop framework in use. | Low |
| electron-updater | **KEEP** | Auto-updates are essential for desktop app. | Low |
| ffmpeg-static | **REWRITE** | Bundled FFmpeg binary introduces GPL licensing risk in distributed builds. | Medium |
| PythonInstaller | **DROP** | Dependent on Python Whisper pipeline. | Low |
| debugLogger.js | **KEEP** | Useful for development and debugging. | Low |

---

## Model Management

| Component | Classification | Reason | Risk |
|-----------|---------------|--------|------|
| modelManagerBridge.js | **REWRITE** | Current implementation tied to Python Whisper model format. | Medium |
| ModelManager.ts | **REWRITE** | Model download/management logic tied to current pipeline. | Medium |
| Whisper model downloads | **KEEP** | User still needs to download models. | Low |

---

## OpenAI API Integration

| Component | Classification | Reason | Risk |
|-----------|---------------|--------|------|
| OpenAI Whisper API fallback | **KEEP** | Cloud fallback is valuable for edge cases. | Low |
| API key management | **KEEP** | Environment-based key storage works. | Low |
| processWithOpenAIAPI() | **KEEP** | Clean HTTP-based transcription. | Low |

---

## Summary Statistics

| Classification | Count | Notes |
|---------------|-------|-------|
| **KEEP** | 22 | Stable, reusable components |
| **DROP** | 6 | Features out of V1 scope |
| **REWRITE** | 6 | Core engine changes required |

---

## Critical Path Components

The following components are on the critical path for WisprFlex V1:

1. **Transcription engine** (REWRITE) - Current Python-based pipeline
2. **audioManager.js** (KEEP) - Core recording logic
3. **clipboard.js** (KEEP) - Text injection
4. **hotkeyManager.js** (KEEP) - Global hotkey handling
5. **windowManager.js** (KEEP) - UI management

---

## Drop Justification

| Component | Drop Reason |
|-----------|-------------|
| ReasoningService | V1 scope is dictation only, not AI text cleanup |
| LocalReasoningService | Same as above |
| AIModelSelectorEnhanced | Same as above |
| llama.cpp integration | Not core to dictation, adds 200MB+ |
| Anthropic/Gemini APIs | Not core to dictation |
| PythonInstaller | Dependent on Python Whisper pipeline |