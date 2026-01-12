# SYSTEM_MAP.md — OpenWhispr System Architecture

**Phase 0 Audit Document**
**Date**: 2026-01-09

---

## Execution Flow Summary

```
App Launch → Electron Main Process → Window Creation → Hotkey Registration
                                                              ↓
Hotkey Press → WindowManager.showDictationPanel() → mainWindow.webContents.send("toggle-dictation")
                                                              ↓
React UI (AudioManager.startRecording) → MediaRecorder → audioChunks[]
                                                              ↓
Recording Stop → audioBlob → processAudio()
                                                              ↓
                    ┌─────────────────┴─────────────────┐
                Local Whisper                    OpenAI API
                    ↓                                   ↓
     electronAPI.transcribeLocalWhisper()      fetch(OpenAI /audio/transcriptions)
                    ↓                                   ↓
          ipcMain handler                        HTTP Response
                    ↓                                   ↓
     whisperManager.transcribeLocalWhisper()    result.text
                    ↓
        spawn(python whisper_bridge.py)
                    ↓
               Whisper Model
                    ↓
              Transcription Text
                                                              ↓
                    └──────────────┬──────────────────────────┘
                                   ↓
                         clipboardManager.pasteText()
                                   ↓
                          Active Application
```

**Current transcription is batch per recording, not incremental token streaming.**

---

## Module Inventory

### 1. main.js (Electron Main Process)
- **Location**: `/main.js`
- **Lines of Code**: 242
- **Ownership**: App Layer
- **Rewrite Pressure**: Low
- **Purpose**: Electron application entry point. Orchestrates application startup and manager initialization.
- **Inputs**: None (entry point)
- **Outputs**: BrowserWindow instances, global shortcut registration
- **Dependencies**: 
  - electron (app, globalShortcut, BrowserWindow, dialog)
  - WindowManager, WhisperManager, TrayManager, DatabaseManager, ClipboardManager, IPCHandlers, UpdateManager, GlobeKeyManager
- **Key Operations**:
  - Initializes all manager classes
  - Creates main window and control panel window
  - Sets up tray icon
  - Registers globe key listener (macOS)
  - Handles app lifecycle events

---

### 2. preload.js (Context Bridge)
- **Location**: `/preload.js`
- **Lines of Code**: 174
- **Ownership**: App Layer
- **Rewrite Pressure**: Low
- **Purpose**: Exposes safe IPC API to renderer process via contextBridge.
- **Inputs**: IPC channel calls from renderer
- **Outputs**: electronAPI object with ~60 methods
- **Dependencies**: electron (contextBridge, ipcRenderer)
- **Key Operations**:
  - Exposes transcription functions
  - Exposes window control functions
  - Exposes model management functions
  - Exposes clipboard functions
  - Registers event listeners with cleanup functions

---

### 3. whisper.js (WhisperManager)
- **Location**: `/src/helpers/whisper.js`
- **Lines of Code**: 1352
- **Ownership**: Engine
- **Rewrite Pressure**: High
- **Purpose**: Manages local Whisper transcription via Python subprocess.
- **Inputs**: Audio blob (ArrayBuffer), model name, language options
- **Outputs**: Transcription result {success, text} or error
- **Dependencies**:
  - child_process (spawn)
  - fs, path, crypto, os
  - PythonInstaller
  - debugLogger
- **Key Operations**:
  - Finds Python executable (Windows registry, filesystem, PATH)
  - Creates temporary WAV files
  - Spawns whisper_bridge.py process
  - Locates and validates FFmpeg
  - Parses JSON output from Python
  - Handles model download and status

---

### 4. whisper_bridge.py (Python Transcription Engine)
- **Location**: `/whisper_bridge.py`
- **Lines of Code**: 527
- **Ownership**: Engine / External
- **Rewrite Pressure**: High
- **Purpose**: Python script that interfaces with OpenAI Whisper library.
- **Inputs**: Audio file path, model name, language, CLI arguments
- **Outputs**: JSON to stdout {text, language, success}
- **Dependencies**:
  - Python 3.x
  - openai-whisper library
  - FFmpeg (system or bundled)
- **Key Operations**:
  - Loads Whisper model (with caching)
  - Transcribes audio file
  - Downloads models with progress monitoring
  - Checks FFmpeg availability
  - Manages model cache (~/.cache/whisper)

---

### 5. audioManager.js (Frontend Audio Controller)
- **Location**: `/src/helpers/audioManager.js`
- **Lines of Code**: 628
- **Ownership**: UI
- **Rewrite Pressure**: Low
- **Purpose**: Manages audio recording and transcription flow in renderer process.
- **Inputs**: User recording start/stop commands
- **Outputs**: Transcribed text, state callbacks
- **Dependencies**:
  - navigator.mediaDevices (Web API)
  - MediaRecorder (Web API)
  - ReasoningService
  - window.electronAPI
