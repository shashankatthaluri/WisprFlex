# DATA_FLOW.md — Runtime Control & Data Flow

**Phase 0 Audit Document**
**Date**: 2026-01-09

---

## Dictation Flow Diagram

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                           USER PRESSES HOTKEY                               │
└─────────────────────────────────────────────────────────────────────────────┘
                                      │
                                      ▼
┌─────────────────────────────────────────────────────────────────────────────┐
│  [MAIN PROCESS] globalShortcut callback                                     │
│  File: main.js                                                              │
│  ────────────────────────────────────────────────────────────────────────── │
│  hotkeyManager triggers → windowManager.showDictationPanel()                │
│  mainWindow.webContents.send("toggle-dictation")                            │
│  ──────────────────────────────────────────────────────────────────────────  │
│  TYPE: ASYNC (non-blocking IPC send)                                        │
└─────────────────────────────────────────────────────────────────────────────┘
                                      │
                                      │ IPC: "toggle-dictation" event
                                      ▼
┌─────────────────────────────────────────────────────────────────────────────┐
│  [RENDERER PROCESS] React Component / App.jsx                               │
│  File: src/App.jsx                                                          │
│  ────────────────────────────────────────────────────────────────────────── │
│  electronAPI.onToggleDictation(callback)                                    │
│  → audioManager.startRecording() or audioManager.stopRecording()            │
│  ──────────────────────────────────────────────────────────────────────────  │
│  TYPE: ASYNC (event listener)                                               │
└─────────────────────────────────────────────────────────────────────────────┘
                                      │
                                      ▼
┌─────────────────────────────────────────────────────────────────────────────┐
│  [RENDERER] AudioManager.startRecording()                                   │
│  File: src/helpers/audioManager.js (line 31-96)                             │
│  ────────────────────────────────────────────────────────────────────────── │
│  navigator.mediaDevices.getUserMedia({ audio: true })                       │
│  → new MediaRecorder(stream)                                                │
│  → mediaRecorder.start()                                                    │
│  → audioChunks[] populated via ondataavailable                              │
│  ──────────────────────────────────────────────────────────────────────────  │
│  TYPE: ASYNC (Promise + callback)                                           │
│  BLOCKS: NO (recording runs in background)                                  │
└─────────────────────────────────────────────────────────────────────────────┘
                                      │
                                      │ User releases hotkey
                                      ▼
┌─────────────────────────────────────────────────────────────────────────────┐
│  [RENDERER] AudioManager.stopRecording()                                    │
│  File: src/helpers/audioManager.js (line 98-130)                            │
│  ────────────────────────────────────────────────────────────────────────── │
│  mediaRecorder.stop()                                                       │
│  → onstop callback fires                                                    │
│  → new Blob(audioChunks, { type: "audio/wav" })                             │
│  → processAudio(audioBlob)                                                  │
│  ─────────────────────────────────────────────────────────────────────────── │
│  TYPE: ASYNC (event callback)                                               │
└─────────────────────────────────────────────────────────────────────────────┘
                                      │
                                      ▼
┌─────────────────────────────────────────────────────────────────────────────┐
│  [RENDERER] AudioManager.processAudio(audioBlob)                            │
│  File: src/helpers/audioManager.js (line 107-130)                           │
│  ────────────────────────────────────────────────────────────────────────── │
│  if (useLocalWhisper === "true") {                                          │
│      processWithLocalWhisper(audioBlob, model)                              │
│  } else {                                                                   │
│      processWithOpenAIAPI(audioBlob)                                        │
│  }                                                                          │
│  ──────────────────────────────────────────────────────────────────────────  │
│  DECISION POINT: localStorage.getItem("useLocalWhisper")                    │
└─────────────────────────────────────────────────────────────────────────────┘
                                      │
              ┌───────────────────────┴───────────────────────┐
              │                                               │
    LOCAL WHISPER PATH                              OPENAI API PATH
              │                                               │
              ▼                                               ▼
