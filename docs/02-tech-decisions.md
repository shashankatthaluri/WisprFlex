Technology Decisions & Rationale
Desktop Framework

Electron (Phase 1)
Reason:

Faster iteration

Existing OpenWhispr base

Proven at scale (Wispr Flow, Slack, Notion)

Migration to Tauri

Considered post-V1

Only if Electron becomes a bottleneck

Transcription Engine

❌ Python Whisper
✅ whisper.cpp or Rust bindings

Reason:

Smaller binaries

Faster startup

Lower memory usage

Better offline reliability

Offline vs Cloud

Default: Offline-first

Cloud transcription:

Optional

Explicit opt-in

Never required for core functionality

Licensing

Codebase strategy: MIT

Product binaries: proprietary allowed

No AGPL dependencies

Deferred Decisions

Plugin system

Cloud accounts

Team features

Analytics beyond basic telemetry