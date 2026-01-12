/**
 * WisprFlex Native Bridge - Worker Thread Simulation
 * 
 * Simulates native worker thread execution for async processing.
 * In real implementation, this would be a C++ native addon using
 * N-API AsyncWorker or libuv work queue.
 * 
 * From ENGINE_ARCHITECTURE.md Section 6:
 * "Native worker threads: Audio processing, Whisper inference"
 * "Engine work must be off main thread"
 */

const { EventType } = require('../types');

/**
 * Simulated work delays (ms)
 */
const WORK_DELAYS = {
    INIT: 50,
    LOAD_MODEL: 500,
    UNLOAD_MODEL: 100,
    START_SESSION: 50,
    PUSH_AUDIO: 200,  // Simulates inference time
    END_SESSION: 100,
    DISPOSE: 50
};

/**
 * Native Worker Simulation
 * 
 * Processes commands asynchronously without blocking the caller.
 * Uses setTimeout to simulate native work happening "off-thread".
 */
class NativeWorker {
    constructor(callback) {
        this._callback = callback;
        this._isRunning = false;
        this._pendingWork = [];
        this._state = {
            initialized: false,
            modelId: null,
            sessionId: null,
            chunkCount: 0
        };
    }

    /**
     * Start the worker
     */
    start() {
        this._isRunning = true;
        console.log('[NativeWorker] Started');
    }

    /**
     * Stop the worker
     */
    stop() {
        this._isRunning = false;
        this._pendingWork = [];
        console.log('[NativeWorker] Stopped');
    }

    /**
     * Queue work for async execution
     * Returns immediately (non-blocking)
     * 
     * @param {string} type - Work type
     * @param {object} data - Work data
     * @returns {Promise} Resolves when work completes
     */
    async execute(type, data = {}) {
        if (!this._isRunning && type !== 'INIT') {
            throw new Error('Worker not running');
        }

        return new Promise((resolve, reject) => {
            const delay = WORK_DELAYS[type] || 50;

            // Simulate async work (in real N-API, this would be libuv work queue)
            const workId = setTimeout(() => {
                try {
                    const result = this._processWork(type, data);
                    resolve(result);
                } catch (err) {
                    reject(err);
                }
            }, delay);

            this._pendingWork.push(workId);
        });
    }

    /**
     * Process work item (runs "off main thread" conceptually)
     * @private
     */
    _processWork(type, data) {
        switch (type) {
            case 'INIT':
                this._state.initialized = true;
                this._isRunning = true;
                return { success: true };

            case 'LOAD_MODEL':
                this._state.modelId = data.modelId;
                // Emit progress events
                this._emitModelProgress(data.modelId);
                return { success: true, modelId: data.modelId };

            case 'UNLOAD_MODEL':
                this._state.modelId = null;
                return { success: true };

            case 'START_SESSION':
                this._state.sessionId = data.sessionId;
                this._state.chunkCount = 0;
                return { success: true, sessionId: data.sessionId };

            case 'PUSH_AUDIO':
                this._state.chunkCount++;
                // Emit partial transcript
                this._emitPartialTranscript(data.sessionId);
                return { success: true };

            case 'END_SESSION':
                // Emit final transcript
                this._emitFinalTranscript(data.sessionId);
                this._state.sessionId = null;
                this._state.chunkCount = 0;
                return { success: true };

            case 'DISPOSE':
                this._state = {
                    initialized: false,
                    modelId: null,
                    sessionId: null,
                    chunkCount: 0
                };
                this._isRunning = false;
                return { success: true };

            default:
                throw new Error(`Unknown work type: ${type}`);
        }
    }

    /**
     * Emit model progress events (simulated)
     * @private
     */
    _emitModelProgress(modelId) {
        if (!this._callback) return;

        const steps = [0, 25, 50, 75, 100];
        steps.forEach((progress, i) => {
            setTimeout(() => {
                this._callback.queueEmit(EventType.MODEL_PROGRESS, {
                    type: EventType.MODEL_PROGRESS,
                    modelId,
                    progress
                });
            }, i * 80);
        });
    }

    /**
     * Emit partial transcript (simulated)
     * @private
     */
    _emitPartialTranscript(sessionId) {
        if (!this._callback) return;

        const phrases = [
            'Hello world',
            'This is a test',
            'WisprFlex engine working'
        ];
        const text = phrases[this._state.chunkCount % phrases.length];
        const isStable = this._state.chunkCount >= 2;

        this._callback.queueEmit(EventType.PARTIAL_TRANSCRIPT, {
            type: EventType.PARTIAL_TRANSCRIPT,
            sessionId,
            text: `[PARTIAL] ${text}`,
            isStable
        });
    }

    /**
     * Emit final transcript (simulated)
     * @private
     */
    _emitFinalTranscript(sessionId) {
        if (!this._callback) return;

        this._callback.queueEmit(EventType.FINAL_TRANSCRIPT, {
            type: EventType.FINAL_TRANSCRIPT,
            sessionId,
            text: '[FINAL] Transcription complete'
        });
    }

    /**
     * Check if worker is running
     */
    isRunning() {
        return this._isRunning;
    }
}

module.exports = NativeWorker;