┌─────────────────────────────────────┐   ┌─────────────────────────────────────┐
│  [RENDERER → IPC]                   │   │  [RENDERER] HTTP Request            │
│  processWithLocalWhisper()          │   │  processWithOpenAIAPI()             │
│  ─────────────────────────────────  │   │  ───────────────────────────────── │
│  audioBlob → arrayBuffer            │   │  audioBlob → FormData               │
│  window.electronAPI                 │   │  fetch() to OpenAI API              │
│    .transcribeLocalWhisper(         │   │  /v1/audio/transcriptions           │
│      arrayBuffer, options)          │   │                                     │
│  ─────────────────────────────────  │   │  ───────────────────────────────── │
│  TYPE: ASYNC (IPC invoke)           │   │  TYPE: ASYNC (HTTP)                 │
│  BLOCKS: YES (waiting for IPC)      │   │  BLOCKS: YES (waiting for response) │
└─────────────────────────────────────┘   └─────────────────────────────────────┘
              │                                               │
              ▼                                               │
┌─────────────────────────────────────┐                       │
│  [MAIN PROCESS] ipcHandlers.js      │                       │
│  Handler: "transcribe-local-whisper"│                       │
│  ─────────────────────────────────  │                       │
│  whisperManager                     │                       │
│    .transcribeLocalWhisper(         │                       │
│       audioBlob, options)           │                       │
│  ─────────────────────────────────  │                       │
│  TYPE: ASYNC (Promise)              │                       │
└─────────────────────────────────────┘                       │
              │                                               │
              ▼                                               │
┌─────────────────────────────────────┐                       │
│  [MAIN] WhisperManager              │                       │
│  File: src/helpers/whisper.js       │                       │
│  ─────────────────────────────────  │                       │
│  1. checkFFmpegAvailability()       │                       │
│  2. createTempAudioFile(audioBlob)  │                       │
│     → writes buffer to temp WAV     │                       │
│  3. runWhisperProcess(tempPath,     │                       │
│     model, language)                │                       │
│  ─────────────────────────────────  │                       │
│  TYPE: ASYNC                        │                       │
│  CREATES: Temp file on disk         │                       │
└─────────────────────────────────────┘                       │
              │                                               │
              ▼                                               │
┌─────────────────────────────────────┐                       │
│  [SUBPROCESS] whisper_bridge.py     │                       │
│  ─────────────────────────────────  │                       │
│  spawn(pythonCmd,                   │                       │
│    [whisper_bridge.py,              │                       │
│     tempAudioPath,                  │                       │
│     "--model", model,               │                       │
│     "--output-format", "json"])     │                       │
│  ─────────────────────────────────  │                       │
│  Python loads Whisper model         │                       │
│  model.transcribe(audio_path)       │                       │
│  ─────────────────────────────────  │                       │
│  STDOUT: {"text":"...","success":1} │                       │
│  ─────────────────────────────────  │                       │
│  TYPE: SYNC (Python blocking call)  │                       │
│  BLOCKS: YES (entire transcription) │                       │
│  TIMEOUT: 120 seconds               │                       │
└─────────────────────────────────────┘                       │
              │                                               │
              ▼                                               │
┌─────────────────────────────────────┐                       │
│  [MAIN] parseWhisperResult(stdout)  │                       │
│  File: whisper.js line 528-558      │                       │
│  ─────────────────────────────────  │                       │
│  JSON.parse() the output            │                       │
│  Return {success, text}             │                       │
│  ─────────────────────────────────  │                       │
│  TYPE: SYNC                         │                       │
└─────────────────────────────────────┘                       │
              │                                               │
              └───────────────────────┬───────────────────────┘
                                      │
                                      ▼
┌─────────────────────────────────────────────────────────────────────────────┐
│  [RENDERER] processTranscription(text, source)                              │
│  File: src/helpers/audioManager.js (line 394-456)                           │
│  ────────────────────────────────────────────────────────────────────────── │
│  Optional: ReasoningService.processText() for cleanup                       │
│  → Returns cleaned/processed text                                           │
│  ──────────────────────────────────────────────────────────────────────────  │
│  TYPE: ASYNC (if reasoning enabled)                                         │
└─────────────────────────────────────────────────────────────────────────────┘
                                      │
                                      ▼
┌─────────────────────────────────────────────────────────────────────────────┐
│  [RENDERER] onTranscriptionComplete(result)                                 │
│  ────────────────────────────────────────────────────────────────────────── │
│  Callback to App component                                                  │
│  → window.electronAPI.pasteText(text)                                       │
│  → window.electronAPI.saveTranscription(text)                               │
│  ──────────────────────────────────────────────────────────────────────────  │
│  TYPE: ASYNC (IPC invocations)                                              │
└─────────────────────────────────────────────────────────────────────────────┘
                                      │
                                      ▼
