# DEPENDENCY_AUDIT.md — OpenWhispr Dependency Analysis

**Phase 0 Audit Document**
**Date**: 2026-01-09

---

## Production Dependencies

> Note: This table includes dependencies bundled into the Electron application at runtime. Some "production" dependencies are not used during active dictation.

| Name | Version | Purpose | Language | Runtime Required | License | Replaceable |
|------|---------|---------|----------|------------------|---------|-------------|
| react | 19.1.0 | UI framework | JavaScript | Yes | MIT | No |
| react-dom | 19.1.0 | React DOM renderer | JavaScript | Yes | MIT | No |
| @radix-ui/react-dialog | 1.1.14 | Modal dialogs | JavaScript | Yes | MIT | Yes |
| @radix-ui/react-dropdown-menu | 2.1.15 | Dropdown menus | JavaScript | Yes | MIT | Yes |
| @radix-ui/react-label | 2.1.7 | Form labels | JavaScript | Yes | MIT | Yes |
| @radix-ui/react-progress | 1.1.7 | Progress bars | JavaScript | Yes | MIT | Yes |
| @radix-ui/react-select | 2.2.5 | Select dropdowns | JavaScript | Yes | MIT | Yes |
| @radix-ui/react-slot | 1.2.3 | Slot component | JavaScript | Yes | MIT | Yes |
| @radix-ui/react-tabs | 1.1.12 | Tab components | JavaScript | Yes | MIT | Yes |
| better-sqlite3 | 11.10.0 | SQLite database | JavaScript (Native) | Yes | MIT | Yes |
| class-variance-authority | 0.7.1 | CSS class variants | JavaScript | Yes | Apache-2.0 | Yes |
| clsx | 2.1.1 | Class name utility | JavaScript | Yes | MIT | Yes |
| dotenv | 16.3.1 | Environment variables | JavaScript | Yes | BSD-2-Clause | Yes |
| electron-updater | 6.6.2 | Auto-updates | JavaScript | Yes | MIT | No |
| ffmpeg-static | 5.2.0 | FFmpeg binary | Binary | Yes | GPL-3.0* | Yes |
| lucide-react | 0.518.0 | Icons | JavaScript | Yes | ISC | Yes |
| shadcn-ui | 0.9.5 | UI components | JavaScript | Yes | MIT | Yes |
| tailwind-merge | 3.3.1 | Tailwind class merge | JavaScript | Yes | MIT | Yes |
| tar | 7.4.3 | Archive handling | JavaScript | Yes | ISC | Yes |
| tw-animate-css | 1.3.4 | Tailwind animations | JavaScript | Yes | MIT | Yes |
| unzipper | 0.12.3 | ZIP extraction | JavaScript | Yes | MIT | Yes |

---

## Development Dependencies

| Name | Version | Purpose | Runtime Required | License |
|------|---------|---------|------------------|---------|
| electron | 36.5.0 | Desktop framework | Yes (bundled)* | MIT |
| electron-builder | 24.6.4 | Build/packaging | No | MIT |
| @electron/notarize | 3.0.1 | macOS notarization | No | MIT |
| vite | 6.3.5 | Build tool | No | MIT |
| @vitejs/plugin-react | 4.4.1 | Vite React plugin | No | MIT |
| tailwindcss | 4.1.10 | CSS framework | No (compiled) | MIT |
| @tailwindcss/vite | 4.1.10 | Tailwind Vite plugin | No | MIT |
| postcss | 8.5.6 | CSS processing | No | MIT |
| autoprefixer | 10.4.21 | CSS autoprefixer | No | MIT |
| eslint | 9.25.0 | Linting | No | MIT |
| @eslint/js | 9.25.0 | ESLint config | No | MIT |
| eslint-plugin-react-hooks | 5.2.0 | React hooks linting | No | MIT |
| eslint-plugin-react-refresh | 0.4.19 | Hot reload linting | No | MIT |
| globals | 16.0.0 | Global variables | No | MIT |
| concurrently | 8.2.2 | Parallel commands | No | MIT |
| cross-env | 10.0.0 | Cross-platform env | No | MIT |
| @types/react | 19.1.2 | TypeScript types | No | MIT |
| @types/react-dom | 19.1.2 | TypeScript types | No | MIT |

