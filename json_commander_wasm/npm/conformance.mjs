// Cross-build conformance tests: compare native json-commander output
// against Wasm module output for identical inputs.
//
// Usage:
//   JCMD_NATIVE=path/to/json-commander node conformance.mjs

import { execFileSync } from 'node:child_process';
import { readFileSync } from 'node:fs';
import { dirname, join } from 'node:path';
import { fileURLToPath } from 'node:url';
import {
  init,
  validateSchema,
  generateHelp,
  generateManpage,
  generateConfigSchema,
  parseArgs,
} from './index.js';

const __dirname = dirname(fileURLToPath(import.meta.url));
const PROJECT_ROOT = join(__dirname, '..', '..');

const NATIVE_BIN =
  process.env.JCMD_NATIVE ||
  join(PROJECT_ROOT, 'build-clang-20', 'tools', 'Release', 'json-commander');

// ---------------------------------------------------------------------------
// Test fixtures
// ---------------------------------------------------------------------------

const FIXTURES = [
  {
    name: 'serve (simple, single-command)',
    path: join(PROJECT_ROOT, 'examples', 'serve', 'serve.json'),
    subcommandPaths: [[]],
    parseCases: [
      { argv: ['--verbose', '-p', '3000', '/tmp'] },
      { argv: ['-H', '0.0.0.0'] },
      { argv: [] },
    ],
  },
  {
    name: 'fake-git (subcommands, flag groups, enums)',
    path: join(PROJECT_ROOT, 'examples', 'fake-git', 'fake-git.json'),
    subcommandPaths: [
      [],
      ['init'],
      ['clone'],
      ['commit'],
      ['status'],
      ['pull'],
      ['stash'],
      ['stash', 'push'],
      ['remote'],
      ['remote', 'add'],
    ],
    parseCases: [
      { argv: ['init', '--bare'] },
      { argv: ['clone', '--depth', '1', '--single-branch', 'https://example.com/repo.git'] },
      { argv: ['commit', '-m', 'initial commit', '--all'] },
      { argv: ['status', '--short', '-u', 'all'] },
      { argv: ['pull', '--rebase', '--autostash'] },
      { argv: ['stash', 'push', '-m', 'wip', '-u'] },
      { argv: ['remote', 'add', 'upstream', 'https://example.com/upstream.git'] },
      { argv: ['branch', '-D', 'old-branch'] },
      { argv: ['log', '--oneline', '-n', '10', '--all'] },
      { argv: ['tag', '-a', '-m', 'v1.0', 'v1.0'] },
    ],
  },
  {
    name: 'json-commander (self-hosting, 6 subcommands)',
    path: join(PROJECT_ROOT, 'tools', 'json_commander.json'),
    subcommandPaths: [
      [],
      ['validate'],
      ['config-schema'],
      ['parse'],
      ['help'],
      ['man'],
      ['codegen'],
    ],
    parseCases: [],  // parse subcommands need file paths — skip for conformance
  },
];

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

let passed = 0;
let failed = 0;

function native(subcommand, schemaPath, extraArgs = []) {
  try {
    return execFileSync(NATIVE_BIN, [subcommand, schemaPath, ...extraArgs], {
      encoding: 'utf-8',
      timeout: 10000,
    });
  } catch (e) {
    if (e.stdout) return e.stdout;
    throw e;
  }
}

function test(name, fn) {
  try {
    fn();
    console.log(`  PASS: ${name}`);
    passed++;
  } catch (e) {
    console.log(`  FAIL: ${name}`);
    console.log(`        ${e.message.split('\n')[0]}`);
    failed++;
  }
}

function assertEqual(actual, expected, label) {
  if (actual !== expected) {
    const actualLines = actual.split('\n');
    const expectedLines = expected.split('\n');
    let diffLine = -1;
    for (let i = 0; i < Math.max(actualLines.length, expectedLines.length); i++) {
      if (actualLines[i] !== expectedLines[i]) {
        diffLine = i + 1;
        break;
      }
    }
    throw new Error(
      `${label}: mismatch at line ${diffLine}\n` +
      `        expected: ${JSON.stringify(expectedLines[diffLine - 1]?.slice(0, 120))}\n` +
      `        actual:   ${JSON.stringify(actualLines[diffLine - 1]?.slice(0, 120))}`
    );
  }
}

