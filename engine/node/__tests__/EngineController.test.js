/**
 * WisprFlex Engine Controller - Test Suite
 * 
 * Verifies Phase 2.1 correctness per ENGINE_API_SPEC.md
 * 
 * Tests:
 * - D1: Lifecycle tests (valid sequences, invalid sequences, repeated init/dispose)
 * - D2: Event order verification (partial before final, no events after dispose)
 * - A2: State validation (error codes match spec)
 */

const EngineController = require('../EngineController');
const { EngineState, EventType } = require('../types');
const { ErrorCode } = require('../errors');

// Simple test framework
let passed = 0;
let failed = 0;

function test(name, fn) {
    return async () => {
        try {
            await fn();
            console.log(`✅ PASS: ${name}`);
            passed++;
        } catch (err) {
            console.log(`❌ FAIL: ${name}`);
            console.log(`   Error: ${err.message}`);
            if (err.code) console.log(`   Code: ${err.code}`);
            failed++;
        }
    };
}

function assert(condition, message) {
    if (!condition) {
        throw new Error(message || 'Assertion failed');
    }
}

function assertEqual(actual, expected, message) {
    if (actual !== expected) {
        throw new Error(message || `Expected ${expected}, got ${actual}`);
    }
}

// ============================================
// LIFECYCLE TESTS (D1)
// ============================================

const testInitSuccess = test('init() succeeds with valid config', async () => {
    const engine = new EngineController();
    await engine.init({ device: 'cpu' });
    assertEqual(engine.getState(), EngineState.INITIALIZED);
    await engine.dispose();
});

const testInitFailsWithoutDevice = test('init() fails without device', async () => {
    const engine = new EngineController();
    try {
        await engine.init({});
        throw new Error('Should have thrown');
    } catch (err) {
        assertEqual(err.code, ErrorCode.DEVICE_NOT_SUPPORTED);
    }
    await engine.dispose();
});

const testInitFailsWithGpu = test('init() fails with GPU in Phase 2.1', async () => {
    const engine = new EngineController();
    try {
        await engine.init({ device: 'gpu' });
        throw new Error('Should have thrown');
    } catch (err) {
        assertEqual(err.code, ErrorCode.DEVICE_NOT_SUPPORTED);
    }
    await engine.dispose();
});

const testLoadModelSuccess = test('loadModel() succeeds after init', async () => {
    const engine = new EngineController();
    await engine.init({ device: 'cpu' });
    await engine.loadModel('base');
    assertEqual(engine.getState(), EngineState.MODEL_LOADED);
    assertEqual(engine.getLoadedModel(), 'base');
    await engine.dispose();
});

const testLoadModelEmitsProgress = test('loadModel() emits progress events', async () => {
    const engine = new EngineController();
    const progressEvents = [];

    engine.on(EventType.MODEL_PROGRESS, (event) => {
        progressEvents.push(event.progress);
    });

    await engine.init({ device: 'cpu' });
    await engine.loadModel('tiny');

    assert(progressEvents.length > 0, 'Should emit progress events');
    assert(progressEvents.includes(100), 'Should reach 100%');
    await engine.dispose();
});

const testUnloadModel = test('unloadModel() clears model', async () => {
    const engine = new EngineController();
    await engine.init({ device: 'cpu' });
    await engine.loadModel('base');
    await engine.unloadModel();
    assertEqual(engine.getLoadedModel(), null);
    assertEqual(engine.getState(), EngineState.INITIALIZED);
    await engine.dispose();
});

const testDisposeIsIdempotent = test('dispose() is idempotent', async () => {
    const engine = new EngineController();
    await engine.init({ device: 'cpu' });
    await engine.dispose();
    await engine.dispose(); // Should not throw
    assertEqual(engine.getState(), EngineState.DISPOSED);
});

const testDisposeAfterSession = test('dispose() cleans up active session', async () => {
    const engine = new EngineController();
    await engine.init({ device: 'cpu' });
    await engine.loadModel('base');
    await engine.startSession({});
    await engine.dispose();
    assertEqual(engine.getState(), EngineState.DISPOSED);
});

// ============================================
// SESSION TESTS
// ============================================

const testStartSessionSuccess = test('startSession() returns sessionId', async () => {
    const engine = new EngineController();
    await engine.init({ device: 'cpu' });
    await engine.loadModel('base');
    const sessionId = await engine.startSession({});
    assert(sessionId && sessionId.startsWith('session_'), 'Should return valid sessionId');
    assertEqual(engine.getState(), EngineState.SESSION_ACTIVE);
    await engine.dispose();
});

const testPushAudioEmitsPartial = test('pushAudio() emits partial_transcript', async () => {
    const engine = new EngineController();
    let partialReceived = false;

    engine.on(EventType.PARTIAL_TRANSCRIPT, (event) => {
        partialReceived = true;
        assert(event.text, 'Should have text');
        assert(typeof event.isStable === 'boolean', 'Should have isStable');
    });

    await engine.init({ device: 'cpu' });
    await engine.loadModel('base');
    const sessionId = await engine.startSession({});
    await engine.pushAudio(sessionId, new Float32Array(1600));

    // Wait for async processing
    await new Promise(r => setTimeout(r, 400));

    assert(partialReceived, 'Should receive partial transcript');
    await engine.dispose();
});

const testEndSessionEmitsFinal = test('endSession() emits final_transcript', async () => {
    const engine = new EngineController();
    let finalReceived = false;

    engine.on(EventType.FINAL_TRANSCRIPT, (event) => {
        finalReceived = true;
        assert(event.text, 'Should have text');
    });

    await engine.init({ device: 'cpu' });
    await engine.loadModel('base');
    const sessionId = await engine.startSession({});
    await engine.endSession(sessionId);

    assert(finalReceived, 'Should receive final transcript');
    assertEqual(engine.getState(), EngineState.MODEL_LOADED);
    await engine.dispose();
});

