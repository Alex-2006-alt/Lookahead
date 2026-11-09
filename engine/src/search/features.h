/*
 * What: Search and evaluation algorithmic feature flags and toggles.
 * Why:  Centralizes all search heuristics for ablation testing and benchmark isolation.
 * Layer: Layer 3 (search) - foundational configuration for search behavior.
 */

#ifndef LOOKAHEAD_SEARCH_FEATURES_H
#define LOOKAHEAD_SEARCH_FEATURES_H

namespace lookahead::search {

// Algorithmic feature toggles (all enabled by default in production).
struct SearchFeatures {
    bool use_transposition_table = true;
    bool use_quiescence_search   = true;
    bool use_move_ordering       = true;
    bool use_null_move_pruning   = true;
    bool use_late_move_reductions = true;
    bool use_reverse_futility    = true;
    bool use_aspiration_windows  = true;
};

// Global active search configuration.
inline SearchFeatures active_features;

// Resets all feature flags to their default active state.
inline void reset_features_to_default() noexcept {
    active_features = SearchFeatures{};
}

}  // namespace lookahead::search

#endif  // LOOKAHEAD_SEARCH_FEATURES_H
