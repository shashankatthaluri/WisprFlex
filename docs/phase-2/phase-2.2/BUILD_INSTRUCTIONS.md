# Phase 2.2.1 Build Instructions

## Prerequisites

1. **CMake 3.14+**
   - Windows: Download from https://cmake.org/download/
   - macOS: `brew install cmake`

2. **C++ Compiler**
   - Windows: Visual Studio 2019+ with C++ workload
   - macOS: Xcode Command Line Tools (`xcode-select --install`)

3. **Git** (for submodules)

---

## Step 1: Add whisper.cpp Submodule

```bash
cd wisprflex
git submodule add https://github.com/ggerganov/whisper.cpp engine/native/third_party/whisper.cpp
git submodule update --init --recursive
```

---

## Step 2: Download Base Model

```bash
# Create models directory
mkdir -p ~/.wisprflex/models

# Download base model (ggml format)
cd ~/.wisprflex/models
curl -LO https://huggingface.co/ggerganov/whisper.cpp/resolve/main/ggml-base.bin
```

---

## Step 3: Build

### Windows (MSVC)
```powershell
cd wisprflex/engine/native
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

### macOS
```bash
cd wisprflex/engine/native
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

---

## Step 4: Run Smoke Test

### Windows
```powershell
.\build\Release\whisper_smoke_test.exe $HOME\.wisprflex\models\ggml-base.bin
```

### macOS/Linux
```bash
./build/whisper_smoke_test ~/.wisprflex/models/ggml-base.bin
```

---

## Expected Output

```
========================================
WisprFlex whisper.cpp Smoke Test
Phase 2.2.1 - Proof of Life
========================================

[1/4] Initializing whisper backend...
      OK
[2/4] Loading model: /path/to/ggml-base.bin
      OK (XXX.XX ms)
[3/4] Preparing audio...
      OK (80000 samples)
[4/4] Running transcription...
      OK

========================================
TRANSCRIPTION RESULT:
========================================
(text output here)
========================================

--- Performance Metrics ---
Model load time: XXX.XX ms
Inference time:  XXX.XX ms
---------------------------

[PASS] Smoke test completed successfully
```

---

## Troubleshooting

### "whisper.cpp not found"
Run the submodule commands in Step 1.

### "Model file not found"
Ensure the model path is absolute and the file exists.

### Build errors on Windows
Ensure you're using a "Developer Command Prompt" or have MSVC in PATH.
