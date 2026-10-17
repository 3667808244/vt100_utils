#pragma once

#include <cstdint>

namespace vt100_utils {
struct RgbColor {
	uint8_t r, g, b;
};

constexpr bool operator==(RgbColor lhs, RgbColor rhs) noexcept {
	return lhs.r == rhs.r && lhs.g == rhs.g && lhs.b == rhs.b;
}

constexpr bool operator!=(RgbColor lhs, RgbColor rhs) noexcept {
	return !(lhs == rhs);
}

constexpr RgbColor gray(uint8_t g) noexcept {
	return RgbColor{g, g, g};
}

// h in degrees (any finite value is normalized); s and l are clamped to [0,1].
RgbColor hsl(float h, float s, float l) noexcept;

// alpha is clamped to [0,1]; 0 returns raw, 1 returns mask.
RgbColor overlay(RgbColor raw, RgbColor mask, float alpha = 0.5f) noexcept;
} // namespace vt100_utils
