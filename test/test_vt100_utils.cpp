#include <cstdio>
#include <string>
#include <utility>
#include <vector>

#include "vt100_utils.h"
#include "../src/probe.h"

namespace {

int g_total = 0;
int g_fail = 0;

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
			std::fprintf(stderr, "FAIL %s:%d: %s == %s\n", __FILE__, __LINE__, #a, #b); \
		} \
	} while (0)

using vt100_utils::FormatCode;
using vt100_utils::RgbColor;

bool rgb_eq(RgbColor a, RgbColor b) {
	return a.r == b.r && a.g == b.g && a.b == b.b;
}

} // namespace

int main() {
	// ---- gen_std_seq (single) ----
	CHECK_EQ(vt100_utils::gen_std_seq(FormatCode::Reset), std::string("\x1b[0m"));
	CHECK_EQ(vt100_utils::gen_std_seq(FormatCode::Bold), std::string("\x1b[1m"));
	CHECK_EQ(vt100_utils::gen_std_seq(FormatCode::FgRed), std::string("\x1b[31m"));
	CHECK_EQ(vt100_utils::gen_std_seq(FormatCode::BgBlack), std::string("\x1b[40m"));
	CHECK_EQ(vt100_utils::gen_std_seq(FormatCode::FgBrightGreen), std::string("\x1b[92m"));
	CHECK_EQ(vt100_utils::gen_std_seq(FormatCode::BgBrightWhite), std::string("\x1b[107m"));

	// ---- gen_std_seq (list / vector) ----
	CHECK_EQ(vt100_utils::gen_std_seq({FormatCode::Bold, FormatCode::FgRed}),
	         std::string("\x1b[1;31m"));
	CHECK_EQ(vt100_utils::gen_std_seq({FormatCode::Reset}), std::string("\x1b[0m"));
	std::vector<FormatCode> v{FormatCode::Bold, FormatCode::Underline, FormatCode::FgBlue};
	CHECK_EQ(vt100_utils::gen_std_seq(v), std::string("\x1b[1;4;34m"));

	// ---- gen_rgb_seq ----
	CHECK_EQ(vt100_utils::gen_rgb_seq(RgbColor{255, 0, 0}),
	         std::string("\x1b[38;2;255;0;0m"));
	CHECK_EQ(vt100_utils::gen_rgb_seq(RgbColor{1, 2, 3}, false),
	         std::string("\x1b[48;2;1;2;3m"));

	// ---- gen_256_seq ----
	CHECK_EQ(vt100_utils::gen_256_seq(196), std::string("\x1b[38;5;196m"));
	CHECK_EQ(vt100_utils::gen_256_seq(52, false), std::string("\x1b[48;5;52m"));
	CHECK_EQ(vt100_utils::gen_256_seq(0), std::string("\x1b[38;5;0m"));

	// ---- rgb_color: gray ----
	CHECK(rgb_eq(vt100_utils::gray(128), RgbColor{128, 128, 128}));
	CHECK(rgb_eq(vt100_utils::gray(0), RgbColor{0, 0, 0}));
	CHECK(rgb_eq(vt100_utils::gray(255), RgbColor{255, 255, 255}));

	// ---- rgb_color: hsl ----
	CHECK(rgb_eq(vt100_utils::hsl(0.0f, 1.0f, 0.5f), RgbColor{255, 0, 0}));
	CHECK(rgb_eq(vt100_utils::hsl(120.0f, 1.0f, 0.5f), RgbColor{0, 255, 0}));
	CHECK(rgb_eq(vt100_utils::hsl(240.0f, 1.0f, 0.5f), RgbColor{0, 0, 255}));
	CHECK(rgb_eq(vt100_utils::hsl(0.0f, 0.0f, 0.5f), RgbColor{128, 128, 128}));
	CHECK(rgb_eq(vt100_utils::hsl(0.0f, 1.0f, 1.0f), RgbColor{255, 255, 255}));

	// ---- rgb_color: overlay ----
	CHECK(rgb_eq(vt100_utils::overlay(RgbColor{100, 100, 100}, RgbColor{200, 200, 200}, 0.5f),
	             RgbColor{150, 150, 150}));
	CHECK(rgb_eq(vt100_utils::overlay(RgbColor{0, 0, 0}, RgbColor{255, 255, 255}, 1.0f),
	             RgbColor{255, 255, 255}));
	CHECK(rgb_eq(vt100_utils::overlay(RgbColor{0, 0, 0}, RgbColor{255, 255, 255}, 0.0f),
	             RgbColor{0, 0, 0}));
	CHECK(rgb_eq(vt100_utils::overlay(RgbColor{0, 0, 0}, RgbColor{255, 255, 255}, 5.0f),
	             RgbColor{255, 255, 255}));
	CHECK(rgb_eq(vt100_utils::overlay(RgbColor{0, 0, 0}, RgbColor{255, 255, 255}, -1.0f),
	             RgbColor{0, 0, 0}));

	// ---- probe: DA1 应答解析 ----
	CHECK(vt100_utils::detail::parse_da1_response("\x1b[?1;2c"));
	CHECK(vt100_utils::detail::parse_da1_response("\x1b[?62;1;2c"));
	CHECK(!vt100_utils::detail::parse_da1_response("\x1b[?c"));
	CHECK(!vt100_utils::detail::parse_da1_response("\x1b[?"));
	CHECK(!vt100_utils::detail::parse_da1_response("hello"));
	CHECK(!vt100_utils::detail::parse_da1_response(""));

	// ---- probe: XTGETTCAP RGB 应答解析 ----
	CHECK(vt100_utils::detail::parse_xtgettcap_rgb("\x1bP1+r6803=1;RGB\x1b\\"));
	CHECK(vt100_utils::detail::parse_xtgettcap_rgb("\x1bP1+r6803=RGB\x1b\\"));
	CHECK(!vt100_utils::detail::parse_xtgettcap_rgb("\x1bP1+r6801=\x1b\\"));
	CHECK(!vt100_utils::detail::parse_xtgettcap_rgb(""));
	CHECK(!vt100_utils::detail::parse_xtgettcap_rgb("\x1bP1+r6803=\x1b\\"));

	// ---- term_size: 返回一致性的 (rows, cols)，要么都是 0 要么都 > 0 ----
	std::pair<std::size_t, std::size_t> ts = vt100_utils::term_size();
	bool ok = (ts.first == 0 && ts.second == 0) || (ts.first > 0 && ts.second > 0);
	CHECK(ok);

	// ---- 主动探测函数：不抛异常即可（非 tty 环境返回 false 属预期） ----
	vt100_utils::supports_vt100();
	vt100_utils::supports_truecolor();
	vt100_utils::enable_vt100();

	std::printf("%d/%d passed\n", g_total - g_fail, g_total);
	return g_fail == 0 ? 0 : 1;
}
