// @json-commander/wasm — JS wrapper around the Emscripten-generated module.
//
// Usage:
//   import { init, validateSchema, generateHelp, ... } from '@json-commander/wasm';
//   await init();
//   const result = validateSchema('{ ... }');

import createJsonCommanderModule from './json_commander_wasm.js';

let wasmModule = null;

function ensureInitialized() {
  if (!wasmModule) {
    throw new Error(
      '@json-commander/wasm: call init() and await it before using any functions'
    );
  }
}

function callWasm(fnName, input) {
  ensureInitialized();
  const resultJson = wasmModule[fnName](JSON.stringify(input));
  return JSON.parse(resultJson);
}

export async function init() {
  if (wasmModule) return;
  wasmModule = await createJsonCommanderModule();
}

export function validateSchema(schemaJson) {
  return callWasm('validateSchema', { schemaJson });
}

export function generateHelp(schemaJson, commandPath) {
  const input = { schemaJson };
  if (commandPath) input.commandPath = commandPath;
  return callWasm('generateHelp', input);
}

export function generateManpage(schemaJson, commandPath) {
  const input = { schemaJson };
  if (commandPath) input.commandPath = commandPath;
  return callWasm('generateManpage', input);
}

export function generateConfigSchema(schemaJson, commandPath) {
  const input = { schemaJson };
  if (commandPath) input.commandPath = commandPath;
  return callWasm('generateConfigSchema', input);
}

export function parseArgs(schemaJson, argv, env) {
  const input = { schemaJson, argv };
  if (env) input.env = env;
  return callWasm('parseArgs', input);
}
