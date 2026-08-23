#pragma once

#include <cstddef>
#include <utility>

namespace vt100_utils {
	bool supports_truecolor() noexcept;
	bool supports_vt100() noexcept;
	bool enable_vt100() noexcept;
	std::pair<std::size_t, std::size_t> term_size() noexcept;
}
