#pragma once

#include <cstddef>
#include <utility>

namespace vt100_utils {
// Active capability probes. On POSIX they briefly switch stdin to raw mode
// and consume pending input; both stdin and stdout must be a TTY.
bool supports_truecolor();
bool supports_vt100();

// On POSIX: no-op, returns whether stdout is a TTY.
// On Windows: enables VT processing and reports whether that succeeded.
bool enable_vt100() noexcept;

std::pair<std::size_t, std::size_t> term_size() noexcept; // {rows, cols}, {0,0} on failure
} // namespace vt100_utils
