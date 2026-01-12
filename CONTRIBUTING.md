# Contributing to WisprFlex

Thank you for your interest in contributing to WisprFlex.

## Before You Start

1. **Read the documentation** — Start with `docs/PUBLIC_OVERVIEW.md`
2. **Understand the phases** — WisprFlex follows strict phase-driven development
3. **Check the issue tracker** — Avoid duplicate work

## What We Accept

- Bug fixes with reproduction steps
- Documentation improvements
- Performance improvements with measured data
- Test coverage improvements

## What We Don't Accept

- Architecture changes without prior discussion
- Features that violate offline-first design
- Performance claims without measurements
- Changes to locked phases (Phase 0, 1, 2)

## How to Contribute

### 1. Fork and Clone

```bash
git clone https://github.com/your-username/wisprflex.git
cd wisprflex
```

### 2. Build the Engine

```bash
cd engine/native
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

### 3. Run Tests

```bash
./build/engine_test
./build/whisper_smoke_test  # Requires model
```

### 4. Submit a Pull Request

- Reference any related issues
- Include test results if applicable
- Keep changes focused and minimal

## Code Style

- C++: Follow existing style in `engine/native/`
- JavaScript: Follow existing style in `engine/node/`
- Documentation: Markdown, concise, factual

## Questions?

Use the Clarification issue template if something is unclear.

---

Thank you for helping make WisprFlex better.
