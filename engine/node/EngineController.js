/**
 * WisprFlex Engine Controller (Node Layer)
 * 
 * Implements ENGINE_API_SPEC.md exactly.
 * This is a DUMMY implementation for Phase 2.1 - no ML logic.
 * 
 * Responsibilities (from ENGINE_ARCHITECTURE.md Section 4.2):
 * - Own transcription sessions
 * - Validate inputs
 * - Queue and forward audio chunks
 * - Apply backpressure (drop / delay audio if needed)
 * - Translate native errors into structured JS errors
 * - Emit partial/final text events
 * 
 * Forbidden:
 * - Audio decoding
 * - ML logic
 * - Model loading (simulated only)
 * - Text post-processing
 */

const { EventEmitter } = require('events');
const { EngineState, EventType } = require('./types');
const { ErrorCode, EngineError, Errors } = require('./errors');

// Constants from STREAMING_STRATEGY.md
const SIMULATED_PARTIAL_DELAY_MS = 250; // 200-300ms range
const SIMULATED_MODEL_LOAD_MS = 500;
const MAX_AUDIO_QUEUE_SIZE = 10; // Backpressure threshold
const ENGINE_VERSION = '0.1.0';

/**
 * Generate a unique session ID
 * @returns {string}
 */
function generateSessionId() {
    return `session_${Date.now()}_${Math.random().toString(36).substr(2, 9)}`;
}

/**
 * Engine Controller - Node Layer
 * 
 * Provides the public API for the WisprFlex transcription engine.
 * All methods return Promises (async by default per spec).
 * Events are emitted via EventEmitter interface.
 */
class EngineController extends EventEmitter {
    constructor() {
        super();

        // Internal state
        this._state = EngineState.UNINITIALIZED;
        this._config = null;
        this._loadedModelId = null;
        this._activeSessionId = null;
        this._sessionConfig = null;
        this._audioQueue = [];
        this._partialText = '';
        this._chunkCount = 0;
        this._isProcessing = false;
        this._processingTimer = null;
    }

    // ============================================
    // PUBLIC API - Engine Lifecycle
    // ============================================

    /**
     * Get engine version (per API spec Section 8)
     * @returns {string}
     */
    getVersion() {
        return ENGINE_VERSION;
    }

    /**
     * Initialize the engine runtime
     * Must be called exactly once before any other operations.
     * 
     * @param {import('./types').EngineConfig} config
     * @returns {Promise<void>}
     */
    async init(config) {
        // Validate state
        if (this._state === EngineState.DISPOSED) {
            throw Errors.engineDisposed();
        }
        if (this._state !== EngineState.UNINITIALIZED) {
            throw Errors.internalError('Engine already initialized');
        }

        // Validate config
        if (!config || typeof config !== 'object') {
            throw Errors.engineInitFailed('Config is required');
        }
        if (!config.device || !['cpu', 'gpu'].includes(config.device)) {
            throw Errors.deviceNotSupported(config.device || 'undefined');
        }

        // For Phase 2.1, we only support CPU (GPU requires native implementation)
        if (config.device === 'gpu') {
            throw Errors.deviceNotSupported('gpu (not implemented in Phase 2.1)');
        }

        // Store config
        this._config = {
            device: config.device,
            logLevel: config.logLevel || 'error'
        };

        // Transition state
        this._state = EngineState.INITIALIZED;
        this._log('info', 'Engine initialized', { config: this._config });
    }

    /**
     * Load a transcription model into memory
     * Only one model loaded at a time - automatically unloads previous.
     * 
     * @param {string} modelId - Model identifier (e.g., 'base', 'small')
     * @returns {Promise<void>}
     */
    async loadModel(modelId) {
        // Validate state
        this._assertNotDisposed();
        if (this._state === EngineState.UNINITIALIZED) {
            throw Errors.internalError('Engine not initialized. Call init() first.');
        }
        if (this._state === EngineState.SESSION_ACTIVE) {
            throw Errors.internalError('Cannot load model while session is active');
        }

        // Validate modelId
        if (!modelId || typeof modelId !== 'string') {
            throw Errors.modelNotFound(modelId || 'undefined');
        }

        // Supported models for Phase 2.1 (dummy)
        const supportedModels = ['tiny', 'base', 'small', 'medium'];
        if (!supportedModels.includes(modelId)) {
            throw Errors.modelNotFound(modelId);
        }

        // Unload previous model if any
        if (this._loadedModelId) {
            await this.unloadModel();
        }

        // Simulate model loading with progress events
        this._log('info', `Loading model: ${modelId}`);

        const steps = 5;
        for (let i = 0; i <= steps; i++) {
            const progress = Math.round((i / steps) * 100);
            this.emit(EventType.MODEL_PROGRESS, {
                type: EventType.MODEL_PROGRESS,
                modelId,
                progress
            });
            await this._sleep(SIMULATED_MODEL_LOAD_MS / steps);
        }

        // Model loaded
        this._loadedModelId = modelId;
        this._state = EngineState.MODEL_LOADED;
        this._log('info', `Model loaded: ${modelId}`);
    }

