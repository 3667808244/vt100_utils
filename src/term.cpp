#include "vt100_utils/term.h"

#include "vt100_utils/detail/probe.h"

#include <cctype>
#include <cstring>

#ifdef _WIN32

#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

#ifndef ENABLE_VIRTUAL_TERMINAL_PROCESSING
#define ENABLE_VIRTUAL_TERMINAL_PROCESSING 0x0004
#endif

#else

#include <cerrno>
#include <chrono>
#include <poll.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>

#endif

namespace vt100_utils {
namespace detail {

// Platform-independent reply parsers.

bool parse_da1_response(const std::string &resp) noexcept {
	const char esc[] = "\x1b[?";
	std::size_t pos = resp.find(esc);
	if (pos == std::string::npos)
		return false;

	std::size_t i = pos + (sizeof(esc) - 1);
	if (i >= resp.size() || !std::isdigit(static_cast<unsigned char>(resp[i])))
		return false;

	while (i < resp.size() && (std::isdigit(static_cast<unsigned char>(resp[i])) || resp[i] == ';'))
		++i;

	return i < resp.size() && resp[i] == 'c';
}

bool parse_xtgettcap_rgb(const std::string &resp) noexcept {
	const char name[] = "6803=";
	std::size_t pos = resp.find(name);
	if (pos == std::string::npos)
		return false;

	// Only inspect the value of this reply: from '=' up to the ST terminator.
	const std::size_t begin = pos + (sizeof(name) - 1);
	std::size_t end = resp.find("\x1b\\", begin);
	if (end == std::string::npos)
		end = resp.size();

	// Standard XTGETTCAP replies hex-encode the value ("RGB" -> "524742");
	// some terminals send the literal name instead.
	return resp.find("524742", begin) < end || resp.find("RGB", begin) < end;
}

} // namespace detail

#ifdef _WIN32

namespace {

// Enables VT processing on stdout and leaves it enabled (required for the
// console to render ANSI sequences). Existing mode flags are preserved;
// only the two flags VT output needs are added.
bool enable_vt_processing() noexcept {
	HANDLE h = ::GetStdHandle(STD_OUTPUT_HANDLE);
	if (h == nullptr || h == INVALID_HANDLE_VALUE)
		return false;

	DWORD mode = 0;
	if (!::GetConsoleMode(h, &mode))
		return false;

	mode |= ENABLE_PROCESSED_OUTPUT | ENABLE_VIRTUAL_TERMINAL_PROCESSING;
	return ::SetConsoleMode(h, mode) != 0;
}

} // namespace

namespace detail {

// Windows consoles do not expose capabilities through escape-query replies.
std::string query_terminal(const char *cmd, int timeout_ms) {
	(void)cmd;
	(void)timeout_ms;
	return std::string();
}

} // namespace detail

bool supports_vt100() {
	return enable_vt_processing();
}

bool supports_truecolor() {
	// Heuristic: a console that accepts VT processing is assumed to render
	// 24-bit color as well (Windows Terminal and current conhost do).
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

		const int rows = static_cast<int>(csbi.srWindow.Bottom - csbi.srWindow.Top) + 1;
		const int cols = static_cast<int>(csbi.srWindow.Right - csbi.srWindow.Left) + 1;
		if (rows > 0 && cols > 0)
			return std::make_pair(static_cast<std::size_t>(rows), static_cast<std::size_t>(cols));
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

std::string query_terminal(const char *cmd, int timeout_ms) {
	if (cmd == nullptr || timeout_ms < 0)
		return std::string();
	if (!::isatty(STDIN_FILENO) || !::isatty(STDOUT_FILENO))
		return std::string();

	TermiosGuard guard;
	if (::tcgetattr(STDIN_FILENO, &guard.saved) != 0)
		return std::string();

	struct termios raw = guard.saved;
	raw.c_lflag &= ~(ICANON | ECHO);
	raw.c_iflag &= ~(ICRNL | IXON);
	raw.c_cc[VMIN] = 0;
	raw.c_cc[VTIME] = 0;
	// TCSAFLUSH also drops pending input so stale bytes are not read as a reply.
	if (::tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) != 0)
		return std::string();
	guard.active = true;

	const std::size_t len = std::strlen(cmd);
	std::size_t written = 0;
	while (written < len) {
		const ssize_t n = ::write(STDOUT_FILENO, cmd + written, len - written);
		if (n < 0) {
			if (errno == EINTR)
				continue;
			return std::string();
		}
		if (n == 0)
			return std::string();
		written += static_cast<std::size_t>(n);
	}

	typedef std::chrono::steady_clock clock_type;
	const clock_type::time_point deadline =
	    clock_type::now() + std::chrono::milliseconds(timeout_ms);

	std::string resp;
	resp.reserve(64);
	char buf[256];
	while (true) {
		const clock_type::duration left = deadline - clock_type::now();
		if (left <= clock_type::duration::zero())
			break;

		long wait_ms =
		    static_cast<long>(std::chrono::duration_cast<std::chrono::milliseconds>(left).count());
		if (wait_ms <= 0)
			wait_ms = 1;

		struct pollfd pfd;
		pfd.fd = STDIN_FILENO;
		pfd.events = POLLIN;
		pfd.revents = 0;
		const int r = ::poll(&pfd, 1, static_cast<int>(wait_ms));
		if (r < 0) {
			if (errno == EINTR)
				continue;
			break;
		}
		if (r == 0)
			break;
		if (!(pfd.revents & POLLIN))
			break;

		const ssize_t n = ::read(STDIN_FILENO, buf, sizeof(buf));
		if (n < 0) {
			if (errno == EINTR)
				continue;
			break;
		}
		if (n == 0)
			break;
		resp.append(buf, static_cast<std::size_t>(n));
	}
	return resp;
}

} // namespace detail

bool supports_vt100() {
	return detail::parse_da1_response(detail::query_terminal("\x1b[c", 150));
}

bool supports_truecolor() {
	return detail::parse_xtgettcap_rgb(detail::query_terminal("\x1bP+q6803\x1b\\", 150));
}

bool enable_vt100() noexcept {
	// POSIX terminals need no enabling; report whether stdout is a terminal.
	return ::isatty(STDOUT_FILENO) != 0;
}

std::pair<std::size_t, std::size_t> term_size() noexcept {
	const int fds[] = {STDOUT_FILENO, STDIN_FILENO, STDERR_FILENO};
	for (std::size_t i = 0; i < sizeof(fds) / sizeof(fds[0]); ++i) {
		const int fd = fds[i];
		struct winsize ws;
		if (::isatty(fd) && ::ioctl(fd, TIOCGWINSZ, &ws) == 0 && ws.ws_row > 0 && ws.ws_col > 0)
			return std::make_pair(static_cast<std::size_t>(ws.ws_row),
			                      static_cast<std::size_t>(ws.ws_col));
	}
	return std::make_pair(static_cast<std::size_t>(0), static_cast<std::size_t>(0));
}

#endif // _WIN32

} // namespace vt100_utils
