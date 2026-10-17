#include "vt100_utils/gen.h"

#include <string>

namespace vt100_utils {

namespace {

template <typename Container>
std::string join_codes(const Container &fcs) {
	if (fcs.begin() == fcs.end())
		return "\x1b[0m";

	std::string res;
	res.reserve(3 + fcs.size() * 4);
	res += "\x1b[";
	bool first = true;
	for (FormatCode fc : fcs) {
		if (!first)
			res += ';';
		res += std::to_string(static_cast<int>(fc));
		first = false;
	}
	res += 'm';
	return res;
}

} // namespace

std::string gen_std_seq(FormatCode fc) {
	return std::string("\x1b[") + std::to_string(static_cast<int>(fc)) + 'm';
}

std::string gen_std_seq(const std::initializer_list<FormatCode> &fcs) {
	return join_codes(fcs);
}

std::string gen_std_seq(const std::vector<FormatCode> &fcs) {
	return join_codes(fcs);
}

std::string gen_rgb_seq(RgbColor color, bool fg) {
	std::string res;
	res.reserve(20);
	res += "\x1b[";
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

std::string gen_256_seq(uint8_t color, bool fg) {
	std::string res;
	res.reserve(12);
	res += "\x1b[";
	res += fg ? "38" : "48";
	res += ";5;";
	res += std::to_string(static_cast<int>(color));
	res += 'm';
	return res;
}

} // namespace vt100_utils