┌─────────────────────────────────────────────────────────────────────────────┐
│  [MAIN PROCESS] clipboardManager.pasteText(text)                            │
│  File: src/helpers/clipboard.js                                             │
│  ────────────────────────────────────────────────────────────────────────── │
│  1. Save current clipboard content                                          │
│  2. clipboard.writeText(text)                                               │
│  3. Simulate Cmd+V / Ctrl+V keypress                                        │
│  4. Restore original clipboard                                              │
│  ──────────────────────────────────────────────────────────────────────────  │
│  TYPE: ASYNC with delays                                                    │
│  OS DEPENDENCY: Native keyboard simulation                                  │
└─────────────────────────────────────────────────────────────────────────────┘
                                      │
                                      ▼
┌─────────────────────────────────────────────────────────────────────────────┐
│                    TEXT APPEARS IN ACTIVE APPLICATION                       │
└─────────────────────────────────────────────────────────────────────────────┘
```

---

## IPC Boundaries

| Channel | Direction | Data Format | Blocking? |
|---------|-----------|-------------|-----------|
| `toggle-dictation` | Main → Renderer | None (event trigger) | No |
| `transcribe-local-whisper` | Renderer → Main | ArrayBuffer + options | Yes |
| `paste-text` | Renderer → Main | String | Yes |
| `db-save-transcription` | Renderer → Main | String | No |
| `hide-window` | Renderer → Main | None | No |
| `show-dictation-panel` | Renderer → Main | None | No |
| `update-hotkey` | Renderer → Main | String (hotkey combo) | No |
| `model-download-progress` | Main → Renderer | {modelId, progress, ...} | No |
| `no-audio-detected` | Main → Renderer | None (event trigger) | No |

---

## Sync vs Async Analysis

| Step | Type | Duration |
|------|------|----------|
| Hotkey detection | Sync | ~1ms |
| IPC to renderer | Async | ~5ms |
| MediaRecorder start | Async | ~50ms |
| Audio recording | Async | User-controlled |
| Blob creation | Sync | ~10ms |
| IPC to main | Async | ~5ms |
| Temp file creation | Async | ~20ms |
| Python subprocess spawn | Async | ~500ms (cold) / ~50ms (warm) |
| Whisper model load | Sync (Python) | 2-5s (cold) / 0ms (cached) |
| Audio transcription | Sync (Python) | 1-30s (varies by audio length) |
| JSON parse | Sync | ~1ms |
| Clipboard write | Sync | ~5ms |
| Keystroke simulation | Async | ~50ms |

---

## Critical Blocking Points

1. **Python Subprocess Spawn**: 500ms+ cold start when Python not cached
2. **Whisper Model Load**: 2-5 seconds on first transcription
3. **Audio Transcription**: Proportional to audio length (batch, not streaming)
4. **FFmpeg Availability Check**: Can add 100-500ms if resolution is complex
5. **Python Whisper execution is single-process and blocks the IPC handler until completion**

---

## Data Formats at Each Stage

| Stage | Format | Size (typical) |
|-------|--------|----------------|
| MediaRecorder output | WebM audio chunks | 10KB-500KB |
| Blob after recording | audio/wav Blob | 50KB-2MB |
| ArrayBuffer for IPC | Uint8Array | Same as Blob |
| Temp file on disk | WAV | Same as Blob |
| Python stdin | None (file path only) | N/A |
| Python stdout | JSON string | ~100 bytes |
| Final text | UTF-8 string | 10-5000 chars |

---

## Transcription Mode Summary

| Mode | Provider | Data Path | Latency Profile |
|------|----------|-----------|-----------------|
| Local | Python Whisper | IPC → Python → Whisper | High initial, moderate after |
| OpenAI API | OpenAI Cloud | HTTP → OpenAI servers | Network-dependent |
| Fallback (Local→API) | Both | Local first, API on error | Variable |
| Fallback (API→Local) | Both | API first, Local on error | Variable |

---

## Transcription: Streaming vs Batch

**Current Implementation**: **BATCH ONLY**

The entire audio recording is captured, then sent for transcription as a single unit. There is no streaming/incremental transcription. The Python Whisper library does not support streaming in the current integration.

Evidence:
- `audioManager.js` collects all chunks then creates a single Blob
- `whisper_bridge.py` receives complete file path, not stream
- `model.transcribe()` is a single blocking call

---

## Rewrite Signals Identified

- Transcription pipeline is batch-only and blocking
- Python subprocess introduces cold start and memory penalties
- Temp file based audio passing adds IO overhead