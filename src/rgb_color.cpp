#include "vt100_utils/rgb_color.h"

#include <algorithm>
#include <cmath>

namespace vt100_utils {

RgbColor gray(uint8_t g) noexcept {
	return RgbColor{g, g, g};
}

RgbColor hsl(float h, float s, float l) noexcept {
	if (s <= 0.0f) {
		uint8_t v = static_cast<uint8_t>(std::lround(l * 255.0f));
		return RgbColor{v, v, v};
	}

	h = std::fmod(h, 360.0f);
	if (h < 0.0f)
		h += 360.0f;
	h /= 360.0f;

	auto hue_to_rgb = [](float p, float q, float t) -> float {
		if (t < 0.0f) t += 1.0f;
		if (t > 1.0f) t -= 1.0f;
		if (t < 1.0f / 6.0f) return p + (q - p) * 6.0f * t;
		if (t < 1.0f / 2.0f) return q;
		if (t < 2.0f / 3.0f) return p + (q - p) * (2.0f / 3.0f - t) * 6.0f;
		return p;
	};

	float q = (l < 0.5f) ? l * (1.0f + s) : l + s - l * s;
	float p = 2.0f * l - q;

	RgbColor out;
	out.r = static_cast<uint8_t>(std::lround(hue_to_rgb(p, q, h + 1.0f / 3.0f) * 255.0f));
	out.g = static_cast<uint8_t>(std::lround(hue_to_rgb(p, q, h) * 255.0f));
	out.b = static_cast<uint8_t>(std::lround(hue_to_rgb(p, q, h - 1.0f / 3.0f) * 255.0f));
	return out;
}

RgbColor overlay(RgbColor raw, RgbColor mask, float alpha) noexcept {
	alpha = std::fmin(1.0f, std::fmax(0.0f, alpha));
	auto blend = [alpha](uint8_t a, uint8_t b) -> uint8_t {
		float v = a * (1.0f - alpha) + b * alpha;
		return static_cast<uint8_t>(std::lround(v));
	};
	return RgbColor{blend(raw.r, mask.r), blend(raw.g, mask.g), blend(raw.b, mask.b)};
}

} // namespace vt100_utils
