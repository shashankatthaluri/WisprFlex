/**
 * WisprFlex Engine - Node Module Entry Point
 * 
 * Public exports for the engine/node module.
 */

const EngineController = require('./EngineController');
const { EngineState, EventType } = require('./types');
const { ErrorCode, EngineError, Errors } = require('./errors');

module.exports = {
    // Main class
    EngineController,

    // Enums
    EngineState,
    EventType,
    ErrorCode,

    // Error utilities
    EngineError,
    Errors,

    // Factory function for convenience
    createEngine: () => new EngineController()
};
