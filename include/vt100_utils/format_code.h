#pragma once

#include <cstdint>

namespace vt100_utils {
	enum class FormatCode : uint8_t {
		// Basic Control Color
		Reset = 0,
		Bold = 1,
		Dim = 2,
		Underline = 4,
		Blink = 5,
		Reverse = 7,

		// Basic Front Ground Color
		FgBlack = 30,
		FgRed = 31,
		FgGreen = 32,
		FgYellow = 33,
		FgBlue = 34,
		FgMagenta = 35,
		FgCyan = 36,
		FgWhite = 37,

		// Basic Back Ground Color
		BgBlack = 40,
		BgRed = 41,
		BgGreen = 42,
		BgYellow = 43,
		BgBlue = 44,
		BgMagenta = 45,
		BgCyan = 46,
		BgWhite = 47,
		
		// Bright Front Ground Color
		FgBrightBlack = 90,
		FgBrightRed = 91,
		FgBrightGreen = 92,
		FgBrightYellow = 93,
		FgBrightBlue = 94,
		FgBrightMagenta = 95,
		FgBrightCyan = 96,
		FgBrightWhite = 97,

		// Bright Back Ground Color
		BgBrightBlack = 100,
		BgBrightRed = 101,
		BgBrightGreen = 102,
		BgBrightYellow = 103,
		BgBrightBlue = 104,
		BgBrightMagenta = 105,
		BgBrightCyan = 106,
		BgBrightWhite = 107
	};

} // namespace vt100_utils