// ============================================
// INVALID SEQUENCE TESTS (A2)
// ============================================

const testStartSessionBeforeLoadModel = test('startSession() before loadModel throws MODEL_NOT_LOADED', async () => {
    const engine = new EngineController();
    await engine.init({ device: 'cpu' });

    try {
        await engine.startSession({});
        throw new Error('Should have thrown');
    } catch (err) {
        assertEqual(err.code, ErrorCode.MODEL_NOT_LOADED);
    }

    await engine.dispose();
});

const testPushAudioAfterEndSession = test('pushAudio() after endSession throws SESSION_ENDED', async () => {
    const engine = new EngineController();
    await engine.init({ device: 'cpu' });
    await engine.loadModel('base');
    const sessionId = await engine.startSession({});
    await engine.endSession(sessionId);

    try {
        await engine.pushAudio(sessionId, new Float32Array(1600));
        throw new Error('Should have thrown');
    } catch (err) {
        assertEqual(err.code, ErrorCode.SESSION_ENDED);
    }

    await engine.dispose();
});

const testPushAudioWithWrongSessionId = test('pushAudio() with wrong sessionId throws INVALID_SESSION', async () => {
    const engine = new EngineController();
    await engine.init({ device: 'cpu' });
    await engine.loadModel('base');
    await engine.startSession({});

    try {
        await engine.pushAudio('wrong_session_id', new Float32Array(1600));
        throw new Error('Should have thrown');
    } catch (err) {
        assertEqual(err.code, ErrorCode.INVALID_SESSION);
    }

    await engine.dispose();
});

const testStartSessionTwice = test('startSession() twice throws SESSION_ALREADY_ACTIVE', async () => {
    const engine = new EngineController();
    await engine.init({ device: 'cpu' });
    await engine.loadModel('base');
    await engine.startSession({});

    try {
        await engine.startSession({});
        throw new Error('Should have thrown');
    } catch (err) {
        assertEqual(err.code, ErrorCode.SESSION_ALREADY_ACTIVE);
    }

    await engine.dispose();
});

const testCallAfterDispose = test('Any call after dispose throws error', async () => {
    const engine = new EngineController();
    await engine.init({ device: 'cpu' });
    await engine.dispose();

    try {
        await engine.loadModel('base');
        throw new Error('Should have thrown');
    } catch (err) {
        assertEqual(err.code, ErrorCode.INTERNAL_ENGINE_ERROR);
    }
});

// ============================================
// EVENT ORDER TESTS (D2)
// ============================================

const testEventOrder = test('Events emitted in correct order: partial before final', async () => {
    const engine = new EngineController();
    const events = [];

    engine.on(EventType.PARTIAL_TRANSCRIPT, () => events.push('partial'));
    engine.on(EventType.FINAL_TRANSCRIPT, () => events.push('final'));

    await engine.init({ device: 'cpu' });
    await engine.loadModel('base');
    const sessionId = await engine.startSession({});
    await engine.pushAudio(sessionId, new Float32Array(1600));

    // Wait for partial
    await new Promise(r => setTimeout(r, 400));

    await engine.endSession(sessionId);

    // Verify order
    const partialIndex = events.indexOf('partial');
    const finalIndex = events.indexOf('final');
    assert(partialIndex >= 0, 'Partial event should be emitted');
    assert(finalIndex >= 0, 'Final event should be emitted');
    assert(partialIndex < finalIndex, 'Partial should come before final');

    await engine.dispose();
});

const testNoEventsAfterDispose = test('No events after dispose', async () => {
    const engine = new EngineController();
    let eventsAfterDispose = 0;

    await engine.init({ device: 'cpu' });
    await engine.loadModel('base');
    const sessionId = await engine.startSession({});
    await engine.pushAudio(sessionId, new Float32Array(1600));

    await engine.dispose();

    // Add listeners after dispose
    engine.on(EventType.PARTIAL_TRANSCRIPT, () => eventsAfterDispose++);
    engine.on(EventType.FINAL_TRANSCRIPT, () => eventsAfterDispose++);

    // Wait and check
    await new Promise(r => setTimeout(r, 500));
    assertEqual(eventsAfterDispose, 0, 'No events should be emitted after dispose');
});

// ============================================
// RUN ALL TESTS
// ============================================

async function runAllTests() {
    console.log('\n========================================');
    console.log('WisprFlex Engine Controller - Test Suite');
    console.log('Phase 2.1 Verification');
    console.log('========================================\n');

    const tests = [
        // Lifecycle
        testInitSuccess,
        testInitFailsWithoutDevice,
        testInitFailsWithGpu,
        testLoadModelSuccess,
        testLoadModelEmitsProgress,
        testUnloadModel,
        testDisposeIsIdempotent,
        testDisposeAfterSession,

        // Sessions
        testStartSessionSuccess,
        testPushAudioEmitsPartial,
        testEndSessionEmitsFinal,

        // Invalid sequences
        testStartSessionBeforeLoadModel,
        testPushAudioAfterEndSession,
        testPushAudioWithWrongSessionId,
        testStartSessionTwice,
        testCallAfterDispose,

        // Event order
        testEventOrder,
        testNoEventsAfterDispose
    ];

    for (const runTest of tests) {
        await runTest();
    }

    console.log('\n========================================');
    console.log(`Results: ${passed} passed, ${failed} failed`);
    console.log('========================================\n');

    if (failed > 0) {
        process.exit(1);
    }
}

// Run if executed directly
if (require.main === module) {
    runAllTests().catch(err => {
        console.error('Test suite crashed:', err);
        process.exit(1);
    });
}

module.exports = { runAllTests };