    /**
     * Unload the currently loaded model
     * Safe to call even if no model is loaded.
     * 
     * @returns {Promise<void>}
     */
    async unloadModel() {
        this._assertNotDisposed();

        if (this._state === EngineState.SESSION_ACTIVE) {
            throw Errors.internalError('Cannot unload model while session is active');
        }

        const previousModel = this._loadedModelId;
        this._loadedModelId = null;

        if (this._state === EngineState.MODEL_LOADED) {
            this._state = EngineState.INITIALIZED;
        }

        if (previousModel) {
            this._log('info', `Model unloaded: ${previousModel}`);
        }
    }

    /**
     * Shut down engine and free all resources
     * Idempotent - safe to call multiple times.
     * After dispose, engine cannot be reused.
     * 
     * @returns {Promise<void>}
     */
    async dispose() {
        if (this._state === EngineState.DISPOSED) {
            return; // Already disposed, idempotent
        }

        // Cancel any processing
        if (this._processingTimer) {
            clearTimeout(this._processingTimer);
            this._processingTimer = null;
        }

        // End active session if any
        if (this._activeSessionId) {
            try {
                await this.endSession(this._activeSessionId);
            } catch (e) {
                // Ignore errors during cleanup
            }
        }

        // Clear all state
        this._config = null;
        this._loadedModelId = null;
        this._activeSessionId = null;
        this._sessionConfig = null;
        this._audioQueue = [];
        this._partialText = '';
        this._chunkCount = 0;
        this._isProcessing = false;

        // Transition to disposed
        this._state = EngineState.DISPOSED;
        this._log('info', 'Engine disposed');

        // Remove all listeners
        this.removeAllListeners();
    }

    // ============================================
    // PUBLIC API - Session Management
    // ============================================

    /**
     * Create a new transcription session
     * Requires model to be loaded. One active session at a time (V1).
     * 
     * @param {import('./types').SessionConfig} config
     * @returns {Promise<string>} Session ID
     */
    async startSession(config = {}) {
        this._assertNotDisposed();

        // Validate: model must be loaded
        if (!this._loadedModelId) {
            throw Errors.modelNotLoaded();
        }

        // Validate: no active session
        if (this._activeSessionId) {
            throw Errors.sessionAlreadyActive();
        }

        // Create session
        const sessionId = generateSessionId();
        this._activeSessionId = sessionId;
        this._sessionConfig = {
            language: config.language || 'auto',
            vadEnabled: config.vadEnabled !== false
        };
        this._audioQueue = [];
        this._partialText = '';
        this._chunkCount = 0;

        // Transition state
        this._state = EngineState.SESSION_ACTIVE;
        this._log('info', `Session started: ${sessionId}`, { config: this._sessionConfig });

        return sessionId;
    }

    /**
     * Push audio data into an active session
     * Non-blocking. Engine may apply backpressure.
     * 
     * @param {string} sessionId
     * @param {Float32Array} pcmChunk - PCM Float32, 16kHz, mono
     * @returns {Promise<void>}
     */
    async pushAudio(sessionId, pcmChunk) {
        this._assertNotDisposed();

        // Validate session
        if (!this._activeSessionId) {
            throw Errors.sessionEnded(sessionId);
        }
        if (sessionId !== this._activeSessionId) {
            throw Errors.invalidSession(sessionId);
        }

        // Validate audio data
        if (!pcmChunk || !(pcmChunk instanceof Float32Array)) {
            throw Errors.audioStreamError('Invalid audio data: expected Float32Array');
        }

        // Check backpressure
        if (this._audioQueue.length >= MAX_AUDIO_QUEUE_SIZE) {
            // Emit warning but don't fail
            this.emit(EventType.BACKPRESSURE_WARNING, {
                type: EventType.BACKPRESSURE_WARNING,
                sessionId,
                droppedChunks: 1
            });

            // Drop oldest chunk (per STREAMING_STRATEGY.md Section 7.2)
            this._audioQueue.shift();
            this._log('warn', 'Backpressure: dropped oldest chunk');
        }

        // Queue the chunk
        this._audioQueue.push(pcmChunk);
        this._chunkCount++;

        // Process asynchronously (simulate transcription)
        this._scheduleProcessing(sessionId);
    }

