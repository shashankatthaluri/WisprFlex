/**
 * WisprFlex Phase 2.1 — Comprehensive Validation Suite
 * 
 * Agent D: QA/Verification Engineer
 * 
 * Purpose: Verify Phase 2.1 correctness before ML is introduced.
 * This test suite validates BOTH EngineController and AsyncNativeBridge
 * against ENGINE_API_SPEC.md requirements.
 * 
 * Test Categories:
 * - D1: Lifecycle Tests (valid sequences, invalid sequences, repeated init/dispose)
 * - D2: Event Order Verification (partial before final, no events after dispose)
 * - D3: Error Handling (error codes match spec)
 */

const EngineController = require('../node/EngineController');
const { AsyncNativeBridge, createBridge } = require('../node/native-bridge');
const { EngineState, EventType } = require('../node/types');
const { ErrorCode } = require('../node/errors');

// ============================================
// Test Framework
// ============================================

let passed = 0;
let failed = 0;
const results = [];

function test(category, name, fn, component = 'both') {
    return { category, name, fn, component };
}

async function runTest(testCase) {
    const { category, name, fn, component } = testCase;
    try {
        await fn();
        console.log(`✅ [${category}] ${name}`);
        passed++;
        results.push({ category, name, status: 'PASS', component });
    } catch (err) {
        console.log(`❌ [${category}] ${name}`);
        console.log(`   Error: ${err.message}`);
        if (err.code) console.log(`   Code: ${err.code}`);
        failed++;
        results.push({ category, name, status: 'FAIL', error: err.message, component });
    }
}

function assert(condition, message) {
    if (!condition) throw new Error(message || 'Assertion failed');
}

function assertEqual(actual, expected, message) {
    if (actual !== expected) {
        throw new Error(message || `Expected '${expected}', got '${actual}'`);
    }
}

function assertErrorCode(err, expectedCode) {
    if (!err || err.code !== expectedCode) {
        throw new Error(`Expected error code '${expectedCode}', got '${err?.code}'`);
    }
}

// ============================================
// D1 — Lifecycle Tests (EngineController)
// ============================================

