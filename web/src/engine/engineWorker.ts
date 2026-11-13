/**
 * Web Worker for Lookahead Chess Engine WebAssembly instance.
 *
 * Runs search calculations asynchronously off the main thread,
 * managing the WASM module lifecycle and streaming telemetry via postMessage.
 */

import { WorkerInMessage, WorkerOutMessage } from './types';
import { parseUciInfoLine, parseBestMoveLine } from './uciParser';

interface LookaheadWasmModule {
  _lookahead_init: (hashMb: number) => void;
  _lookahead_send_command: (cmdPtr: number) => void;
  _lookahead_set_output_callback: (cbPtr: number) => void;
  _lookahead_shutdown: () => void;
  cwrap?: (name: string, returnType: string | null, argTypes: string[]) => (...args: unknown[]) => unknown;
  addFunction?: (fn: (ptr: number) => void, sig: string) => number;
  UTF8ToString?: (ptr: number) => string;
  allocateUTF8?: (str: string) => number;
  _free?: (ptr: number) => void;
}

let wasmModule: LookaheadWasmModule | null = null;
let sendCommandFn: ((cmd: string) => void) | null = null;
let isInitialized = false;

function postOut(msg: WorkerOutMessage) {
  self.postMessage(msg);
}

function handleEngineOutput(line: string) {
  postOut({ type: 'uci', line });

  const info = parseUciInfoLine(line);
  if (info) {
    postOut({ type: 'info', info });
  }

  const bm = parseBestMoveLine(line);
  if (bm) {
    postOut({ type: 'bestmove', bestMove: bm.bestMove, ponderMove: bm.ponderMove });
    postOut({ type: 'status', status: 'ready' });
  }
}

async function initEngine(hashMb = 16) {
  if (isInitialized) return;

  postOut({ type: 'status', status: 'loading', message: 'Loading WebAssembly engine...' });

  try {
    // Attempt dynamic import of the Emscripten factory from /lookahead.js or global
    let factory: (() => Promise<LookaheadWasmModule>) | undefined;

    // Check if script was loaded or can be imported
    if (typeof (self as unknown as { LookaheadModule?: () => Promise<LookaheadWasmModule> }).LookaheadModule === 'function') {
      factory = (self as unknown as { LookaheadModule: () => Promise<LookaheadWasmModule> }).LookaheadModule;
    } else {
      try {
        const importUrl = '/lookahead.js';
        const dynamicImport = new Function('url', 'return import(url)');
        const mod = await dynamicImport(importUrl);
        factory = mod.default || mod.LookaheadModule;
      } catch {
        // Fallback when /lookahead.js is not yet copied to public/
      }
    }

    if (factory) {
      wasmModule = await factory();
      if (wasmModule.cwrap) {
        const initFn = wasmModule.cwrap('lookahead_init', null, ['number']) as (mb: number) => void;
        initFn(hashMb);
        const rawSend = wasmModule.cwrap('lookahead_send_command', null, ['string']) as (cmd: string) => void;
        sendCommandFn = (cmd: string) => rawSend(cmd + '\n');
      } else {
        wasmModule._lookahead_init(hashMb);
        sendCommandFn = (cmd: string) => {
          if (wasmModule?.allocateUTF8 && wasmModule._free) {
            const ptr = wasmModule.allocateUTF8(cmd + '\n');
            wasmModule._lookahead_send_command(ptr);
            wasmModule._free(ptr);
          }
        };
      }

      if (wasmModule.addFunction && wasmModule._lookahead_set_output_callback) {
        const cbPtr = wasmModule.addFunction((linePtr: number) => {
          const line = wasmModule?.UTF8ToString ? wasmModule.UTF8ToString(linePtr) : '';
          handleEngineOutput(line);
        }, 'vi');
        wasmModule._lookahead_set_output_callback(cbPtr);
      }

      // Initial UCI handshake
      sendCommandFn('uci');
      sendCommandFn('isready');

      isInitialized = true;
      postOut({ type: 'status', status: 'ready' });
    } else {
      console.warn('[EngineWorker] WASM binary not yet present at /lookahead.js; running in stand-by mode.');
      postOut({ type: 'status', status: 'ready', message: 'WASM engine ready (stand-by mode).' });
    }
  } catch (err) {
    const errorMsg = err instanceof Error ? err.message : String(err);
    console.error('[EngineWorker] Initialization failed:', errorMsg);
    postOut({ type: 'status', status: 'error', message: errorMsg });
  }
}

self.onmessage = async (e: MessageEvent<WorkerInMessage>) => {
  const msg = e.data;

  switch (msg.type) {
    case 'init':
      await initEngine(msg.hashMb);
      break;

    case 'command':
      if (sendCommandFn) {
        sendCommandFn(msg.cmd);
      }
      break;

    case 'newgame':
      if (sendCommandFn) {
        sendCommandFn('ucinewgame');
        sendCommandFn('isready');
      }
      break;

    case 'position': {
      if (!sendCommandFn) break;
      let cmd = 'position ';
      if (msg.fen && msg.fen !== 'startpos') {
        cmd += `fen ${msg.fen}`;
      } else {
        cmd += 'startpos';
      }
      if (msg.moves && msg.moves.length > 0) {
        cmd += ` moves ${msg.moves.join(' ')}`;
      }
      sendCommandFn(cmd);
      break;
    }

    case 'go': {
      if (!sendCommandFn) break;
      postOut({ type: 'status', status: 'searching' });
      let cmd = 'go';
      if (msg.depth) {
        cmd += ` depth ${msg.depth}`;
      } else if (msg.movetime) {
        cmd += ` movetime ${msg.movetime}`;
      } else {
        cmd += ' depth 6';
      }
      sendCommandFn(cmd);
      break;
    }

    case 'stop':
      if (sendCommandFn) {
        sendCommandFn('stop');
      }
      break;

    case 'setoption':
      if (sendCommandFn) {
        sendCommandFn(`setoption name ${msg.name} value ${msg.value}`);
      }
      break;
  }
};
