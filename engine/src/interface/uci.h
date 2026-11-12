/*
 * What: Universal Chess Interface (UCI) protocol interpreter and command parser.
 * Why:  Provides standard communication protocol connecting Lookahead to chess GUIs.
 * Layer: Layer 4 (interface) - coordinates search, movegen, and core position state.
 */

#ifndef LOOKAHEAD_INTERFACE_UCI_H
#define LOOKAHEAD_INTERFACE_UCI_H

#include <iostream>
#include <string>
#include <string_view>
#include <thread>
#include <atomic>
#include <mutex>
#include "core/types.h"
#include "core/position.h"
#include "search/search.h"

namespace lookahead::interface {

// Manages UCI protocol state machine and standard I/O interaction.
class UciHandler {
public:
    explicit UciHandler(std::istream& in = std::cin, std::ostream& out = std::cout);
    ~UciHandler();

    // Runs the main blocking UCI command loop until 'quit' or EOF.
    void loop();

    // Processes a single line UCI command.
    void handle_command(std::string_view line);

    // Stops any active search and waits for the search worker to finish.
    void stop();

    // Current internal board state query.
    [[nodiscard]] const core::Position& position() const noexcept { return pos_; }

    [[nodiscard]] bool is_running() const noexcept { return running_; }
    [[nodiscard]] bool is_searching() const noexcept { return searching_.load(std::memory_order_relaxed); }

private:
    void handle_position(std::string_view args);
    void handle_go(std::string_view args);
    void handle_setoption(std::string_view args);
    void stop_and_join() noexcept;

    std::istream& in_;
    std::ostream& out_;
    core::Position pos_;
    bool running_ = true;

    // Asynchronous search worker state
    std::atomic<bool> searching_{false};
    std::atomic<bool> stop_signal_{false};
    std::thread search_thread_;
    std::mutex out_mutex_;
};

}  // namespace lookahead::interface

#endif  // LOOKAHEAD_INTERFACE_UCI_H