const d1Tests = [
    // Valid sequences
    test('D1', 'EC: init() with valid config', async () => {
        const engine = new EngineController();
        await engine.init({ device: 'cpu' });
        assertEqual(engine.getState(), EngineState.INITIALIZED);
        await engine.dispose();
    }, 'EngineController'),

    test('D1', 'EC: loadModel() after init', async () => {
        const engine = new EngineController();
        await engine.init({ device: 'cpu' });
        await engine.loadModel('base');
        assertEqual(engine.getState(), EngineState.MODEL_LOADED);
        assertEqual(engine.getLoadedModel(), 'base');
        await engine.dispose();
    }, 'EngineController'),

    test('D1', 'EC: startSession() after loadModel', async () => {
        const engine = new EngineController();
        await engine.init({ device: 'cpu' });
        await engine.loadModel('base');
        const sessionId = await engine.startSession({});
        assert(sessionId.startsWith('session_'), 'Invalid sessionId format');
        assertEqual(engine.getState(), EngineState.SESSION_ACTIVE);
        await engine.dispose();
    }, 'EngineController'),

    test('D1', 'EC: Full lifecycle (init → load → session → end → dispose)', async () => {
        const engine = new EngineController();
        await engine.init({ device: 'cpu' });
        await engine.loadModel('small');
        const sessionId = await engine.startSession({ language: 'en' });
        await engine.pushAudio(sessionId, new Float32Array(1600));
        await engine.endSession(sessionId);
        assertEqual(engine.getState(), EngineState.MODEL_LOADED);
        await engine.dispose();
        assertEqual(engine.getState(), EngineState.DISPOSED);
    }, 'EngineController'),

    // Invalid sequences
    test('D1', 'EC: init() without config throws', async () => {
        const engine = new EngineController();
        try {
            await engine.init(null);
            throw new Error('Should have thrown');
        } catch (err) {
            assertErrorCode(err, ErrorCode.ENGINE_INIT_FAILED);
        }
    }, 'EngineController'),

    test('D1', 'EC: init() with GPU throws DEVICE_NOT_SUPPORTED', async () => {
        const engine = new EngineController();
        try {
            await engine.init({ device: 'gpu' });
            throw new Error('Should have thrown');
        } catch (err) {
            assertErrorCode(err, ErrorCode.DEVICE_NOT_SUPPORTED);
        }
    }, 'EngineController'),

    test('D1', 'EC: startSession() before loadModel throws MODEL_NOT_LOADED', async () => {
        const engine = new EngineController();
        await engine.init({ device: 'cpu' });
        try {
            await engine.startSession({});
            throw new Error('Should have thrown');
        } catch (err) {
            assertErrorCode(err, ErrorCode.MODEL_NOT_LOADED);
        }
        await engine.dispose();
    }, 'EngineController'),

    test('D1', 'EC: Double startSession() throws SESSION_ALREADY_ACTIVE', async () => {
        const engine = new EngineController();
        await engine.init({ device: 'cpu' });
        await engine.loadModel('base');
        await engine.startSession({});
        try {
            await engine.startSession({});
            throw new Error('Should have thrown');
        } catch (err) {
            assertErrorCode(err, ErrorCode.SESSION_ALREADY_ACTIVE);
        }
        await engine.dispose();
    }, 'EngineController'),

    test('D1', 'EC: pushAudio() with wrong sessionId throws INVALID_SESSION', async () => {
        const engine = new EngineController();
        await engine.init({ device: 'cpu' });
        await engine.loadModel('base');
        await engine.startSession({});
        try {
            await engine.pushAudio('wrong_id', new Float32Array(100));
            throw new Error('Should have thrown');
        } catch (err) {
            assertErrorCode(err, ErrorCode.INVALID_SESSION);
        }
        await engine.dispose();
    }, 'EngineController'),

    test('D1', 'EC: pushAudio() after endSession throws SESSION_ENDED', async () => {
        const engine = new EngineController();
        await engine.init({ device: 'cpu' });
        await engine.loadModel('base');
        const sessionId = await engine.startSession({});
        await engine.endSession(sessionId);
        try {
            await engine.pushAudio(sessionId, new Float32Array(100));
            throw new Error('Should have thrown');
        } catch (err) {
            assertErrorCode(err, ErrorCode.SESSION_ENDED);
        }
        await engine.dispose();
    }, 'EngineController'),

    // Repeated operations
    test('D1', 'EC: Repeated init/dispose (10 cycles) no crash', async () => {
        for (let i = 0; i < 10; i++) {
            const engine = new EngineController();
            await engine.init({ device: 'cpu' });
            await engine.loadModel('tiny');
            const sessionId = await engine.startSession({});
            await engine.pushAudio(sessionId, new Float32Array(160));
            await engine.endSession(sessionId);
            await engine.dispose();
        }
        // No crash = pass
    }, 'EngineController'),

    test('D1', 'EC: dispose() is idempotent', async () => {
        const engine = new EngineController();
        await engine.init({ device: 'cpu' });
        await engine.dispose();
        await engine.dispose();
        await engine.dispose();
        // No crash = pass
    }, 'EngineController'),
];

// ============================================
// D1 — Lifecycle Tests (NativeBridge)
// ============================================

