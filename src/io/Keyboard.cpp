#include "Keyboard.hpp"

#include <stdexcept>

#if defined(_WIN32)
#include <conio.h>

namespace Keyboard {
bool keyPressed() {
    return _kbhit() != 0;
}

std::string getKey() {
    const int key = _getch();
    if (key == 13) return "ENTER";
    if (key == 27) return "ESC";
    if (key == 0 || key == 224) {
        switch (_getch()) {
            case 72: return "UP";
            case 80: return "DOWN";
            case 75: return "LEFT";
            case 77: return "RIGHT";
            default: return "SPECIAL";
        }
    }
    return std::string(1, static_cast<char>(key));
}
}

#elif defined(__linux__) || defined(__APPLE__)
#include <fcntl.h>
#include <sys/select.h>
#include <termios.h>
#include <unistd.h>

namespace {
class TerminalMode {
public:
    TerminalMode() {
        if (!isatty(STDIN_FILENO)) {
            throw std::runtime_error("Keyboard input requires an interactive terminal.");
        }
        if (tcgetattr(STDIN_FILENO, &original) == -1) {
            throw std::runtime_error("Could not read terminal settings for keyboard input.");
        }
        termios raw = original;
        raw.c_lflag &= static_cast<tcflag_t>(~(ICANON | ECHO));
        raw.c_cc[VMIN] = 1;
        raw.c_cc[VTIME] = 0;
        if (tcsetattr(STDIN_FILENO, TCSANOW, &raw) == -1) {
            throw std::runtime_error("Could not enable keyboard input mode.");
        }
        active = true;
    }

    ~TerminalMode() {
        if (active) tcsetattr(STDIN_FILENO, TCSANOW, &original);
    }

private:
    termios original{};
    bool active = false;
};

TerminalMode& terminalMode() {
    static TerminalMode mode;
    return mode;
}
}

namespace Keyboard {
bool keyPressed() {
    (void)terminalMode();
    fd_set input;
    FD_ZERO(&input);
    FD_SET(STDIN_FILENO, &input);
    timeval timeout{0, 0};
    const int ready = select(STDIN_FILENO + 1, &input, nullptr, nullptr, &timeout);
    if (ready < 0) throw std::runtime_error("Could not check keyboard input.");
    return ready > 0;
}

std::string getKey() {
    (void)terminalMode();
    char key{};
    if (read(STDIN_FILENO, &key, 1) != 1) {
        throw std::runtime_error("Could not read a key from the terminal.");
    }
    if (key == '\n' || key == '\r') return "ENTER";
    if (key == '\x1b') return "ESC";
    return std::string(1, key);
}
}

#else
namespace Keyboard {
bool keyPressed() {
    throw std::runtime_error("keyPressed() is not supported on this operating system.");
}

std::string getKey() {
    throw std::runtime_error("getKey() is not supported on this operating system.");
}
}
#endif
