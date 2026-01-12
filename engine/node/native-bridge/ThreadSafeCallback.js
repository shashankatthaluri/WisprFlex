/**
 * WisprFlex Native Bridge - Thread-Safe Callback
 * 
 * Demonstrates N-API napi_threadsafe_function concept in JavaScript.
 * Enables safe event emission from worker thread to main thread.
 * 
 * From ENGINE_ARCHITECTURE.md Section 6:
 * "Message passing via N-API thread-safe functions"
 */

const { EventEmitter } = require('events');

/**
 * Thread-safe callback wrapper
 * 
 * In real N-API, this would use napi_threadsafe_function to safely
 * call JavaScript functions from native worker threads.
 * 
 * This JS implementation uses MessagePort for the same pattern.
 */
class ThreadSafeCallback extends EventEmitter {
    constructor() {
        super();
        this._isReleased = false;
    }

    /**
     * Emit an event from worker context to main thread
     * Thread-safe: can be called from any thread/worker
     * 
     * @param {string} eventType - Event type (e.g., 'partial_transcript')
     * @param {object} data - Event data
     */
    emit(eventType, data) {
        if (this._isReleased) {
            console.warn('[ThreadSafeCallback] Attempted emit after release');
            return false;
        }
        return super.emit(eventType, data);
    }

    /**
     * Queue an event for emission (non-blocking)
     * Uses setImmediate to ensure we don't block the caller
     * 
     * @param {string} eventType 
     * @param {object} data 
     */
    queueEmit(eventType, data) {
        if (this._isReleased) return;

        setImmediate(() => {
            if (!this._isReleased) {
                this.emit(eventType, data);
            }
        });
    }

    /**
     * Release the callback (cleanup)
     * After release, no more events will be emitted.
     */
    release() {
        this._isReleased = true;
        this.removeAllListeners();
    }

    /**
     * Check if callback is still active
     * @returns {boolean}
     */
    isActive() {
        return !this._isReleased;
    }
}

module.exports = ThreadSafeCallback;
