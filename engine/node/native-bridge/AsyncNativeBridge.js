/**
 * WisprFlex Native Bridge - Async Native Bridge
 * 
 * Main binding layer that wraps native engine calls with async Promises.
 * All calls are non-blocking per ENGINE_ARCHITECTURE.md Section 6.
 * 
 * This is the JavaScript equivalent of what would be a C++ N-API addon.
 * The patterns here translate directly to napi_async_work and
 * napi_threadsafe_function when moving to real native code.
 * 
 * Key Design:
 * - All methods return Promises (non-blocking)
 * - Events emitted via ThreadSafeCallback
 * - Worker handles "native" processing off main thread
 */

const NativeWorker = require('./NativeWorker');
const ThreadSafeCallback = require('./ThreadSafeCallback');
const { EngineState, EventType } = require('../types');
const { ErrorCode, Errors } = require('../errors');

/**
 * Session ID generator
 */
function generateSessionId() {
    return `session_${Date.now()}_${Math.random().toString(36).substr(2, 9)}`;
}

/**
 * Async Native Bridge
 * 
 * Wraps native engine with non-blocking Promise-based API.
 * Demonstrates the N-API patterns for async execution.
 */
class AsyncNativeBridge {
    constructor() {
        // Thread-safe callback for events
        this._callback = new ThreadSafeCallback();

        // Worker for async execution
        this._worker = new NativeWorker(this._callback);

        // State tracking (minimal, main state in worker)
        this._state = EngineState.UNINITIALIZED;
        this._modelId = null;
        this._sessionId = null;
    }

    // ============================================
    // Event Subscription
    // ============================================

    /**
     * Subscribe to engine events
     * Thread-safe: events are queued from worker to main thread
     * 
     * @param {string} event - Event type
     * @param {function} listener - Event listener
     */
    on(event, listener) {
        this._callback.on(event, listener);
        return this;
    }

    /**
     * Remove event listener
     * @param {string} event
     * @param {function} listener
     */
    off(event, listener) {
        this._callback.off(event, listener);
        return this;
    }

    /**
     * Subscribe once
     * @param {string} event
     * @param {function} listener
     */
    once(event, listener) {
        this._callback.once(event, listener);
        return this;
    }

    // ============================================
    // Engine Lifecycle API (All Async)
    // ============================================

    /**
     * Initialize the engine
     * Non-blocking: returns immediately, work happens async
     * 
     * @param {object} config - { device: 'cpu'|'gpu', logLevel: 'error'|'warn'|'info' }
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
        if (!config || !config.device) {
            throw Errors.engineInitFailed('Config with device required');
        }
        if (config.device !== 'cpu') {
            throw Errors.deviceNotSupported(config.device);
        }

        // Execute async (non-blocking)
        await this._worker.execute('INIT', config);
        this._state = EngineState.INITIALIZED;
    }

    /**
     * Load a transcription model
     * Non-blocking: returns Promise, work happens on worker
     * 
     * @param {string} modelId
     * @returns {Promise<void>}
     */
    async loadModel(modelId) {
        this._assertNotDisposed();

        if (this._state === EngineState.UNINITIALIZED) {
            throw Errors.internalError('Engine not initialized');
        }
        if (this._state === EngineState.SESSION_ACTIVE) {
            throw Errors.internalError('Cannot load model during active session');
        }

        // Validate modelId
        const supported = ['tiny', 'base', 'small', 'medium'];
        if (!supported.includes(modelId)) {
            throw Errors.modelNotFound(modelId);
        }

        // Execute async (non-blocking)
        await this._worker.execute('LOAD_MODEL', { modelId });
        this._modelId = modelId;
        this._state = EngineState.MODEL_LOADED;
    }

    /**
     * Unload current model
     * Non-blocking
     * 
     * @returns {Promise<void>}
     */
    async unloadModel() {
        this._assertNotDisposed();

        if (this._state === EngineState.SESSION_ACTIVE) {
            throw Errors.internalError('Cannot unload model during active session');
        }

        await this._worker.execute('UNLOAD_MODEL');
        this._modelId = null;

        if (this._state === EngineState.MODEL_LOADED) {
            this._state = EngineState.INITIALIZED;
        }
    }

    /**
     * Start a transcription session
     * Non-blocking: returns immediately with session ID
     * 
     * @param {object} config - { language, vadEnabled }
     * @returns {Promise<string>} Session ID
     */
    async startSession(config = {}) {
        this._assertNotDisposed();

        if (!this._modelId) {
            throw Errors.modelNotLoaded();
        }
        if (this._sessionId) {
            throw Errors.sessionAlreadyActive();
        }

        const sessionId = generateSessionId();

        await this._worker.execute('START_SESSION', {
            sessionId,
            language: config.language || 'auto',
            vadEnabled: config.vadEnabled !== false
        });

        this._sessionId = sessionId;
        this._state = EngineState.SESSION_ACTIVE;

        return sessionId;
    }

    /**
     * Push audio data for transcription
     * Non-blocking: queues work, returns immediately
     * 
     * @param {string} sessionId
     * @param {Float32Array} pcmData - 16kHz mono Float32
     * @returns {Promise<void>}
     */
    async pushAudio(sessionId, pcmData) {
        this._assertNotDisposed();

        if (!this._sessionId) {
            throw Errors.sessionEnded(sessionId);
        }
        if (sessionId !== this._sessionId) {
            throw Errors.invalidSession(sessionId);
        }
        if (!pcmData || !(pcmData instanceof Float32Array)) {
            throw Errors.audioStreamError('Invalid audio data');
        }

        // Non-blocking: queue work
        await this._worker.execute('PUSH_AUDIO', {
            sessionId,
            samples: pcmData.length
        });
    }

    /**
     * End transcription session
     * Triggers final transcript emission
     * 
     * @param {string} sessionId
     * @returns {Promise<void>}
     */
    async endSession(sessionId) {
        this._assertNotDisposed();

        if (!this._sessionId) {
            throw Errors.sessionEnded(sessionId);
        }
        if (sessionId !== this._sessionId) {
            throw Errors.invalidSession(sessionId);
        }

        await this._worker.execute('END_SESSION', { sessionId });
        this._sessionId = null;
        this._state = EngineState.MODEL_LOADED;
    }

    /**
     * Dispose engine and free all resources
     * Non-blocking, idempotent
     * 
     * @returns {Promise<void>}
     */
    async dispose() {
        if (this._state === EngineState.DISPOSED) {
            return; // Idempotent
        }

        try {
            await this._worker.execute('DISPOSE');
        } catch (e) {
            // Ignore errors during cleanup
        }

        this._worker.stop();
        this._callback.release();

        this._state = EngineState.DISPOSED;
        this._modelId = null;
        this._sessionId = null;
    }

    // ============================================
    // Utility Methods
    // ============================================

    /**
     * Get engine version
     * @returns {string}
     */
    getVersion() {
        return '0.1.0';
    }

    /**
     * Get current state
     * @returns {string}
     */
    getState() {
        return this._state;
    }

    /**
     * Get loaded model
     * @returns {string|null}
     */
    getLoadedModel() {
        return this._modelId;
    }

    /**
     * Get active session
     * @returns {string|null}
     */
    getActiveSession() {
        return this._sessionId;
    }

    // ============================================
    // Private Methods
    // ============================================

    /**
     * Assert engine not disposed
     * @private
     */
    _assertNotDisposed() {
        if (this._state === EngineState.DISPOSED) {
            throw Errors.engineDisposed();
        }
    }
}

module.exports = AsyncNativeBridge;
