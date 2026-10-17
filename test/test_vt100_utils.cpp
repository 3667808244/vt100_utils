#include <cstdio>
#include <limits>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include "vt100_utils.h"
#include "vt100_utils/detail/probe.h"

#if defined(_MSVC_LANG)
#if _MSVC_LANG >= 201703L
#define TEST_HAS_OPTIONAL 1
#endif
#else
#if __cplusplus >= 201703L
#define TEST_HAS_OPTIONAL 1
#endif
#endif

#ifdef TEST_HAS_OPTIONAL
#include <optional>
#endif

namespace {

int g_total = 0;
int g_fail = 0;

// Escape control bytes so failures are readable on any terminal.
std::string to_debug(const std::string &s) {
	std::string out;
	out.reserve(s.size() + 2);
	out += '"';
	for (std::size_t i = 0; i < s.size(); ++i) {
		const unsigned char c = static_cast<unsigned char>(s[i]);
		switch (c) {
		case '\\':
			out += "\\\\";
			break;
		case '"':
			out += "\\\"";
			break;
		default:
			if (c >= 0x20 && c < 0x7f) {
				out += static_cast<char>(c);
			} else {
				char hex[8];
				std::snprintf(hex, sizeof(hex), "\\x%02x", static_cast<unsigned>(c));
				out += hex;
			}
		}
	}
	out += '"';
	return out;
}

std::string to_debug(vt100_utils::RgbColor c) {
	char buf[64];
	std::snprintf(buf, sizeof(buf), "{r=%u,g=%u,b=%u}", static_cast<unsigned>(c.r),
	              static_cast<unsigned>(c.g), static_cast<unsigned>(c.b));
	return buf;
}

template <typename T>
typename std::enable_if<std::is_arithmetic<T>::value, std::string>::type to_debug(T v) {
	return std::to_string(v);
}

#define CHECK(expr) \
	do { \
		++g_total; \
		if (!(expr)) { \
			++g_fail; \
			std::fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #expr); \
		} \
	} while (0)

#define CHECK_EQ(a, b) \
	do { \
		++g_total; \
		auto va = (a); \
		auto vb = (b); \
		if (!(va == vb)) { \
			++g_fail; \
			std::fprintf(stderr, "FAIL %s:%d: %s == %s\n  left:  %s\n  right: %s\n", __FILE__, \
			             __LINE__, #a, #b, to_debug(va).c_str(), to_debug(vb).c_str()); \
		} \
	} while (0)

using vt100_utils::ColorLayer;
using vt100_utils::FormatCode;
using vt100_utils::RgbColor;

} // namespace

int main() {
	// ---- rgb_color: comparison / constexpr ----
	CHECK((RgbColor{1, 2, 3} == RgbColor{1, 2, 3}));
	CHECK((RgbColor{1, 2, 3} != RgbColor{1, 2, 4}));
	static_assert(vt100_utils::gray(7).r == 7, "gray() should be constexpr");

	// ---- gen_std_seq (single) ----
	CHECK_EQ(vt100_utils::gen_std_seq(FormatCode::Reset), std::string("\x1b[0m"));
	CHECK_EQ(vt100_utils::gen_std_seq(FormatCode::Bold), std::string("\x1b[1m"));
	CHECK_EQ(vt100_utils::gen_std_seq(FormatCode::FgRed), std::string("\x1b[31m"));
	CHECK_EQ(vt100_utils::gen_std_seq(FormatCode::BgBlack), std::string("\x1b[40m"));
	CHECK_EQ(vt100_utils::gen_std_seq(FormatCode::FgBrightGreen), std::string("\x1b[92m"));
	CHECK_EQ(vt100_utils::gen_std_seq(FormatCode::BgBrightWhite), std::string("\x1b[107m"));
	CHECK_EQ(vt100_utils::gen_std_seq(FormatCode::BoldOff), std::string("\x1b[22m"));
	CHECK_EQ(vt100_utils::gen_std_seq(FormatCode::UnderlineOff), std::string("\x1b[24m"));
	CHECK_EQ(vt100_utils::gen_std_seq(FormatCode::StrikethroughOff), std::string("\x1b[29m"));
	CHECK_EQ(vt100_utils::gen_std_seq(FormatCode::FgDefault), std::string("\x1b[39m"));
	CHECK_EQ(vt100_utils::gen_std_seq(FormatCode::BgDefault), std::string("\x1b[49m"));

	// ---- gen_std_seq (list / vector) ----
	CHECK_EQ(vt100_utils::gen_std_seq({FormatCode::Bold, FormatCode::FgRed}),
	         std::string("\x1b[1;31m"));
	CHECK_EQ(vt100_utils::gen_std_seq({FormatCode::Reset}), std::string("\x1b[0m"));
	CHECK_EQ(vt100_utils::gen_std_seq({}),
	         std::string("\x1b[0m")); // empty list => reset
	std::vector<FormatCode> v{FormatCode::Bold, FormatCode::Underline, FormatCode::FgBlue};
	CHECK_EQ(vt100_utils::gen_std_seq(v), std::string("\x1b[1;4;34m"));
	std::vector<FormatCode> empty;
	CHECK_EQ(vt100_utils::gen_std_seq(empty), std::string("\x1b[0m"));

	// ---- gen_rgb_seq ----
	CHECK_EQ(vt100_utils::gen_rgb_seq(RgbColor{255, 0, 0}), std::string("\x1b[38;2;255;0;0m"));
	CHECK_EQ(vt100_utils::gen_rgb_seq(RgbColor{1, 2, 3}, false), std::string("\x1b[48;2;1;2;3m"));
	CHECK_EQ(vt100_utils::gen_rgb_seq(RgbColor{1, 2, 3}, ColorLayer::Background),
	         std::string("\x1b[48;2;1;2;3m"));
	CHECK_EQ(vt100_utils::gen_rgb_seq({255, 0, 0}), std::string("\x1b[38;2;255;0;0m"));
	// must resolve to the RgbColor overload, not be ambiguous with the pair
	// helper
	CHECK_EQ(vt100_utils::gen_rgb_seq({}), std::string("\x1b[38;2;0;0;0m"));

	// ---- gen_256_seq ----
	CHECK_EQ(vt100_utils::gen_256_seq(196), std::string("\x1b[38;5;196m"));
	CHECK_EQ(vt100_utils::gen_256_seq(52, false), std::string("\x1b[48;5;52m"));
	CHECK_EQ(vt100_utils::gen_256_seq(52, ColorLayer::Background), std::string("\x1b[48;5;52m"));
	CHECK_EQ(vt100_utils::gen_256_seq(0), std::string("\x1b[38;5;0m"));

#ifdef TEST_HAS_OPTIONAL
	// ---- gen_rgb_pair_seq (C++17) ----
	CHECK_EQ(vt100_utils::gen_rgb_pair_seq(), std::string("\x1b[0m"));
	CHECK_EQ(vt100_utils::gen_rgb_pair_seq(RgbColor{255, 0, 0}), std::string("\x1b[38;2;255;0;0m"));
	CHECK_EQ(vt100_utils::gen_rgb_pair_seq(RgbColor{255, 0, 0}, RgbColor{0, 0, 255}),
	         std::string("\x1b[38;2;255;0;0m\x1b[48;2;0;0;255m"));
	CHECK_EQ(vt100_utils::gen_rgb_pair_seq(std::nullopt, RgbColor{0, 0, 255}),
	         std::string("\x1b[48;2;0;0;255m"));
	CHECK_EQ(vt100_utils::gen_rgb_pair_seq(std::nullopt, std::nullopt), std::string("\x1b[0m"));
#endif

	// ---- rgb_color: gray ----
	CHECK_EQ(vt100_utils::gray(128), (RgbColor{128, 128, 128}));
	CHECK_EQ(vt100_utils::gray(0), (RgbColor{0, 0, 0}));
	CHECK_EQ(vt100_utils::gray(255), (RgbColor{255, 255, 255}));

	// ---- rgb_color: hsl ----
	CHECK_EQ(vt100_utils::hsl(0.0f, 1.0f, 0.5f), (RgbColor{255, 0, 0}));
	CHECK_EQ(vt100_utils::hsl(120.0f, 1.0f, 0.5f), (RgbColor{0, 255, 0}));
	CHECK_EQ(vt100_utils::hsl(240.0f, 1.0f, 0.5f), (RgbColor{0, 0, 255}));
	CHECK_EQ(vt100_utils::hsl(0.0f, 0.0f, 0.5f), (RgbColor{128, 128, 128}));
	CHECK_EQ(vt100_utils::hsl(0.0f, 1.0f, 1.0f), (RgbColor{255, 255, 255}));
	CHECK_EQ(vt100_utils::hsl(-120.0f, 1.0f, 0.5f), (RgbColor{0, 0, 255}));
	CHECK_EQ(vt100_utils::hsl(480.0f, 1.0f, 0.5f), (RgbColor{0, 255, 0}));

	// out-of-range input is clamped instead of wrapping
	CHECK_EQ(vt100_utils::hsl(0.0f, 1.0f, 2.0f), (RgbColor{255, 255, 255}));
	CHECK_EQ(vt100_utils::hsl(0.0f, 1.0f, -1.0f), (RgbColor{0, 0, 0}));
	CHECK_EQ(vt100_utils::hsl(0.0f, 2.0f, 0.5f), (RgbColor{255, 0, 0}));
	CHECK_EQ(vt100_utils::hsl(0.0f, -2.0f, 0.5f), (RgbColor{128, 128, 128}));
	CHECK_EQ(vt100_utils::hsl(std::numeric_limits<float>::quiet_NaN(), 1.0f, 0.5f),
	         (RgbColor{255, 0, 0}));

	// ---- rgb_color: overlay ----
	CHECK_EQ(vt100_utils::overlay(RgbColor{100, 100, 100}, RgbColor{200, 200, 200}, 0.5f),
	         (RgbColor{150, 150, 150}));
	CHECK_EQ(vt100_utils::overlay(RgbColor{0, 0, 0}, RgbColor{255, 255, 255}, 1.0f),
	         (RgbColor{255, 255, 255}));
	CHECK_EQ(vt100_utils::overlay(RgbColor{0, 0, 0}, RgbColor{255, 255, 255}, 0.0f),
	         (RgbColor{0, 0, 0}));
	CHECK_EQ(vt100_utils::overlay(RgbColor{0, 0, 0}, RgbColor{255, 255, 255}, 5.0f),
	         (RgbColor{255, 255, 255}));
	CHECK_EQ(vt100_utils::overlay(RgbColor{1, 2, 3}, RgbColor{255, 255, 255}, -1.0f),
	         (RgbColor{1, 2, 3}));
	CHECK_EQ(vt100_utils::overlay(RgbColor{1, 2, 3}, RgbColor{255, 255, 255},
	                              std::numeric_limits<float>::quiet_NaN()),
	         (RgbColor{1, 2, 3}));

	// ---- probe: DA1 应答解析 ----
	CHECK(vt100_utils::detail::parse_da1_response("\x1b[?1;2c"));
	CHECK(vt100_utils::detail::parse_da1_response("\x1b[?62;1;2c"));
	CHECK(vt100_utils::detail::parse_da1_response("\x1b[?1;2c junk"));
	CHECK(!vt100_utils::detail::parse_da1_response("\x1b[?c"));
	CHECK(!vt100_utils::detail::parse_da1_response("\x1b[?"));
	CHECK(!vt100_utils::detail::parse_da1_response("\x1b[?1;2"));
	CHECK(!vt100_utils::detail::parse_da1_response("hello"));
	CHECK(!vt100_utils::detail::parse_da1_response(""));

	// ---- probe: XTGETTCAP RGB 应答解析 ----
	// Standard reply: value is the hex encoding of "RGB".
	CHECK(vt100_utils::detail::parse_xtgettcap_rgb("\x1bP1+r6803=524742\x1b\\"));
	// Some terminals reply with the literal name.
	CHECK(vt100_utils::detail::parse_xtgettcap_rgb("\x1bP1+r6803=RGB\x1b\\"));
	CHECK(!vt100_utils::detail::parse_xtgettcap_rgb("\x1bP1+r6803=1\x1b\\"));
	CHECK(!vt100_utils::detail::parse_xtgettcap_rgb("\x1bP0+r6803\x1b\\"));
	CHECK(!vt100_utils::detail::parse_xtgettcap_rgb("\x1bP1+r6801=\x1b\\"));
	CHECK(!vt100_utils::detail::parse_xtgettcap_rgb(""));
	// matches must stay inside the value of the 6803 reply
	CHECK(
	    !vt100_utils::detail::parse_xtgettcap_rgb("\x1bP1+r6803=1\x1b\\\x1bP1+r9999=524742\x1b\\"));
	CHECK(!vt100_utils::detail::parse_xtgettcap_rgb("\x1bP1+r6803=\x1b\\524742"));

	// ---- term_size: 返回一致性的 (rows, cols)，要么都是 0 要么都 > 0 ----
	std::pair<std::size_t, std::size_t> ts = vt100_utils::term_size();
	const bool ok = (ts.first == 0 && ts.second == 0) || (ts.first > 0 && ts.second > 0);
	CHECK(ok);

	// ---- 主动探测函数：不抛异常即可（非 tty 环境返回 false 属预期） ----
	vt100_utils::supports_vt100();
	vt100_utils::supports_truecolor();
	vt100_utils::enable_vt100();

	std::printf("%d/%d passed\n", g_total - g_fail, g_total);
	return g_fail == 0 ? 0 : 1;
}