    /**
     * Signal end of audio stream
     * Flushes remaining buffers and triggers final transcription.
     * 
     * @param {string} sessionId
     * @returns {Promise<void>}
     */
    async endSession(sessionId) {
        this._assertNotDisposed();

        // Validate session
        if (!this._activeSessionId) {
            throw Errors.sessionEnded(sessionId);
        }
        if (sessionId !== this._activeSessionId) {
            throw Errors.invalidSession(sessionId);
        }

        // Cancel pending processing
        if (this._processingTimer) {
            clearTimeout(this._processingTimer);
            this._processingTimer = null;
        }

        // Emit final transcript
        const finalText = this._partialText || this._generateDummyText(true);
        this.emit(EventType.FINAL_TRANSCRIPT, {
            type: EventType.FINAL_TRANSCRIPT,
            sessionId,
            text: finalText
        });

        this._log('info', `Session ended: ${sessionId}`, { finalText });

        // Clear session state
        this._activeSessionId = null;
        this._sessionConfig = null;
        this._audioQueue = [];
        this._partialText = '';
        this._chunkCount = 0;
        this._isProcessing = false;

        // Return to model loaded state
        this._state = EngineState.MODEL_LOADED;
    }

    // ============================================
    // INTERNAL METHODS
    // ============================================

    /**
     * Assert that engine is not disposed
     * @private
     */
    _assertNotDisposed() {
        if (this._state === EngineState.DISPOSED) {
            throw Errors.engineDisposed();
        }
    }

    /**
     * Schedule async processing of audio queue
     * @private
     * @param {string} sessionId
     */
    _scheduleProcessing(sessionId) {
        if (this._isProcessing || !this._activeSessionId) {
            return;
        }

        this._isProcessing = true;
        this._processingTimer = setTimeout(() => {
            this._processQueue(sessionId);
        }, SIMULATED_PARTIAL_DELAY_MS);
    }

    /**
     * Process queued audio and emit partial transcript
     * @private
     * @param {string} sessionId
     */
    _processQueue(sessionId) {
        // Check if session is still active
        if (sessionId !== this._activeSessionId) {
            this._isProcessing = false;
            return;
        }

        // Generate dummy partial text
        const newPartial = this._generateDummyText(false);
        this._partialText = newPartial;

        // Determine stability (per STREAMING_STRATEGY.md Section 5.2)
        // Stable after 2 consecutive chunks without change
        const isStable = this._chunkCount >= 2;

        // Emit partial transcript
        this.emit(EventType.PARTIAL_TRANSCRIPT, {
            type: EventType.PARTIAL_TRANSCRIPT,
            sessionId,
            text: newPartial,
            isStable
        });

        // Clear processed chunks
        this._audioQueue = [];
        this._isProcessing = false;

        this._log('info', 'Partial transcript emitted', { text: newPartial, isStable });
    }

    /**
     * Generate dummy transcription text
     * @private
     * @param {boolean} isFinal
     * @returns {string}
     */
    _generateDummyText(isFinal) {
        const dummyPhrases = [
            'Hello world',
            'This is a test transcription',
            'The quick brown fox jumps over the lazy dog',
            'Voice transcription is working',
            'WisprFlex engine is online'
        ];

        const phrase = dummyPhrases[this._chunkCount % dummyPhrases.length];
        return isFinal ? `[FINAL] ${phrase}` : `[PARTIAL] ${phrase}`;
    }

    /**
     * Internal logging
     * @private
     * @param {'error' | 'warn' | 'info'} level
     * @param {string} message
     * @param {object} [data]
     */
    _log(level, message, data = {}) {
        const levels = { error: 0, warn: 1, info: 2 };
        const configLevel = this._config?.logLevel || 'error';

        if (levels[level] <= levels[configLevel]) {
            console.log(`[EngineController:${level.toUpperCase()}] ${message}`, data);
        }
    }

    /**
     * Sleep utility
     * @private
     * @param {number} ms
     * @returns {Promise<void>}
     */
    _sleep(ms) {
        return new Promise(resolve => setTimeout(resolve, ms));
    }

    // ============================================
    // STATE INSPECTION (for testing)
    // ============================================

    /**
     * Get current state (for testing purposes)
     * @returns {string}
     */
    getState() {
        return this._state;
    }

    /**
     * Get loaded model ID (for testing purposes)
     * @returns {string|null}
     */
    getLoadedModel() {
        return this._loadedModelId;
    }

    /**
     * Get active session ID (for testing purposes)
     * @returns {string|null}
     */
    getActiveSession() {
        return this._activeSessionId;
    }
}

module.exports = EngineController;