const d1BridgeTests = [
    test('D1', 'NB: init() with valid config', async () => {
        const bridge = createBridge();
        await bridge.init({ device: 'cpu' });
        assertEqual(bridge.getState(), EngineState.INITIALIZED);
        await bridge.dispose();
    }, 'NativeBridge'),

    test('D1', 'NB: Full lifecycle', async () => {
        const bridge = createBridge();
        await bridge.init({ device: 'cpu' });
        await bridge.loadModel('base');
        const sessionId = await bridge.startSession({});
        await bridge.pushAudio(sessionId, new Float32Array(1600));
        await bridge.endSession(sessionId);
        await bridge.dispose();
    }, 'NativeBridge'),

    test('D1', 'NB: startSession() before loadModel throws MODEL_NOT_LOADED', async () => {
        const bridge = createBridge();
        await bridge.init({ device: 'cpu' });
        try {
            await bridge.startSession({});
            throw new Error('Should have thrown');
        } catch (err) {
            assertErrorCode(err, ErrorCode.MODEL_NOT_LOADED);
        }
        await bridge.dispose();
    }, 'NativeBridge'),

    test('D1', 'NB: Repeated init/dispose (10 cycles) no crash', async () => {
        for (let i = 0; i < 10; i++) {
            const bridge = createBridge();
            await bridge.init({ device: 'cpu' });
            await bridge.loadModel('tiny');
            await bridge.dispose();
        }
    }, 'NativeBridge'),
];

// ============================================
// D2 — Event Order Verification
// ============================================

const d2Tests = [
    test('D2', 'EC: pushAudio emits partial_transcript', async () => {
        const engine = new EngineController();
        let received = false;
        engine.on(EventType.PARTIAL_TRANSCRIPT, (e) => {
            received = true;
            assert(e.text, 'Missing text');
            assert(typeof e.isStable === 'boolean', 'Missing isStable');
        });
        await engine.init({ device: 'cpu' });
        await engine.loadModel('base');
        const sessionId = await engine.startSession({});
        await engine.pushAudio(sessionId, new Float32Array(1600));
        await new Promise(r => setTimeout(r, 400));
        assert(received, 'partial_transcript not received');
        await engine.dispose();
    }, 'EngineController'),

    test('D2', 'EC: endSession emits final_transcript', async () => {
        const engine = new EngineController();
        let received = false;
        engine.on(EventType.FINAL_TRANSCRIPT, (e) => {
            received = true;
            assert(e.text, 'Missing text');
        });
        await engine.init({ device: 'cpu' });
        await engine.loadModel('base');
        const sessionId = await engine.startSession({});
        await engine.endSession(sessionId);
        assert(received, 'final_transcript not received');
        await engine.dispose();
    }, 'EngineController'),

    test('D2', 'EC: Events in order (partial before final)', async () => {
        const engine = new EngineController();
        const events = [];
        engine.on(EventType.PARTIAL_TRANSCRIPT, () => events.push('partial'));
        engine.on(EventType.FINAL_TRANSCRIPT, () => events.push('final'));
        await engine.init({ device: 'cpu' });
        await engine.loadModel('base');
        const sessionId = await engine.startSession({});
        await engine.pushAudio(sessionId, new Float32Array(1600));
        await new Promise(r => setTimeout(r, 400));
        await engine.endSession(sessionId);
        assert(events.indexOf('partial') < events.indexOf('final'), 'Partial must come before final');
        await engine.dispose();
    }, 'EngineController'),

    test('D2', 'EC: No events after dispose', async () => {
        const engine = new EngineController();
        let eventsAfter = 0;
        await engine.init({ device: 'cpu' });
        await engine.loadModel('base');
        await engine.dispose();
        engine.on(EventType.PARTIAL_TRANSCRIPT, () => eventsAfter++);
        await new Promise(r => setTimeout(r, 300));
        assertEqual(eventsAfter, 0, 'Events emitted after dispose');
    }, 'EngineController'),

    test('D2', 'EC: loadModel emits model_progress (0 to 100)', async () => {
        const engine = new EngineController();
        const progress = [];
        engine.on(EventType.MODEL_PROGRESS, (e) => progress.push(e.progress));
        await engine.init({ device: 'cpu' });
        await engine.loadModel('base');
        assert(progress.length > 0, 'No progress events');
        assert(progress.includes(100), 'Did not reach 100%');
        await engine.dispose();
    }, 'EngineController'),

    test('D2', 'NB: Events in order (partial before final)', async () => {
        const bridge = createBridge();
        const events = [];
        bridge.on(EventType.PARTIAL_TRANSCRIPT, () => events.push('partial'));
        bridge.on(EventType.FINAL_TRANSCRIPT, () => events.push('final'));
        await bridge.init({ device: 'cpu' });
        await bridge.loadModel('base');
        const sessionId = await bridge.startSession({});
        await bridge.pushAudio(sessionId, new Float32Array(1600));
        await new Promise(r => setTimeout(r, 400));
        await bridge.endSession(sessionId);
        await new Promise(r => setTimeout(r, 200));
        assert(events.indexOf('partial') < events.indexOf('final'), 'Partial must come before final');
        await bridge.dispose();
    }, 'NativeBridge'),
];

