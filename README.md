# vt100_utils

A small C++11 static library for generating VT100/ANSI terminal escape sequences
and probing terminal capabilities, with no dependencies beyond the C/C++ standard
library, POSIX (on Unix) and the Win32 console API (on Windows NT).

## Features

- Generate SGR (Select Graphic Rendition) escape sequences from `FormatCode`
  enums (single, initializer-list, or vector). An empty list produces a reset.
- Generate 24-bit truecolor (`\x1b[38;2;r;g;bm`) and 256-color
  (`\x1b[38;5;Nm`) sequences.
- Color helpers: grayscale, HSL→RGB conversion, and alpha blending.
- Actively probe terminal capabilities by sending escape queries and reading the
  responses — no environment variables involved:
  - `supports_vt100()` — on POSIX sends a **DA1** query (`ESC[c`) and parses the
    reply; on Windows enables console VT processing (`ENABLE_VIRTUAL_TERMINAL_PROCESSING`).
  - `supports_truecolor()` — on POSIX sends an **XTGETTCAP** query
    (`ESC P + q 6803 ESC \`, where `6803` is the hex encoding of `"RGB"`) and
    accepts both the standard hex-encoded reply (`524742`) and the literal
    `RGB`; on Windows a console that supports VT is assumed to support 24-bit
    color, so it agrees with `supports_vt100()`.
- Query terminal size via `ioctl(TIOCGWINSZ)` on POSIX and
  `GetConsoleScreenBufferInfo` on Windows.

## Build

The project is managed with [EazyMake](https://github.com/Anomalyco/ezmk).

```bash
ezmk build        # build the static library
ezmk test         # build and run unit tests
```

Output: `build/libvt100_utils.a`.

> POSIX probing only works on an interactive terminal (both stdin and stdout must
> be a TTY). In non-interactive environments `supports_vt100()` and
> `supports_truecolor()` return `false`, which is expected.
>
> The POSIX probes temporarily put stdin into raw mode and discard pending input,
> so do not call them while the application relies on already-buffered input.
>
> On Windows, ANSI sequences render only when VT processing is enabled, so the
> capability check enables it and leaves it on (requires a modern console, e.g.
> Windows Terminal or recent conhost). Existing console mode flags are preserved.
> `enable_vt100()` performs the same enabling and reports whether it succeeded.

## Usage

```cpp
#include <vt100_utils.h>

using namespace vt100_utils;

// Basic formatting
std::cout << gen_std_seq(FormatCode::Bold) << "bold" << gen_std_seq(FormatCode::Reset);

// Combined codes
std::cout << gen_std_seq({FormatCode::Bold, FormatCode::FgRed})
          << "bold red" << gen_std_seq(FormatCode::Reset);

// Truecolor
std::cout << gen_rgb_seq(RgbColor{255, 0, 0}) << "red" << gen_std_seq(FormatCode::Reset);

// Background color (no magic bools)
std::cout << gen_rgb_seq(RgbColor{0, 0, 255}, ColorLayer::Background) << "blue bg";

// 256-color
std::cout << gen_256_seq(196) << "red" << gen_std_seq(FormatCode::Reset);

// Color helpers
RgbColor g = gray(128);                              // {128,128,128}
RgbColor c = hsl(210.0f, 1.0f, 0.5f);                // a bright blue
RgbColor blended = overlay(c, gray(0), 0.5f);        // 50% blend with black

// Terminal capabilities (active probing, see note above)
if (supports_truecolor()) { /* ... */ }

// Terminal size {rows, cols}
auto size = term_size();
std::size_t rows = size.first;
std::size_t cols = size.second;
```

With C++17, the optional foreground/background pair helper is available:

```cpp
#include <optional>

// fg/bg both nullopt -> returns "\x1b[0m" (reset)
std::cout << gen_rgb_pair_seq(std::nullopt, std::nullopt);

// fg only
std::cout << gen_rgb_pair_seq(RgbColor{255, 0, 0});

// both fg and bg
std::cout << gen_rgb_pair_seq(RgbColor{255, 0, 0}, RgbColor{0, 0, 255});
```

## API Reference

### `version.h`

```cpp
#define VT100_UTILS_VERSION_MAJOR 0
#define VT100_UTILS_VERSION_MINOR 2
#define VT100_UTILS_VERSION_PATCH 1
#define VT100_UTILS_VERSION_STRING "0.2.1"
```

### `format_code.h`

```cpp
enum class FormatCode : uint8_t {
    Reset = 0, Bold = 1, Dim = 2, Italic = 3, Underline = 4,
    Blink = 5, Reverse = 7, Strikethrough = 9,
    BoldOff = 22, DimOff = 22, ItalicOff = 23, UnderlineOff = 24,
    BlinkOff = 25, ReverseOff = 27, StrikethroughOff = 29,
    FgDefault = 39, BgDefault = 49,
    FgBlack = 30, ... FgWhite = 37,
    BgBlack = 40, ... BgWhite = 47,
    FgBrightBlack = 90, ... FgBrightWhite = 97,
    BgBrightBlack = 100, ... BgBrightWhite = 107
};
```

### `rgb_color.h`

```cpp
struct RgbColor { uint8_t r, g, b; };
constexpr bool operator==(RgbColor, RgbColor) noexcept;
constexpr bool operator!=(RgbColor, RgbColor) noexcept;

constexpr RgbColor gray(uint8_t g) noexcept;              // {g,g,g}
RgbColor hsl(float h, float s, float l) noexcept;         // h in degrees,
                                                          // s/l clamped to [0,1]
RgbColor overlay(RgbColor raw, RgbColor mask, float alpha = 0.5f) noexcept;
                                                          // alpha clamped to [0,1]
```

### `gen.h`

```cpp
enum class ColorLayer : uint8_t { Foreground, Background };

std::string gen_std_seq(FormatCode fc);                              // "\x1b[{code}m"
std::string gen_std_seq(const std::initializer_list<FormatCode>& fcs);
std::string gen_std_seq(const std::vector<FormatCode>& fcs);         // "\x1b[1;31m";
                                                                     // empty -> "\x1b[0m"
std::string gen_rgb_seq(RgbColor color, bool fg = true);             // "\x1b[38;2;r;g;bm"
std::string gen_rgb_seq(RgbColor color, ColorLayer layer);
std::string gen_256_seq(uint8_t color, bool fg = true);              // "\x1b[38;5;Nm"
std::string gen_256_seq(uint8_t color, ColorLayer layer);

// C++17 only (the overload is compiled when the language mode is >= C++17):
std::string gen_rgb_pair_seq(const std::optional<RgbColor>& fg = std::nullopt,
                             const std::optional<RgbColor>& bg = std::nullopt);
```

### `term.h`

```cpp
bool supports_truecolor();       // active probe, may consume pending stdin
bool supports_vt100();           // active probe, may consume pending stdin
bool enable_vt100() noexcept;    // POSIX: no-op, returns whether stdout is a TTY;
                                 // Windows: enables VT processing and reports success
std::pair<std::size_t, std::size_t> term_size() noexcept;   // {rows, cols}, {0,0} on failure
```

## Testing

```bash
ezmk test
```

The built-in test framework compiles `test/test_vt100_utils.cpp` as a standalone
executable covering exact escape strings, color math (including out-of-range
input), DA1/XTGETTCAP response parsing (including the standard hex-encoded
reply), and `term_size` invariants.
