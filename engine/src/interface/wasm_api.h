/*
 * What: WebAssembly C-API declarations for Lookahead engine.
 * Why:  Provides exported C-linkage functions for Web Workers and JavaScript runtimes.
 * Layer: Layer 4 (interface) - depends on core and interface.
 */

#ifndef LOOKAHEAD_INTERFACE_WASM_API_H
#define LOOKAHEAD_INTERFACE_WASM_API_H

#if defined(__EMSCRIPTEN__)
#include <emscripten.h>
#define LOOKAHEAD_EXPORT EMSCRIPTEN_KEEPALIVE
#else
#define LOOKAHEAD_EXPORT
#endif

#ifdef __cplusplus
extern "C" {
#endif

// Callback signature for engine output lines.
typedef void (*LookaheadOutputCallback)(const char* line);

// Initializes engine state, attack tables, and allocates hash table.
LOOKAHEAD_EXPORT void lookahead_init(int hash_mb);

// Sends a single UCI command or newline-separated command batch.
LOOKAHEAD_EXPORT void lookahead_send_command(const char* command);

// Registers an asynchronous callback function invoked for every engine output line.
LOOKAHEAD_EXPORT void lookahead_set_output_callback(LookaheadOutputCallback callback);

// Alternative polling API: copies next pending output line into buffer (returns 1 on success, 0 if queue empty).
LOOKAHEAD_EXPORT int lookahead_read_output(char* buffer, int max_len);

// Stops active search, joins worker threads, and releases resources.
LOOKAHEAD_EXPORT void lookahead_shutdown();

#ifdef __cplusplus
}
#endif

#endif  // LOOKAHEAD_INTERFACE_WASM_API_H

