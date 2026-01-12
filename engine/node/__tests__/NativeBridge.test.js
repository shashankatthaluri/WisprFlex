/**
 * WisprFlex Native Bridge - Test Suite
 * 
 * Verifies Agent C exit criteria:
 * - Node can call native without blocking
 * - Native can emit events to Node
 * - Clean teardown without memory leaks
 */

const { AsyncNativeBridge, createBridge } = require('../native-bridge');
const { EventType } = require('../types');

// Test framework
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
            failed++;
        }
    };
}

function assert(condition, message) {
    if (!condition) throw new Error(message || 'Assertion failed');
}

function assertEqual(actual, expected, message) {
    if (actual !== expected) {
        throw new Error(message || `Expected ${expected}, got ${actual}`);
    }
}

// ============================================
// Non-Blocking Tests
// ============================================

const testInitNonBlocking = test('init() is non-blocking', async () => {
    const bridge = createBridge();
    const start = Date.now();
    await bridge.init({ device: 'cpu' });
    const elapsed = Date.now() - start;

    // Should complete quickly (work is async)
    assert(elapsed < 500, `init took ${elapsed}ms, expected < 500ms`);
    await bridge.dispose();
});

const testLoadModelNonBlocking = test('loadModel() is non-blocking', async () => {
    const bridge = createBridge();
    await bridge.init({ device: 'cpu' });

    const start = Date.now();
    const loadPromise = bridge.loadModel('base');
    const immediateElapsed = Date.now() - start;

    // Should return Promise immediately
    assert(immediateElapsed < 50, `loadModel didn't return immediately`);

    // Wait for completion
    await loadPromise;
    await bridge.dispose();
});

const testPushAudioNonBlocking = test('pushAudio() is non-blocking', async () => {
    const bridge = createBridge();
    await bridge.init({ device: 'cpu' });
    await bridge.loadModel('base');
    const sessionId = await bridge.startSession({});

    const audio = new Float32Array(1600);

    const start = Date.now();
    await bridge.pushAudio(sessionId, audio);
    const elapsed = Date.now() - start;

    // Should complete quickly (real work is async)
    assert(elapsed < 500, `pushAudio blocked for ${elapsed}ms`);

    await bridge.dispose();
});

// ============================================
// Event Callback Tests
// ============================================

const testPartialTranscriptEvent = test('Partial transcript events received', async () => {
    const bridge = createBridge();
    let eventReceived = false;

    bridge.on(EventType.PARTIAL_TRANSCRIPT, (event) => {
        eventReceived = true;
        assert(event.type === EventType.PARTIAL_TRANSCRIPT, 'Wrong event type');
        assert(event.text, 'Missing text');
        assert(event.sessionId, 'Missing sessionId');
        assert(typeof event.isStable === 'boolean', 'Missing isStable');
    });

    await bridge.init({ device: 'cpu' });
    await bridge.loadModel('base');
    const sessionId = await bridge.startSession({});
    await bridge.pushAudio(sessionId, new Float32Array(1600));

    // Wait for async event
    await new Promise(r => setTimeout(r, 350));

    assert(eventReceived, 'Partial transcript event not received');
    await bridge.dispose();
});

const testFinalTranscriptEvent = test('Final transcript event received', async () => {
    const bridge = createBridge();
    let finalReceived = false;

    bridge.on(EventType.FINAL_TRANSCRIPT, (event) => {
        finalReceived = true;
        assert(event.type === EventType.FINAL_TRANSCRIPT, 'Wrong event type');
        assert(event.text, 'Missing text');
        assert(event.sessionId, 'Missing sessionId');
    });

    await bridge.init({ device: 'cpu' });
    await bridge.loadModel('base');
    const sessionId = await bridge.startSession({});
    await bridge.endSession(sessionId);

    // Wait for async event
    await new Promise(r => setTimeout(r, 200));

    assert(finalReceived, 'Final transcript event not received');
    await bridge.dispose();
});

