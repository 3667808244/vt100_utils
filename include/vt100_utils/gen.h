#pragma once

#include "vt100_utils/format_code.h"
#include "vt100_utils/rgb_color.h"
#include <cstdint>
#include <initializer_list>
#include <vector>
#include <string>

namespace vt100_utils {
	std::string gen_std_seq(FormatCode fc) noexcept;
	std::string gen_std_seq(const std::initializer_list<FormatCode> &fcs) noexcept;
	std::string gen_std_seq(const std::vector<FormatCode> &fcs) noexcept;

	std::string gen_rgb_seq(const RgbColor color, bool fg = true) noexcept;
#ifdef __cpp_lib_optional
#include <optional>
	inline std::string gen_rgb_seq(
		const std::optional<RgbColor> &fg = std::nullopt,
		const std::optional<RgbColor> &bg = std::nullopt
	) noexcept {
		std::string res;
		if (fg.has_value())
			res += gen_rgb_seq(fg.value(), true);
		if (bg.has_value())
			res += gen_rgb_seq(bg.value(), false);
		if (res.empty())
			res = "\x1b[0m";
		return res;
	}
#endif

	std::string gen_256_seq(uint8_t color, bool fg = true) noexcept;
}
