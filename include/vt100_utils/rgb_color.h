#pragma once

#include <cmath>
#include <cstdint>

namespace vt100_utils {
	struct RgbColor{
		uint8_t r,g,b;
	};

	RgbColor gray(uint8_t g) noexcept;
	RgbColor hsl(float h, float s, float l) noexcept;
	RgbColor overlay(RgbColor raw, RgbColor mask, float alpha = 0.5f) noexcept;
}
