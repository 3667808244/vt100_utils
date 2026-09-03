# vt100_utils

一个轻量的 C++11 静态库，用于生成 VT100/ANSI 终端转义序列并探测终端能力。
除 C/C++ 标准库外，仅依赖 POSIX 接口（Unix）与 Win32 控制台 API（Windows NT）。

## 特性

- 通过 `FormatCode` 枚举生成 SGR（Select Graphic Rendition）转义序列
  （支持单个、initializer-list、vector 三种形式）。
- 生成 24 位真彩色（`\x1b[38;2;r;g;bm`）与 256 色（`\x1b[38;5;Nm`）序列。
- 颜色工具：灰度、HSL→RGB 转换、alpha 混合。
- **主动探测**终端能力：向终端发送转义查询序列并读取应答，完全不依赖环境变量：
  - `supports_vt100()` — POSIX 下发送 **DA1** 查询（`ESC[c`）并解析应答；
    Windows 下启用控制台 VT 处理（`ENABLE_VIRTUAL_TERMINAL_PROCESSING`）。
  - `supports_truecolor()` — POSIX 下发送 **XTGETTCAP** 查询（`ESC P + q 6803 ESC \`，
    `6803` 为 `"RGB"` 的十进制编码），应答含 `RGB` 即判定支持；Windows 下
    支持 VT 的控制台即支持 24 位真彩色，故与 `supports_vt100()` 结论一致。
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
> Windows 下 ANSI 序列需在启用 VT 处理后才会渲染，因此能力探测会顺带启用并保持开启
> （要求现代控制台，如 Windows Terminal 或新版 conhost）。`enable_vt100()` 做同样的启用。

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

// 256 色
std::cout << gen_256_seq(196) << "红色" << gen_std_seq(FormatCode::Reset);

// 颜色工具
RgbColor g = gray(128);                               // {128,128,128}
RgbColor c = hsl(210.0f, 1.0f, 0.5f);                 // 亮蓝色
RgbColor blended = overlay(c, gray(0), 0.5f);         // 与黑色 50% 混合

// 终端能力（主动探测，见上方说明）
if (supports_truecolor()) { /* ... */ }

// 终端尺寸 {rows, cols}
auto [rows, cols] = term_size();
```

C++17 下可用的可选重载：

```cpp
#include <optional>

// fg/bg 均为 nullopt -> 返回 "\x1b[0m"（重置）
std::cout << gen_rgb_seq(std::nullopt, std::nullopt);

// 仅前景色
std::cout << gen_rgb_seq(std::optional<RgbColor>{RgbColor{255, 0, 0}});

// 前景 + 背景
std::cout << gen_rgb_seq(
    std::optional<RgbColor>{RgbColor{255, 0, 0}},
    std::optional<RgbColor>{RgbColor{0, 0, 255}});
```

## API 参考

### `format_code.h`

```cpp
enum class FormatCode : uint8_t {
    Reset = 0, Bold = 1, Dim = 2, Underline = 4, Blink = 5, Reverse = 7,
    FgBlack = 30, ... FgWhite = 37,
    BgBlack = 40, ... BgWhite = 47,
    FgBrightBlack = 90, ... FgBrightWhite = 97,
    BgBrightBlack = 100, ... BgBrightWhite = 107
};
```

### `rgb_color.h`

```cpp
struct RgbColor { uint8_t r, g, b; };

RgbColor gray(uint8_t g) noexcept;                        // {g,g,g}
RgbColor hsl(float h, float s, float l) noexcept;         // h 为角度，s/l 取值范围 [0,1]
RgbColor overlay(RgbColor raw, RgbColor mask, float alpha = 0.5f) noexcept;
```

### `gen.h`

```cpp
std::string gen_std_seq(FormatCode fc) noexcept;                              // "\x1b[{code}m"
std::string gen_std_seq(const std::initializer_list<FormatCode>& fcs) noexcept;
std::string gen_std_seq(const std::vector<FormatCode>& fcs) noexcept;         // "\x1b[1;31m"
std::string gen_rgb_seq(RgbColor color, bool fg = true) noexcept;             // "\x1b[38;2;r;g;bm"
std::string gen_256_seq(uint8_t color, bool fg = true) noexcept;              // "\x1b[38;5;Nm"

// 仅 C++17（受 __cpp_lib_optional 保护）：
std::string gen_rgb_seq(const std::optional<RgbColor>& fg = std::nullopt,
                        const std::optional<RgbColor>& bg = std::nullopt) noexcept;
```

### `term.h`

```cpp
bool supports_truecolor() noexcept;            // 主动探测，不使用环境变量
bool supports_vt100() noexcept;                // 主动探测，不使用环境变量
bool enable_vt100() noexcept;                  // POSIX 下无操作，返回 true；
                                               // Windows 下启用 VT 处理
std::pair<std::size_t, std::size_t> term_size() noexcept;   // {rows, cols}，失败返回 {0,0}
```

## 测试

```bash
ezmk test
```

内置测试框架将 `test/test_vt100_utils.cpp` 编译为独立可执行文件，
覆盖转义序列精确输出、颜色数学、DA1/XTGETTCAP 应答解析以及 `term_size` 不变式。
