#pragma once

#include <string>

namespace vt100_utils {
namespace detail {

	bool parse_da1_response(const std::string &resp) noexcept;
	bool parse_xtgettcap_rgb(const std::string &resp) noexcept;

	// 向终端发送查询序列，带超时读取应答；失败或超时返回空串
	std::string query_terminal(const char *cmd, int timeout_ms) noexcept;

} // namespace detail
} // namespace vt100_utils
