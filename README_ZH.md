# vt100_utils

一个轻量的 C++11 静态库，用于生成 VT100/ANSI 终端转义序列并探测终端能力。
除 C/C++ 标准库外，仅依赖 POSIX 接口（Unix）与 Win32 控制台 API（Windows NT）。

## 特性

- 通过 `FormatCode` 枚举生成 SGR（Select Graphic Rendition）转义序列
  （支持单个、initializer-list、vector 三种形式；空列表输出重置序列）。
- 生成 24 位真彩色（`\x1b[38;2;r;g;bm`）与 256 色（`\x1b[38;5;Nm`）序列。
- 颜色工具：灰度、HSL→RGB 转换、alpha 混合。
- **主动探测**终端能力：向终端发送转义查询序列并读取应答，完全不依赖环境变量：
  - `supports_vt100()` — POSIX 下发送 **DA1** 查询（`ESC[c`）并解析应答；
    Windows 下启用控制台 VT 处理（`ENABLE_VIRTUAL_TERMINAL_PROCESSING`）。
  - `supports_truecolor()` — POSIX 下发送 **XTGETTCAP** 查询
    （`ESC P + q 6803 ESC \`，`6803` 为 `"RGB"` 的十六进制编码），同时接受
    标准的十六进制应答（`524742`）与部分终端的字面量 `RGB` 应答；Windows 下
    假定支持 VT 的控制台也支持 24 位真彩色，故与 `supports_vt100()` 结论一致。
- 终端尺寸：POSIX 通过 `ioctl(TIOCGWINSZ)`，Windows 通过
  `GetConsoleScreenBufferInfo` 获取。

## 构建

项目由 [EazyMake](https://github.com/Anomalyco/ezmk) 管理。

```bash
ezmk build        # 构建静态库
ezmk test         # 构建并运行单元测试
```

产物：`build/libvt100_utils.a`。

> POSIX 下的主动探测仅在交互式终端（stdin 与 stdout 均为 TTY）下有效；非交互环境下
> `supports_vt100()` 与 `supports_truecolor()` 返回 `false`，属预期行为。
>
> POSIX 探测会临时把 stdin 切换为 raw 模式并丢弃尚未读取的输入，因此不要在应用
> 依赖已缓冲输入时调用。
>
> Windows 下 ANSI 序列需在启用 VT 处理后才会渲染，因此能力探测会顺带启用并保持开启
> （要求现代控制台，如 Windows Terminal 或新版 conhost）。启用时会保留控制台原有的
> 其他模式标志。`enable_vt100()` 做同样的启用并返回是否成功。

## 用法

```cpp
#include <vt100_utils.h>

using namespace vt100_utils;

// 基础格式化
std::cout << gen_std_seq(FormatCode::Bold) << "加粗" << gen_std_seq(FormatCode::Reset);

// 组合代码
std::cout << gen_std_seq({FormatCode::Bold, FormatCode::FgRed})
          << "粗体红色" << gen_std_seq(FormatCode::Reset);

// 真彩色
std::cout << gen_rgb_seq(RgbColor{255, 0, 0}) << "红色" << gen_std_seq(FormatCode::Reset);

// 背景色（无需 magic bool）
std::cout << gen_rgb_seq(RgbColor{0, 0, 255}, ColorLayer::Background) << "蓝色背景";

// 256 色
std::cout << gen_256_seq(196) << "红色" << gen_std_seq(FormatCode::Reset);

// 颜色工具
RgbColor g = gray(128);                               // {128,128,128}
RgbColor c = hsl(210.0f, 1.0f, 0.5f);                 // 亮蓝色
RgbColor blended = overlay(c, gray(0), 0.5f);         // 与黑色 50% 混合

// 终端能力（主动探测，见上方说明）
if (supports_truecolor()) { /* ... */ }

// 终端尺寸 {rows, cols}
auto size = term_size();
std::size_t rows = size.first;
std::size_t cols = size.second;
```

C++17 下可用的前景/背景可选重载：

```cpp
#include <optional>

// fg/bg 均为 nullopt -> 返回 "\x1b[0m"（重置）
std::cout << gen_rgb_pair_seq(std::nullopt, std::nullopt);

// 仅前景色
std::cout << gen_rgb_pair_seq(RgbColor{255, 0, 0});

// 前景 + 背景
std::cout << gen_rgb_pair_seq(RgbColor{255, 0, 0}, RgbColor{0, 0, 255});
```

## API 参考

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
RgbColor hsl(float h, float s, float l) noexcept;         // h 为角度，
                                                          // s/l 会被 clamp 到 [0,1]
RgbColor overlay(RgbColor raw, RgbColor mask, float alpha = 0.5f) noexcept;
                                                          // alpha 会被 clamp 到 [0,1]
```

### `gen.h`

```cpp
enum class ColorLayer : uint8_t { Foreground, Background };

std::string gen_std_seq(FormatCode fc);                              // "\x1b[{code}m"
std::string gen_std_seq(const std::initializer_list<FormatCode>& fcs);
std::string gen_std_seq(const std::vector<FormatCode>& fcs);         // "\x1b[1;31m";
                                                                     // 空列表 -> "\x1b[0m"
std::string gen_rgb_seq(RgbColor color, bool fg = true);             // "\x1b[38;2;r;g;bm"
std::string gen_rgb_seq(RgbColor color, ColorLayer layer);
std::string gen_256_seq(uint8_t color, bool fg = true);              // "\x1b[38;5;Nm"
std::string gen_256_seq(uint8_t color, ColorLayer layer);

// 仅 C++17（语言模式 >= C++17 时提供该重载）：
std::string gen_rgb_pair_seq(const std::optional<RgbColor>& fg = std::nullopt,
                             const std::optional<RgbColor>& bg = std::nullopt);
```

### `term.h`

```cpp
bool supports_truecolor();       // 主动探测，可能消费待处理的 stdin 输入
bool supports_vt100();           // 主动探测，可能消费待处理的 stdin 输入
bool enable_vt100() noexcept;    // POSIX：无操作，返回 stdout 是否为 TTY；
                                 // Windows：启用 VT 处理并返回是否成功
std::pair<std::size_t, std::size_t> term_size() noexcept;   // {rows, cols}，失败返回 {0,0}
```

## 测试

```bash
ezmk test
```

内置测试框架将 `test/test_vt100_utils.cpp` 编译为独立可执行文件，
覆盖转义序列精确输出、颜色数学（含越界输入）、DA1/XTGETTCAP 应答解析
（含标准十六进制应答）以及 `term_size` 不变式。
