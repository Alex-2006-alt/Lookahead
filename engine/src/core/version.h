/*
 * What: Version definitions and engine metadata for Lookahead.
 * Why:  Centralizes semantic versioning and identity strings in one header.
 * Layer: Layer 0 (core) - foundational configuration with no dependencies.
 */

#ifndef LOOKAHEAD_CORE_VERSION_H
#define LOOKAHEAD_CORE_VERSION_H

#include <string_view>

namespace lookahead::core {

inline constexpr int VERSION_MAJOR = 0;
inline constexpr int VERSION_MINOR = 1;
inline constexpr int VERSION_PATCH = 0;

inline constexpr std::string_view ENGINE_VERSION = "0.1.0";
inline constexpr std::string_view ENGINE_NAME = "Lookahead";
inline constexpr std::string_view ENGINE_AUTHOR = "Alex";

}  // namespace lookahead::core

#endif  // LOOKAHEAD_CORE_VERSION_H
