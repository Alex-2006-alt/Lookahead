/*
 * What: Search time allocation and periodic node execution abort controller.
 * Why:  Ensures the engine operates within tournament time constraints without overstepping.
 * Layer: Layer 3 (search) - depends on core types.
 */

#ifndef LOOKAHEAD_SEARCH_TIME_MANAGER_H
#define LOOKAHEAD_SEARCH_TIME_MANAGER_H

#include <chrono>
#include <cstdint>
#include <atomic>
#include "core/types.h"

namespace lookahead::search {

struct SearchLimits;

class TimeManager {
public:
    TimeManager() = default;

    // Initializes time allocation given search limits and side to move.
    void init(const SearchLimits& limits, core::Color us,
              const std::atomic<bool>* external_stop = nullptr) noexcept;

    // Checks whether search must terminate immediately.
    [[nodiscard]] bool should_stop(uint64_t nodes) noexcept;

    // Checks if iterative deepening should start a new depth.
    [[nodiscard]] bool can_start_next_depth() const noexcept;

    // Signals an external stop request (e.g. from UCI 'stop').
    void request_stop() noexcept { stop_requested_.store(true, std::memory_order_relaxed); }

    [[nodiscard]] bool is_stopped() const noexcept { return stop_requested_.load(std::memory_order_relaxed); }

    [[nodiscard]] double elapsed_seconds() const noexcept;
    [[nodiscard]] int64_t elapsed_ms() const noexcept;

private:
    std::chrono::time_point<std::chrono::high_resolution_clock> start_time_{};
    int64_t target_time_ms_ = -1;
    int64_t hard_time_limit_ms_ = -1;
    uint64_t max_nodes_ = 0;
    std::atomic<bool> stop_requested_{false};
    const std::atomic<bool>* external_stop_ = nullptr;
    bool time_limited_ = false;
};

}  // namespace lookahead::search

#endif  // LOOKAHEAD_SEARCH_TIME_MANAGER_H
