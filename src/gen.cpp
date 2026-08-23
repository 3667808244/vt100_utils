#include "vt100_utils/gen.h"

#include <sstream>

namespace vt100_utils {

namespace {

template <typename Container>
std::string join_codes(const Container &fcs) {
	std::ostringstream oss;
	bool first = true;
	for (FormatCode fc : fcs) {
		if (!first)
			oss << ';';
		oss << static_cast<int>(fc);
		first = false;
	}
	return std::string("\x1b[") + oss.str() + 'm';
}

} // namespace

std::string gen_std_seq(FormatCode fc) noexcept {
	return std::string("\x1b[") + std::to_string(static_cast<int>(fc)) + 'm';
}

std::string gen_std_seq(const std::initializer_list<FormatCode> &fcs) noexcept {
	return join_codes(fcs);
}

std::string gen_std_seq(const std::vector<FormatCode> &fcs) noexcept {
	return join_codes(fcs);
}

std::string gen_rgb_seq(const RgbColor color, bool fg) noexcept {
	std::string res = "\x1b[";
	res += fg ? "38" : "48";
	res += ";2;";
	res += std::to_string(static_cast<int>(color.r));
	res += ';';
	res += std::to_string(static_cast<int>(color.g));
	res += ';';
	res += std::to_string(static_cast<int>(color.b));
	res += 'm';
	return res;
}

std::string gen_256_seq(uint8_t color, bool fg) noexcept {
	std::string res = "\x1b[";
	res += fg ? "38" : "48";
	res += ";5;";
	res += std::to_string(static_cast<int>(color));
	res += 'm';
	return res;
}

} // namespace vt100_utils