// ============================================
// D3 — Error Handling Verification
// ============================================

const d3Tests = [
    test('D3', 'Error code: MODEL_NOT_FOUND for invalid model', async () => {
        const engine = new EngineController();
        await engine.init({ device: 'cpu' });
        try {
            await engine.loadModel('nonexistent_model');
            throw new Error('Should have thrown');
        } catch (err) {
            assertErrorCode(err, ErrorCode.MODEL_NOT_FOUND);
        }
        await engine.dispose();
    }, 'EngineController'),

    test('D3', 'Error has recoverable flag', async () => {
        const engine = new EngineController();
        await engine.init({ device: 'cpu' });
        try {
            await engine.loadModel('invalid');
        } catch (err) {
            assert(typeof err.recoverable === 'boolean', 'Missing recoverable flag');
        }
        await engine.dispose();
    }, 'EngineController'),

    test('D3', 'All errors have code, message, recoverable', async () => {
        const engine = new EngineController();
        await engine.init({ device: 'cpu' });
        try {
            await engine.startSession({}); // No model loaded
        } catch (err) {
            assert(err.code, 'Missing code');
            assert(err.message, 'Missing message');
            assert(typeof err.recoverable === 'boolean', 'Missing recoverable');
        }
        await engine.dispose();
    }, 'EngineController'),
];

// ============================================
// Run All Tests
// ============================================

async function runAllTests() {
    console.log('\n' + '='.repeat(60));
    console.log('WisprFlex Phase 2.1 — Comprehensive Validation Suite');
    console.log('Agent D: QA/Verification Engineer');
    console.log('='.repeat(60) + '\n');

    console.log('--- D1: Lifecycle Tests (EngineController) ---\n');
    for (const t of d1Tests) await runTest(t);

    console.log('\n--- D1: Lifecycle Tests (NativeBridge) ---\n');
    for (const t of d1BridgeTests) await runTest(t);

    console.log('\n--- D2: Event Order Verification ---\n');
    for (const t of d2Tests) await runTest(t);

    console.log('\n--- D3: Error Handling ---\n');
    for (const t of d3Tests) await runTest(t);

    // Summary
    console.log('\n' + '='.repeat(60));
    console.log('VALIDATION SUMMARY');
    console.log('='.repeat(60));
    console.log(`Total Tests: ${passed + failed}`);
    console.log(`Passed: ${passed}`);
    console.log(`Failed: ${failed}`);
    console.log('');

    // Go/No-Go Decision
    const decision = failed === 0 ? 'GO' : 'NO-GO';
    console.log(`Phase 2.2 Decision: ${decision}`);
    console.log('='.repeat(60) + '\n');

    return { passed, failed, decision, results };
}

if (require.main === module) {
    runAllTests().then(({ decision }) => {
        process.exit(decision === 'GO' ? 0 : 1);
    }).catch(err => {
        console.error('Validation suite crashed:', err);
        process.exit(1);
    });
}

module.exports = { runAllTests };
