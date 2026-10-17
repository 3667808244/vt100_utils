#pragma once

#include <string>

namespace vt100_utils {
namespace detail {

bool parse_da1_response(const std::string &resp) noexcept;
bool parse_xtgettcap_rgb(const std::string &resp) noexcept;

// Sends a query sequence and reads the reply. Returns an empty string on
// any failure or timeout.
//
// POSIX only: requires stdin and stdout to be TTYs, switches stdin to
// non-canonical/no-echo mode for the duration of the call, and discards
// pending input (TCSAFLUSH) so stale bytes are not mistaken for a reply.
std::string query_terminal(const char *cmd, int timeout_ms);

} // namespace detail
} // namespace vt100_utils
