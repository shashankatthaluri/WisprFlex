/**
 * WisprFlex Native Bridge - Module Entry Point
 * 
 * Exports the async native bridge for use by EngineController.
 */

const AsyncNativeBridge = require('./AsyncNativeBridge');
const ThreadSafeCallback = require('./ThreadSafeCallback');
const NativeWorker = require('./NativeWorker');

module.exports = {
    AsyncNativeBridge,
    ThreadSafeCallback,
    NativeWorker,

    // Factory function
    createBridge: () => new AsyncNativeBridge()
};