const testModelProgressEvents = test('Model progress events received', async () => {
    const bridge = createBridge();
    const progressEvents = [];

    bridge.on(EventType.MODEL_PROGRESS, (event) => {
        progressEvents.push(event.progress);
    });

    await bridge.init({ device: 'cpu' });
    await bridge.loadModel('tiny');

    // Wait for async events
    await new Promise(r => setTimeout(r, 600));

    assert(progressEvents.length > 0, 'No progress events');
    assert(progressEvents.includes(100), 'Did not reach 100%');

    await bridge.dispose();
});

// ============================================
// Clean Teardown Tests
// ============================================

const testCleanTeardown = test('Dispose cleans up resources', async () => {
    const bridge = createBridge();
    await bridge.init({ device: 'cpu' });
    await bridge.loadModel('base');
    const sessionId = await bridge.startSession({});

    // Dispose should not throw
    await bridge.dispose();

    assertEqual(bridge.getState(), 'DISPOSED', 'State should be DISPOSED');
    assertEqual(bridge.getLoadedModel(), null, 'Model should be null');
    assertEqual(bridge.getActiveSession(), null, 'Session should be null');
});

const testNoEventsAfterDispose = test('No events after dispose', async () => {
    const bridge = createBridge();
    let eventsAfterDispose = 0;

    await bridge.init({ device: 'cpu' });
    await bridge.loadModel('base');

    await bridge.dispose();

    // Add listener after dispose
    bridge.on(EventType.PARTIAL_TRANSCRIPT, () => eventsAfterDispose++);

    // Wait
    await new Promise(r => setTimeout(r, 300));

    assertEqual(eventsAfterDispose, 0, 'Events emitted after dispose');
});

const testDisposeIdempotent = test('Dispose is idempotent', async () => {
    const bridge = createBridge();
    await bridge.init({ device: 'cpu' });

    await bridge.dispose();
    await bridge.dispose(); // Should not throw
    await bridge.dispose(); // Should not throw
});

// ============================================
// Event Order Tests
// ============================================

const testEventOrder = test('Events in correct order: partial before final', async () => {
    const bridge = createBridge();
    const events = [];

    bridge.on(EventType.PARTIAL_TRANSCRIPT, () => events.push('partial'));
    bridge.on(EventType.FINAL_TRANSCRIPT, () => events.push('final'));

    await bridge.init({ device: 'cpu' });
    await bridge.loadModel('base');
    const sessionId = await bridge.startSession({});
    await bridge.pushAudio(sessionId, new Float32Array(1600));

    // Wait for partial
    await new Promise(r => setTimeout(r, 350));

    await bridge.endSession(sessionId);

    // Wait for final
    await new Promise(r => setTimeout(r, 200));

    const partialIndex = events.indexOf('partial');
    const finalIndex = events.indexOf('final');

    assert(partialIndex >= 0, 'Partial event missing');
    assert(finalIndex >= 0, 'Final event missing');
    assert(partialIndex < finalIndex, 'Partial should come before final');

    await bridge.dispose();
});

// ============================================
// Run All Tests
// ============================================

async function runAllTests() {
    console.log('\n========================================');
    console.log('WisprFlex Native Bridge - Test Suite');
    console.log('Agent C Exit Criteria Verification');
    console.log('========================================\n');

    const tests = [
        // Non-blocking
        testInitNonBlocking,
        testLoadModelNonBlocking,
        testPushAudioNonBlocking,

        // Events
        testPartialTranscriptEvent,
        testFinalTranscriptEvent,
        testModelProgressEvents,

        // Teardown
        testCleanTeardown,
        testNoEventsAfterDispose,
        testDisposeIdempotent,

        // Order
        testEventOrder
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

if (require.main === module) {
    runAllTests().catch(err => {
        console.error('Test suite crashed:', err);
        process.exit(1);
    });
}

module.exports = { runAllTests };