- **Key Operations**:
  - Captures audio via MediaRecorder
  - Routes to local Whisper or OpenAI API
  - Handles fallback between providers
  - Optimizes audio (16kHz mono conversion)
  - Post-processes with reasoning models (optional)

---

### 6. ipcHandlers.js (IPC Communication Layer)
- **Location**: `/src/helpers/ipcHandlers.js`
- **Lines of Code**: 547
- **Ownership**: App Layer
- **Rewrite Pressure**: Medium
- **Purpose**: Registers all ipcMain handlers for main process.
- **Inputs**: IPC invocations from renderer
- **Outputs**: Varies by handler (results, errors)
- **Dependencies**: All manager classes
- **Key Operations**:
  - Window control (minimize, maximize, close)
  - Transcription handling
  - Model management
  - Hotkey updates
  - Python/Whisper installation
  - API key management

---

### 7. windowManager.js
- **Location**: `/src/helpers/windowManager.js`
- **Lines of Code**: ~300
- **Ownership**: App Layer
- **Rewrite Pressure**: Low
- **Purpose**: Manages Electron BrowserWindow instances.
- **Inputs**: Window creation/control commands
- **Outputs**: BrowserWindow instances
- **Dependencies**: electron, HotkeyManager, DragManager
- **Key Operations**:
  - Creates main dictation window
  - Creates control panel window
  - Manages window visibility
  - Enforces always-on-top for dictation panel

---

### 8. hotkeyManager.js
- **Location**: `/src/helpers/hotkeyManager.js`
- **Lines of Code**: ~200
- **Ownership**: App Layer
- **Rewrite Pressure**: Low
- **Purpose**: Registers and manages global keyboard shortcuts.
- **Inputs**: Hotkey configuration
- **Outputs**: Hotkey events to WindowManager
- **Dependencies**: electron (globalShortcut)
- **Key Operations**:
  - Registers global hotkey
  - Updates hotkey on settings change
  - Unregisters on quit

---

### 9. clipboardManager.js
- **Location**: `/src/helpers/clipboard.js`
- **Lines of Code**: ~500
- **Ownership**: App Layer
- **Rewrite Pressure**: Low
- **Purpose**: Handles clipboard operations and text injection.
- **Inputs**: Text to paste
- **Outputs**: Text injected into active application
- **Dependencies**: electron (clipboard), OS-specific APIs
- **Key Operations**:
  - Preserves clipboard state
  - Writes text to clipboard
  - Triggers paste key event
  - Restores original clipboard

---

### 10. databaseManager.js
- **Location**: `/src/helpers/database.js`
- **Lines of Code**: ~150
- **Ownership**: App Layer
- **Rewrite Pressure**: Low
- **Purpose**: Manages SQLite database for transcription history.
- **Inputs**: Transcription text
- **Outputs**: Saved/retrieved transcriptions
- **Dependencies**: better-sqlite3
- **Key Operations**:
  - Saves transcriptions
  - Retrieves history
  - Clears/deletes entries

---

## Component Boundaries Analysis

| Boundary | Status | Notes |
|----------|--------|-------|
| UI ↔ Backend | **DEFINED** | Clear IPC boundary via preload.js |
| Audio ↔ Transcription | **DEFINED** | ArrayBuffer passed via IPC |
| Node ↔ Python | **SUBPROCESS** | spawn() with JSON communication |
| Whisper ↔ FFmpeg | **IMPLICIT** | FFmpeg resolved at runtime |
| Model ↔ Cache | **FILESYSTEM** | ~/.cache/whisper |

---

## Audio Entry Point
- **Where**: `audioManager.js` line 37
- **How**: `navigator.mediaDevices.getUserMedia({ audio: true })`
- **Format**: WebM audio via MediaRecorder

## Audio Processing Location
- **Where**: `whisperManager.transcribeLocalWhisper()` → `whisper_bridge.py`
- **How**: Python subprocess spawned with temp WAV file path
- **Runtime**: whisper.load_model() → model.transcribe()

## Text Generation Location
- **Where**: `whisper_bridge.py` line 405-414
- **How**: `model.transcribe(audio_path, **options)`
- **Output**: JSON to stdout `{"text": "...", "success": true}`

## Text Injection Location
- **Where**: `clipboardManager.pasteText()`
- **How**: Clipboard write + simulated Cmd/Ctrl+V keystroke
- **Target**: Active foreground application

---

## File Count Summary

| Directory | Files | Total Lines |
|-----------|-------|-------------|
| /src/helpers | 20 | ~5,000 |
| /src/components | 9 + UI lib | ~2,500 |
| /src/services | 4 | ~500 |
| /src/hooks | 11 | ~400 |
| Root JS files | 4 | ~800 |
| Python | 1 | 527 |