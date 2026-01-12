/**
 * WisprFlex Engine - Error Codes and EngineError Class
 * Matches ENGINE_API_SPEC.md Section 6 exactly
 */

/**
 * Engine error codes - non-exhaustive list from spec
 * @readonly
 * @enum {string}
 */
const ErrorCode = {
    ENGINE_INIT_FAILED: 'ENGINE_INIT_FAILED',
    DEVICE_NOT_SUPPORTED: 'DEVICE_NOT_SUPPORTED',
    MODEL_NOT_FOUND: 'MODEL_NOT_FOUND',
    MODEL_LOAD_FAILED: 'MODEL_LOAD_FAILED',
    MODEL_NOT_LOADED: 'MODEL_NOT_LOADED',
    OUT_OF_MEMORY: 'OUT_OF_MEMORY',
    SESSION_ALREADY_ACTIVE: 'SESSION_ALREADY_ACTIVE',
    INVALID_SESSION: 'INVALID_SESSION',
    SESSION_ENDED: 'SESSION_ENDED',
    BACKPRESSURE_LIMIT: 'BACKPRESSURE_LIMIT',
    AUDIO_STREAM_ERROR: 'AUDIO_STREAM_ERROR',
    INTERNAL_ENGINE_ERROR: 'INTERNAL_ENGINE_ERROR'
};

/**
 * Structured error type matching ENGINE_API_SPEC.md
 * All errors conform to: { code, message, recoverable }
 */
class EngineError extends Error {
    /**
     * @param {string} code - Error code from ErrorCode enum
     * @param {string} message - Human-readable error message
     * @param {boolean} recoverable - Whether the error is recoverable
     */
    constructor(code, message, recoverable = true) {
        super(message);
        this.name = 'EngineError';
        this.code = code;
        this.message = message;
        this.recoverable = recoverable;
    }

    /**
     * Convert to plain object for event emission
     * @returns {{ code: string, message: string, recoverable: boolean }}
     */
    toJSON() {
        return {
            code: this.code,
            message: this.message,
            recoverable: this.recoverable
        };
    }
}

/**
 * Factory functions for common errors
 */
const Errors = {
    engineInitFailed: (reason = 'Engine initialization failed') =>
        new EngineError(ErrorCode.ENGINE_INIT_FAILED, reason, false),

    deviceNotSupported: (device) =>
        new EngineError(ErrorCode.DEVICE_NOT_SUPPORTED, `Device '${device}' is not supported`, false),

    modelNotFound: (modelId) =>
        new EngineError(ErrorCode.MODEL_NOT_FOUND, `Model '${modelId}' not found`, true),

    modelLoadFailed: (modelId, reason = 'Unknown error') =>
        new EngineError(ErrorCode.MODEL_LOAD_FAILED, `Failed to load model '${modelId}': ${reason}`, true),

    modelNotLoaded: () =>
        new EngineError(ErrorCode.MODEL_NOT_LOADED, 'No model is currently loaded', true),

    outOfMemory: () =>
        new EngineError(ErrorCode.OUT_OF_MEMORY, 'Insufficient memory to complete operation', false),

    sessionAlreadyActive: () =>
        new EngineError(ErrorCode.SESSION_ALREADY_ACTIVE, 'A session is already active', true),

    invalidSession: (sessionId) =>
        new EngineError(ErrorCode.INVALID_SESSION, `Invalid session ID: ${sessionId}`, true),

    sessionEnded: (sessionId) =>
        new EngineError(ErrorCode.SESSION_ENDED, `Session '${sessionId}' has already ended`, true),

    backpressureLimit: () =>
        new EngineError(ErrorCode.BACKPRESSURE_LIMIT, 'Backpressure limit reached, audio dropped', true),

    audioStreamError: (reason = 'Audio stream error') =>
        new EngineError(ErrorCode.AUDIO_STREAM_ERROR, reason, true),

    internalError: (reason = 'Internal engine error') =>
        new EngineError(ErrorCode.INTERNAL_ENGINE_ERROR, reason, false),

    engineDisposed: () =>
        new EngineError(ErrorCode.INTERNAL_ENGINE_ERROR, 'Engine has been disposed', false)
};

module.exports = {
    ErrorCode,
    EngineError,
    Errors
};
