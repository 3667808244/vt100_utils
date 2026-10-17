#pragma once

#include "vt100_utils/format_code.h"
#include "vt100_utils/rgb_color.h"
#include <cstdint>
#include <initializer_list>
#include <string>
#include <vector>

#if defined(_MSVC_LANG)
#define VT100_UTILS_CPLUSPLUS _MSVC_LANG
#else
#define VT100_UTILS_CPLUSPLUS __cplusplus
#endif

#if VT100_UTILS_CPLUSPLUS >= 201703L
#include <optional>
#define VT100_UTILS_HAS_OPTIONAL 1
#endif

namespace vt100_utils {
enum class ColorLayer : uint8_t { Foreground = 0, Background = 1 };

std::string gen_std_seq(FormatCode fc);
std::string gen_std_seq(const std::initializer_list<FormatCode> &fcs);
std::string gen_std_seq(const std::vector<FormatCode> &fcs);

std::string gen_rgb_seq(RgbColor color, bool fg = true);
inline std::string gen_rgb_seq(RgbColor color, ColorLayer layer) {
	return gen_rgb_seq(color, layer == ColorLayer::Foreground);
}

#ifdef VT100_UTILS_HAS_OPTIONAL
// Convenience overload for foreground/background pairs. An empty pair
// produces a reset sequence. Named differently from gen_rgb_seq() so that
// calls such as gen_rgb_seq({}) are not ambiguous with RgbColor.
inline std::string gen_rgb_pair_seq(const std::optional<RgbColor> &fg = std::nullopt,
                                    const std::optional<RgbColor> &bg = std::nullopt) {
	if (!fg && !bg)
		return "\x1b[0m";

	std::string res;
	res.reserve(40);
	if (fg)
		res += gen_rgb_seq(*fg, true);
	if (bg)
		res += gen_rgb_seq(*bg, false);
	return res;
}
#endif

std::string gen_256_seq(uint8_t color, bool fg = true);
inline std::string gen_256_seq(uint8_t color, ColorLayer layer) {
	return gen_256_seq(color, layer == ColorLayer::Foreground);
}
} // namespace vt100_utils

#undef VT100_UTILS_HAS_OPTIONAL
#undef VT100_UTILS_CPLUSPLUS
