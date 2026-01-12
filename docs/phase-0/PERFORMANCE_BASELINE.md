# PERFORMANCE_BASELINE.md — OpenWhispr Performance Measurements

**Phase 0 Audit Document**
**Date**: 2026-01-09

---

## Test Environment

> **Note**: The following are documented observations and specifications from code analysis. Actual measurements require running the application and will vary by hardware.

### Reference Hardware (from code/docs analysis)
- **Development Platform**: Windows 11 / macOS
- **Python Version Required**: 3.8-3.11

### Performance Targets (from 01-architecture.md)
| Metric | Target |
|--------|--------|
| Idle RAM | <150MB |
| Active Dictation RAM | <500MB |
| Cold Start | <2s |

---

## Code-Derived Performance Estimates (Not Empirical)

> All values in this section are derived from code inspection, known model characteristics, and documented Electron/Python baselines.
> No live runtime profiling was performed in Phase 0.

### 1. Idle RAM Usage

| Component | Estimated RAM |
|-----------|---------------|
| Electron main process | ~80-120MB |
| Electron renderer (React app) | ~50-80MB |
| **Total Idle** | **~130-200MB** |

**Assessment**: At or above 150MB target based on estimated memory footprint

**Source**: Typical Electron app baseline. Actual measurement required.

---

### 2. Active Dictation RAM Usage

| Component | Estimated RAM |
|-----------|---------------|
| Electron processes | ~150MB |
| Python subprocess | ~50MB |
| Whisper model (base) | ~300-400MB |
| Audio buffer | ~5-20MB |
| **Total Active** | **~500-700MB** |

**Assessment**: Above target based on estimated memory footprint

**Notes**:
- Whisper model sizes: tiny (~150MB), base (~300MB), small (~500MB), medium (~1GB), large (~2GB)
- Python runtime adds overhead vs native implementation
- Model remains in memory after first transcription (cached)

---

### 3. Cold Start Time

| Phase | Estimated Duration |
|-------|-------------------|
| Electron startup | ~1-2s |
| React render | ~200-500ms |
| Window display | ~100-200ms |
| **App Visible** | **~1.5-3s** |

| Phase (First Transcription) | Estimated Duration |
|----------------------------|-------------------|
| Python subprocess spawn | ~500ms |
| Whisper model load | ~2-5s |
| **First Transcription Ready** | **~4-8s** |

**Assessment**: App visible within range of target; first transcription above target based on code flow analysis

---

### 4. Disk Footprint

| Component | Size |
|-----------|------|
| Electron app (installed) | ~200-300MB |
| node_modules (dev) | ~500MB |
| Whisper model (base) | 74MB |
| Whisper model (small) | 244MB |
| Whisper model (medium) | 769MB |
| Whisper model (large) | 1.5GB |
| Python runtime (if bundled) | ~100MB |
| FFmpeg binary | ~100MB |
| **Minimum Install** | **~400MB + model** |

---

### 5. CPU Usage During Transcription

| State | CPU Usage (Estimated) |
|-------|----------------------|
| Idle | ~0-2% |
| Recording | ~2-5% |
| Transcribing (CPU) | ~80-100% (single core) |
| Transcribing (GPU) | ~10-30% CPU, GPU varies |

**Notes**:
- Current implementation uses CPU by default
- GPU acceleration available but requires CUDA/Metal setup
- Transcription is blocking (not streaming)

---

## Timing Analysis (Code-Based)

### Timeout Values in Code

| Operation | Timeout |
|-----------|---------|
| Whisper transcription | 120s (whisper.js line 457) |
| Python subprocess spawn | 5s (quick check) |
| FFmpeg check | 5-10s |
| Model download | No timeout (progress monitored) |

### Expected Latencies (from code flow)

| Step | Latency Range |
|------|---------------|
| Hotkey detection | 1-5ms |
| IPC round-trip | 5-20ms |
| Audio recording | User-defined |
| Temp file write | 10-50ms |
| Python spawn (cached) | 50-100ms |
| Python spawn (cold) | 500-1000ms |
| Model load (cached) | 0ms |
| Model load (cold) | 2000-5000ms |
| Transcription (10s audio, base model) | 3-15s |
| Clipboard paste | 50-100ms |

---

## Model Performance Characteristics

| Model | Size | Relative Speed | Accuracy |
|-------|------|----------------|----------|
| tiny | 39MB | 32x faster | Lower |
| base | 74MB | 16x faster | Good |
| small | 244MB | 6x faster | Better |
| medium | 769MB | 2x faster | High |
| large | 1.5GB | 1x (baseline) | Highest |
| turbo | 809MB | ~6x faster | ~Large quality |

**Source**: OpenAI Whisper documentation

---

## Performance Bottlenecks Identified

1. **Python subprocess spawn** — 500ms+ overhead on first call
2. **Whisper model load** — 2-5s on cold start
3. **Batch transcription** — Latency proportional to audio length
4. **Memory footprint** — Python + model exceeds targets
5. **Electron baseline** — ~150MB before any audio processing

---

## Recommended Measurements

For actual performance baseline, measure the following:

| Metric | How to Measure |
|--------|----------------|
| Idle RAM | Task Manager/Activity Monitor after app starts, idle 30s |
| Active RAM | Memory during 10s transcription with base model |
| Cold start | Stopwatch from app launch to window visible |
| First transcription | Time from hotkey to text appearing |
| Subsequent transcription | Same, with model cached |
| Disk usage | File size of installed app directory |

---

## Reference Benchmarks (External, Non-Audited)

> The following table is included for contextual awareness only.
> These values are sourced from external whisper.cpp documentation and were not validated during Phase 0.
> They must not be treated as guarantees.

| Metric | Python Whisper | whisper.cpp (External Reference) |
|--------|----------------|----------------------------------|
| Cold start time | 2-5s | 0.5-1s |
| Model load time | 2-5s | 0.5-2s |
| Memory (base model) | 300-400MB | 150-200MB |
| Transcription speed | 1x | 1.5-2x |
| Binary size | 100MB+ Python | 10-20MB |

**Source**: whisper.cpp documentation (external, not audited)

---

## Platform Variance Notes

| Platform | Expected Difference |
|----------|---------------------|
| Windows | Python detection slower, may need installer |
| macOS (Intel) | Standard performance |
| macOS (Apple Silicon) | Faster transcription (Metal acceleration) |
| Linux | Untested, may have dependency issues |