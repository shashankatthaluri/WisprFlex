/**
 * WisprFlex Engine - Type Definitions
 * Matches ENGINE_API_SPEC.md exactly
 */

/**
 * @typedef {'cpu' | 'gpu'} DeviceType
 * @typedef {'error' | 'warn' | 'info'} LogLevel
 */

/**
 * @typedef {Object} EngineConfig
 * @property {DeviceType} device - Hardware device to use
 * @property {LogLevel} [logLevel] - Logging level (default: 'error')
 */

/**
 * @typedef {Object} SessionConfig
 * @property {string} [language] - Language code (default: 'auto')
 * @property {boolean} [vadEnabled] - Voice activity detection (default: true)
 */

/**
 * @typedef {string} SessionId - Opaque session identifier
 */

/**
 * Engine states
 * @readonly
 * @enum {string}
 */
const EngineState = {
    UNINITIALIZED: 'UNINITIALIZED',
    INITIALIZED: 'INITIALIZED',
    MODEL_LOADED: 'MODEL_LOADED',
    SESSION_ACTIVE: 'SESSION_ACTIVE',
    DISPOSED: 'DISPOSED'
};

/**
 * Event types emitted by the engine
 * @readonly
 * @enum {string}
 */
const EventType = {
    PARTIAL_TRANSCRIPT: 'partial_transcript',
    FINAL_TRANSCRIPT: 'final_transcript',
    ERROR: 'error',
    MODEL_PROGRESS: 'model_progress',
    BACKPRESSURE_WARNING: 'backpressure_warning'
};

/**
 * @typedef {Object} PartialTranscriptEvent
 * @property {'partial_transcript'} type
 * @property {SessionId} sessionId
 * @property {string} text
 * @property {boolean} isStable
 */

/**
 * @typedef {Object} FinalTranscriptEvent
 * @property {'final_transcript'} type
 * @property {SessionId} sessionId
 * @property {string} text
 */

/**
 * @typedef {Object} ErrorEvent
 * @property {'error'} type
 * @property {SessionId} [sessionId]
 * @property {import('./errors').EngineError} error
 */

/**
 * @typedef {Object} ModelProgressEvent
 * @property {'model_progress'} type
 * @property {string} modelId
 * @property {number} progress - 0-100
 */

/**
 * @typedef {Object} BackpressureWarningEvent
 * @property {'backpressure_warning'} type
 * @property {SessionId} sessionId
 * @property {number} droppedChunks
 */

module.exports = {
    EngineState,
    EventType
};
