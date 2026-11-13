/*
 * What: Implementation of WebAssembly C-API bridge.
 * Why:  Connects browser JavaScript and Web Worker runtimes to the Lookahead UCI interpreter.
 * Layer: Layer 4 (interface) - depends on wasm_api.h, uci.h, and movegen/attacks.
 */

#include "wasm_api.h"
#include <iostream>
#include <sstream>
#include <string>
#include <deque>
#include <mutex>
#include <memory>
#include <cstring>
#include <algorithm>
#include "interface/uci.h"
#include "movegen/attacks.h"
#include "search/transposition_table.h"

namespace {

class WasmOutputStreambuf : public std::streambuf {
public:
    WasmOutputStreambuf(std::mutex& mutex,
                        std::deque<std::string>& queue,
                        LookaheadOutputCallback& callback)
        : mutex_(mutex), queue_(queue), callback_(callback) {}

protected:
    int_type overflow(int_type ch) override {
        if (ch != traits_type::eof()) {
            if (ch == '\n') {
                flush_line();
            } else if (ch != '\r') {
                current_line_ += static_cast<char>(ch);
            }
        }
        return ch;
    }

    int sync() override {
        if (!current_line_.empty()) {
            flush_line();
        }
        return 0;
    }

private:
    void flush_line() {
        std::lock_guard<std::mutex> lock(mutex_);
        queue_.push_back(current_line_);
        if (callback_) {
            callback_(current_line_.c_str());
        }
        current_line_.clear();
    }

    std::mutex& mutex_;
    std::deque<std::string>& queue_;
    LookaheadOutputCallback& callback_;
    std::string current_line_;
};

struct EngineContext {
    std::mutex mutex;
    std::deque<std::string> queue;
    LookaheadOutputCallback callback = nullptr;
    std::unique_ptr<WasmOutputStreambuf> streambuf;
    std::unique_ptr<std::ostream> out_stream;
    std::istringstream dummy_in;
    std::unique_ptr<lookahead::interface::UciHandler> uci;
};

std::unique_ptr<EngineContext> g_ctx;
std::mutex g_init_mutex;

}  // namespace

extern "C" {

void lookahead_init(int hash_mb) {
    std::lock_guard<std::mutex> init_lock(g_init_mutex);
    if (!g_ctx) {
        lookahead::movegen::attacks::init();
        g_ctx = std::make_unique<EngineContext>();
        g_ctx->streambuf = std::make_unique<WasmOutputStreambuf>(
            g_ctx->mutex, g_ctx->queue, g_ctx->callback);
        g_ctx->out_stream = std::make_unique<std::ostream>(g_ctx->streambuf.get());
        g_ctx->uci = std::make_unique<lookahead::interface::UciHandler>(
            g_ctx->dummy_in, *g_ctx->out_stream);
    }
    const int mb = (hash_mb > 0) ? hash_mb : 16;
    lookahead::search::global_tt.resize(mb);
}

void lookahead_send_command(const char* command) {
    if (!command) return;
    if (!g_ctx) lookahead_init(16);

    std::istringstream stream(command);
    std::string line;
    while (std::getline(stream, line)) {
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        if (!line.empty()) {
            g_ctx->uci->handle_command(line);
        }
    }
}

void lookahead_set_output_callback(LookaheadOutputCallback callback) {
    if (!g_ctx) lookahead_init(16);
    std::lock_guard<std::mutex> lock(g_ctx->mutex);
    g_ctx->callback = callback;
}

int lookahead_read_output(char* buffer, int max_len) {
    if (!buffer || max_len <= 0 || !g_ctx) return 0;

    std::lock_guard<std::mutex> lock(g_ctx->mutex);
    if (g_ctx->queue.empty()) return 0;

    const std::string line = std::move(g_ctx->queue.front());
    g_ctx->queue.pop_front();

    const size_t copy_len = std::min(line.size(), static_cast<size_t>(max_len - 1));
    std::memcpy(buffer, line.data(), copy_len);
    buffer[copy_len] = '\0';
    return 1;
}

void lookahead_shutdown() {
    std::lock_guard<std::mutex> init_lock(g_init_mutex);
    if (g_ctx) {
        if (g_ctx->uci) {
            g_ctx->uci->stop();
        }
        g_ctx.reset();
    }
}

}  // extern "C"

