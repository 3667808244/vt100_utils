#include "vt100_utils/term.h"

#include "probe.h"

#include <cctype>
#include <cstring>

#ifdef _WIN32

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#ifndef ENABLE_VIRTUAL_TERMINAL_PROCESSING
#define ENABLE_VIRTUAL_TERMINAL_PROCESSING 0x0004
#endif

#else

#include <poll.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>

#endif

namespace vt100_utils {

#ifdef _WIN32
namespace {

// 探测并启用 stdout 控制台的 VT 处理；启用成功后保持开启（VT 为 Windows
// 控制台渲染 ANSI 序列的前提）。stdout 被重定向或非控制台时返回 false。
bool enable_vt_processing() noexcept {
	HANDLE h = ::GetStdHandle(STD_OUTPUT_HANDLE);
	if (h == nullptr || h == INVALID_HANDLE_VALUE)
		return false;

	DWORD mode = 0;
	if (!::GetConsoleMode(h, &mode))
		return false;

	mode |= ENABLE_PROCESSED_OUTPUT | ENABLE_WRAP_AT_EOL_OUTPUT |
	        ENABLE_VIRTUAL_TERMINAL_PROCESSING;
	return ::SetConsoleMode(h, mode) != 0;
}

} // namespace

namespace detail {

bool parse_da1_response(const std::string &resp) noexcept {
	const char esc[] = "\x1b[?";
	std::size_t pos = resp.find(esc);
	if (pos == std::string::npos)
		return false;

	std::size_t i = pos + (sizeof(esc) - 1);
	if (i >= resp.size() || !std::isdigit(static_cast<unsigned char>(resp[i])))
		return false;

	while (i < resp.size() &&
	       (std::isdigit(static_cast<unsigned char>(resp[i])) || resp[i] == ';'))
		++i;

	return i < resp.size() && resp[i] == 'c';
}

bool parse_xtgettcap_rgb(const std::string &resp) noexcept {
	std::size_t pos = resp.find("6803=");
	if (pos == std::string::npos)
		return false;
	return resp.find("RGB", pos) != std::string::npos;
}

// Windows 控制台不通过转义查询应答来暴露能力，此实现仅在 POSIX 下生效。
std::string query_terminal(const char *cmd, int timeout_ms) noexcept {
	(void)cmd;
	(void)timeout_ms;
	return std::string();
}

} // namespace detail

bool supports_vt100() noexcept {
	return enable_vt_processing();
}

bool supports_truecolor() noexcept {
	// Windows 控制台一旦支持 VT 处理即支持 24 位真彩色（Windows Terminal /
	// 现代 conhost），故与 supports_vt100 结论一致。
	return enable_vt_processing();
}

bool enable_vt100() noexcept {
	return enable_vt_processing();
}

std::pair<std::size_t, std::size_t> term_size() noexcept {
	const DWORD handles[] = {STD_OUTPUT_HANDLE, STD_INPUT_HANDLE, STD_ERROR_HANDLE};
	for (std::size_t i = 0; i < sizeof(handles) / sizeof(handles[0]); ++i) {
		HANDLE h = ::GetStdHandle(handles[i]);
		if (h == nullptr || h == INVALID_HANDLE_VALUE)
			continue;

		CONSOLE_SCREEN_BUFFER_INFO csbi;
		if (!::GetConsoleScreenBufferInfo(h, &csbi))
			continue;

		int rows = static_cast<int>(csbi.srWindow.Bottom - csbi.srWindow.Top) + 1;
		int cols = static_cast<int>(csbi.srWindow.Right - csbi.srWindow.Left) + 1;
		if (rows > 0 && cols > 0)
			return std::make_pair(static_cast<std::size_t>(rows),
			                      static_cast<std::size_t>(cols));
	}
	return std::make_pair(static_cast<std::size_t>(0), static_cast<std::size_t>(0));
}

#else // !_WIN32

namespace {

struct TermiosGuard {
	bool active = false;
	struct termios saved;

	~TermiosGuard() {
		if (active)
			::tcsetattr(STDIN_FILENO, TCSANOW, &saved);
	}
};

} // namespace

namespace detail {

bool parse_da1_response(const std::string &resp) noexcept {
	const char esc[] = "\x1b[?";
	std::size_t pos = resp.find(esc);
	if (pos == std::string::npos)
		return false;

	std::size_t i = pos + (sizeof(esc) - 1);
	if (i >= resp.size() || !std::isdigit(static_cast<unsigned char>(resp[i])))
		return false;

	while (i < resp.size() &&
	       (std::isdigit(static_cast<unsigned char>(resp[i])) || resp[i] == ';'))
		++i;

	return i < resp.size() && resp[i] == 'c';
}

bool parse_xtgettcap_rgb(const std::string &resp) noexcept {
	std::size_t pos = resp.find("6803=");
	if (pos == std::string::npos)
		return false;
	return resp.find("RGB", pos) != std::string::npos;
}

std::string query_terminal(const char *cmd, int timeout_ms) noexcept {
	if (!::isatty(STDIN_FILENO) || !::isatty(STDOUT_FILENO))
		return std::string();

	TermiosGuard guard;
	if (::tcgetattr(STDIN_FILENO, &guard.saved) != 0)
		return std::string();
	guard.active = true;

	struct termios raw = guard.saved;
	raw.c_lflag &= ~(ICANON | ECHO);
	raw.c_cc[VMIN] = 0;
	raw.c_cc[VTIME] = 0;
	if (::tcsetattr(STDIN_FILENO, TCSANOW, &raw) != 0)
		return std::string();

	::tcflush(STDIN_FILENO, TCIFLUSH);

	std::size_t len = std::strlen(cmd);
	if (::write(STDOUT_FILENO, cmd, len) != static_cast<ssize_t>(len))
		return std::string();
	::tcdrain(STDOUT_FILENO);

	std::string resp;
	char buf[256];
	while (true) {
		struct pollfd pfd;
		pfd.fd = STDIN_FILENO;
		pfd.events = POLLIN;
		int r = ::poll(&pfd, 1, timeout_ms);
		if (r <= 0 || !(pfd.revents & POLLIN))
			break;

		ssize_t n = ::read(STDIN_FILENO, buf, sizeof(buf));
		if (n <= 0)
			break;
		resp.append(buf, static_cast<std::size_t>(n));
	}
	return resp;
}

} // namespace detail

bool supports_vt100() noexcept {
	return detail::parse_da1_response(detail::query_terminal("\x1b[c", 150));
}

bool supports_truecolor() noexcept {
	return detail::parse_xtgettcap_rgb(detail::query_terminal("\x1bP+q6803\x1b\\", 150));
}

bool enable_vt100() noexcept {
	return true;
}

std::pair<std::size_t, std::size_t> term_size() noexcept {
	const int fds[] = {STDOUT_FILENO, STDIN_FILENO, STDERR_FILENO};
	for (std::size_t i = 0; i < sizeof(fds) / sizeof(fds[0]); ++i) {
		int fd = fds[i];
		struct winsize ws;
		if (::isatty(fd) && ::ioctl(fd, TIOCGWINSZ, &ws) == 0 && ws.ws_row > 0 && ws.ws_col > 0)
			return std::make_pair(static_cast<std::size_t>(ws.ws_row),
			                      static_cast<std::size_t>(ws.ws_col));
	}
	return std::make_pair(static_cast<std::size_t>(0), static_cast<std::size_t>(0));
}

#endif // _WIN32

} // namespace vt100_utils
