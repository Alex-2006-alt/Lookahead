/*
 * What: Implementation of tournament time allocation and search abortion logic.
 * Why:  Balances deep exploration with strict time conservation.
 * Layer: Layer 3 (search) - depends on time_manager.h and search.h.
 */

#include "time_manager.h"
#include <algorithm>
#include "search.h"

namespace lookahead::search {

void TimeManager::init(const SearchLimits& limits, core::Color us) noexcept {
    start_time_ = std::chrono::high_resolution_clock::now();
    stop_requested_.store(false, std::memory_order_relaxed);
    max_nodes_ = limits.max_nodes;
    time_limited_ = false;
    target_time_ms_ = -1;
    hard_time_limit_ms_ = -1;

    if (limits.move_time_ms > 0) {
        time_limited_ = true;
        target_time_ms_ = limits.move_time_ms;
        hard_time_limit_ms_ = limits.move_time_ms;
    } else {
        const int my_time = (us == core::Color::White) ? limits.wtime_ms : limits.btime_ms;
        const int my_inc = (us == core::Color::White) ? limits.winc_ms : limits.binc_ms;

        if (my_time > 0) {
            time_limited_ = true;
            const int mtg = (limits.moves_to_go > 0) ? std::min(limits.moves_to_go, 40) : 25;
            int64_t target = (my_time / mtg) + (my_inc * 3 / 4);
            target = std::max<int64_t>(10, target);

            int64_t hard = std::min<int64_t>(my_time - 50, target * 3);
            hard = std::max<int64_t>(10, hard);

            target_time_ms_ = target;
            hard_time_limit_ms_ = hard;
        }
    }
}

bool TimeManager::should_stop(uint64_t nodes) noexcept {
    if (stop_requested_.load(std::memory_order_relaxed)) {
        return true;
    }

    if (max_nodes_ > 0 && nodes >= max_nodes_) {
        stop_requested_.store(true, std::memory_order_relaxed);
        return true;
    }

    // Check clock periodically every 2048 nodes
    if (time_limited_ && (nodes & 2047) == 0) {
        if (elapsed_ms() >= hard_time_limit_ms_) {
            stop_requested_.store(true, std::memory_order_relaxed);
            return true;
        }
    }

    return false;
}

bool TimeManager::can_start_next_depth() const noexcept {
    if (stop_requested_.load(std::memory_order_relaxed)) {
        return false;
    }
    if (!time_limited_) {
        return true;
    }
    return elapsed_ms() < (target_time_ms_ / 2);
}

int64_t TimeManager::elapsed_ms() const noexcept {
    const auto now = std::chrono::high_resolution_clock::now();
    return std::chrono::duration_cast<std::chrono::milliseconds>(now - start_time_).count();
}

double TimeManager::elapsed_seconds() const noexcept {
    const auto now = std::chrono::high_resolution_clock::now();
    const std::chrono::duration<double> dur = now - start_time_;
    return dur.count();
}

}  // namespace lookahead::search