*Electron is bundled into production builds but not required as a separate runtime dependency during development.

---

## Python Dependencies (whisper_bridge.py)

| Name | Purpose | Runtime Required | License | Replaceable |
|------|---------|------------------|---------|-------------|
| openai-whisper | Speech-to-text model | Yes | MIT | Yes |
| torch | ML framework (Whisper dep) | Yes | BSD-3-Clause | No (required by Whisper) |
| numpy | Numerical computing | Yes | BSD-3-Clause | No (required by Whisper) |
| ffmpeg | Audio processing | Yes (system/bundled) | GPL-3.0* | Yes (system install) |

---

## OS-Level Permissions

| Permission | Platform | Why Needed | Required |
|------------|----------|------------|----------|
| Microphone | All | Audio recording | Yes |
| Accessibility | macOS | Text injection via keystroke | Yes |
| Audio Input | Windows | Audio recording | Yes |
| Keyboard Monitoring | macOS | Globe key detection | Optional |

---

## Native Modules / Binaries

| Module | Platform | Purpose | Risk |
|--------|----------|---------|------|
| better-sqlite3 | All | Database | Requires native rebuild per platform |
| ffmpeg-static | All | Audio conversion | Large binary (~100MB), GPL licensed |
| Python | All | Whisper runtime | Must be installed by user or bundled |
| Whisper models | All | ML models | 74MB-1.5GB per model, user downloads |

---

## Special Focus Analysis

### Python Dependency
- **Current State**: Python 3.8-3.11 required on user's system
- **Detection**: Registry (Windows), filesystem search, PATH
- **Installation**: Optional installer via pythonInstaller.js
- **Risk**: User may not have Python, installation can fail

### Whisper Bindings
- **Current**: openai-whisper Python package
- **Integration**: Subprocess spawn, JSON over stdout
- **Model Storage**: ~/.cache/whisper
- **Model Sizes**: tiny (39MB), base (74MB), small (244MB), medium (769MB), large (1.5GB)

### Electron APIs Used
| API | Purpose | Replaceability |
|-----|---------|----------------|
| globalShortcut | Hotkey registration | Tauri alternative exists |
| BrowserWindow | Window management | Tauri alternative exists |
| ipcMain/ipcRenderer | IPC communication | Tauri uses different pattern |
| clipboard | Clipboard access | Tauri alternative exists |
| shell | Open external links | Tauri alternative exists |
| dialog | System dialogs | Tauri alternative exists |
| app.dock | macOS dock control | Tauri alternative exists |

---

## License Risk Assessment

| Dependency | License | Risk Level | Notes |
|------------|---------|------------|-------|
| ffmpeg-static | GPL-3.0 | **MEDIUM → HIGH if statically bundled into proprietary distribution** | Binary bundled, may trigger GPL requirements |
| torch | BSD-3-Clause | LOW | Permissive |
| openai-whisper | MIT | LOW | Permissive |
| All Radix UI | MIT | LOW | Permissive |
| Electron | MIT | LOW | Permissive |
| better-sqlite3 | MIT | LOW | Permissive |

**GPL-3.0 Note**: ffmpeg-static bundles FFmpeg binaries which are GPL licensed. Current approach bundles these binaries in the Electron app, which may have licensing implications. Alternative: require user to install FFmpeg separately.

---

## Dependency Size Analysis

| Category | Approximate Size |
|----------|------------------|
| node_modules | ~500MB |
| Electron runtime | ~200MB |
| FFmpeg binary | ~100MB |
| Whisper base model | 74MB |
| Python runtime | ~100MB (if bundled) |
| **Total app size** | **~400-600MB** |