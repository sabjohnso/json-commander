// Smoke tests for @json-commander/wasm
import {
  init,
  validateSchema,
  generateHelp,
  generateManpage,
  generateConfigSchema,
  parseArgs,
} from './index.js';

const SCHEMA = JSON.stringify({
  name: 'test-app',
  version: '1.0.0',
  doc: ['A test application.'],
  args: [
    {
      kind: 'flag',
      names: ['verbose', 'v'],
      doc: ['Enable verbose output.'],
    },
    {
      kind: 'option',
      names: ['output', 'o'],
      type: 'string',
      doc: ['Output file path.'],
    },
    {
      kind: 'positional',
      name: 'input',
      type: 'string',
      doc: ['Input file.'],
    },
  ],
});

let passed = 0;
let failed = 0;

function test(name, fn) {
  try {
    fn();
    console.log(`  PASS: ${name}`);
    passed++;
  } catch (e) {
    console.log(`  FAIL: ${name}`);
    console.log(`        ${e.message}`);
    failed++;
  }
}

function assert(condition, message) {
  if (!condition) throw new Error(message || 'Assertion failed');
}

function assertEqual(actual, expected, message) {
  if (actual !== expected) {
    throw new Error(
      `${message || 'Assertion failed'}: expected ${JSON.stringify(expected)}, got ${JSON.stringify(actual)}`
    );
  }
}

// Initialize
await init();
console.log('Tests:');

// --- validateSchema ---

test('validateSchema: valid schema succeeds', () => {
  const result = validateSchema(SCHEMA);
  assert(result.success, 'Expected success');
});

test('validateSchema: invalid schema returns errors', () => {
  const result = validateSchema('{"name": "x"}');
  assertEqual(result.success, false, 'Expected failure');
  assert(result.errors.length > 0, 'Expected at least one error');
});

test('validateSchema: malformed JSON returns error', () => {
  const result = validateSchema('{not json}');
  assertEqual(result.success, false, 'Expected failure');
  assert(result.errors[0].message.includes('Invalid JSON'), 'Expected JSON error');
});

test('validateSchema: empty string returns error', () => {
  const result = validateSchema('');
  assertEqual(result.success, false, 'Expected failure');
});

// --- generateHelp ---

test('generateHelp: produces plain text', () => {
  const result = generateHelp(SCHEMA);
  assert(result.success, 'Expected success');
  assert(result.text.length > 0, 'Expected non-empty text');
  assert(result.text.includes('test-app'), 'Expected app name in help');
});

test('generateHelp: invalid schema returns error', () => {
  const result = generateHelp('{"bad": true}');
  assertEqual(result.success, false, 'Expected failure');
});

// --- generateManpage ---

test('generateManpage: produces groff source', () => {
  const result = generateManpage(SCHEMA);
  assert(result.success, 'Expected success');
  assert(result.text.includes('.TH'), 'Expected groff .TH macro');
});

// --- generateConfigSchema ---

test('generateConfigSchema: produces JSON Schema', () => {
  const result = generateConfigSchema(SCHEMA);
  assert(result.success, 'Expected success');
  assert(result.schema['$schema'], 'Expected $schema field');
  assert(result.schema.properties, 'Expected properties field');
});

// --- parseArgs ---

test('parseArgs: parses valid args', () => {
  const result = parseArgs(SCHEMA, ['--verbose', '-o', 'out.txt', 'in.txt']);
  assert(result.success, `Expected success, got: ${JSON.stringify(result)}`);
  assertEqual(result.config.verbose, true, 'Expected verbose=true');
  assertEqual(result.config.output, 'out.txt', 'Expected output=out.txt');
  assertEqual(result.config.input, 'in.txt', 'Expected input=in.txt');
});

test('parseArgs: uses env variables', () => {
  const envSchema = JSON.stringify({
    name: 'env-test',
    version: '1.0.0',
    doc: ['Test env vars.'],
    args: [
      {
        kind: 'option',
        names: ['level'],
        type: 'string',
        env: 'MY_LEVEL',
        doc: ['Log level.'],
      },
    ],
  });
  const result = parseArgs(envSchema, [], { MY_LEVEL: 'debug' });
  assert(result.success, `Expected success, got: ${JSON.stringify(result)}`);
  assertEqual(result.config.level, 'debug', 'Expected level from env');
});

test('parseArgs: rejects --help', () => {
  const result = parseArgs(SCHEMA, ['--help']);
  assertEqual(result.success, false, 'Expected failure for --help');
  assert(
    result.errors[0].message.includes('generateHelp'),
    'Expected guidance to use generateHelp()'
  );
});

test('parseArgs: rejects --version', () => {
  const result = parseArgs(SCHEMA, ['--version']);
  assertEqual(result.success, false, 'Expected failure for --version');
});

test('parseArgs: rejects --man', () => {
  const result = parseArgs(SCHEMA, ['--man']);
  assertEqual(result.success, false, 'Expected failure for --man');
});

test('parseArgs: unknown option returns error', () => {
  const result = parseArgs(SCHEMA, ['--unknown']);
  assertEqual(result.success, false, 'Expected failure');
});

// --- Summary ---

console.log(`\nResults: ${passed} passed, ${failed} failed`);
if (failed > 0) process.exit(1);