// ---------------------------------------------------------------------------
// Tests
// ---------------------------------------------------------------------------

await init();
console.log(`Native binary: ${NATIVE_BIN}`);
console.log('');

for (const fixture of FIXTURES) {
  console.log(`--- ${fixture.name} ---`);
  const schemaJson = readFileSync(fixture.path, 'utf-8');

  // Validate
  test(`validateSchema`, () => {
    const result = validateSchema(schemaJson);
    if (!result.success) {
      throw new Error(`Wasm validation failed: ${JSON.stringify(result.errors)}`);
    }
  });

  // Help (plain text) — compare for each subcommand path
  for (const cmdPath of fixture.subcommandPaths) {
    const label = cmdPath.length ? cmdPath.join(' ') : '(root)';
    test(`generateHelp [${label}]`, () => {
      const nativeOutput = native('help', fixture.path, cmdPath);
      const wasmResult = generateHelp(schemaJson, cmdPath);
      if (!wasmResult.success) {
        throw new Error(`Wasm error: ${JSON.stringify(wasmResult.errors)}`);
      }
      assertEqual(wasmResult.text, nativeOutput, 'help text');
    });
  }

  // Man page (groff) — compare for each subcommand path
  for (const cmdPath of fixture.subcommandPaths) {
    const label = cmdPath.length ? cmdPath.join(' ') : '(root)';
    test(`generateManpage [${label}]`, () => {
      const nativeOutput = native('man', fixture.path, cmdPath);
      const wasmResult = generateManpage(schemaJson, cmdPath);
      if (!wasmResult.success) {
        throw new Error(`Wasm error: ${JSON.stringify(wasmResult.errors)}`);
      }
      assertEqual(wasmResult.text, nativeOutput, 'groff source');
    });
  }

  // Config schema — compare for each subcommand path
  for (const cmdPath of fixture.subcommandPaths) {
    const label = cmdPath.length ? cmdPath.join(' ') : '(root)';
    test(`generateConfigSchema [${label}]`, () => {
      const nativeOutput = native('config-schema', fixture.path, cmdPath);
      const wasmResult = generateConfigSchema(schemaJson, cmdPath);
      if (!wasmResult.success) {
        throw new Error(`Wasm error: ${JSON.stringify(wasmResult.errors)}`);
      }
      // Compare as formatted JSON (native outputs dump(2) + newline)
      const wasmFormatted = JSON.stringify(wasmResult.schema, null, 2) + '\n';
      assertEqual(wasmFormatted, nativeOutput, 'config schema');
    });
  }

  // Parse args — compare config output
  // The native tool requires '--' before schema-args to prevent the outer
  // parser from consuming them.
  for (const parseCase of fixture.parseCases) {
    const argStr = parseCase.argv.join(' ');
    test(`parseArgs [${argStr}]`, () => {
      const nativeOutput = native('parse', fixture.path, ['--', ...parseCase.argv]);
      const wasmResult = parseArgs(schemaJson, parseCase.argv, parseCase.env);
      if (!wasmResult.success) {
        throw new Error(`Wasm error: ${JSON.stringify(wasmResult.errors)}`);
      }
      // Compare as formatted JSON (native outputs dump(2) + newline)
      const wasmFormatted = JSON.stringify(wasmResult.config, null, 2) + '\n';
      assertEqual(wasmFormatted, nativeOutput, 'parse config');
    });
  }

  console.log('');
}

// ---------------------------------------------------------------------------
// Summary
// ---------------------------------------------------------------------------

console.log(`Results: ${passed} passed, ${failed} failed`);
if (failed > 0) process.exit(1);
