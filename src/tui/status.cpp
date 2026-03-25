#include "myheader.hpp"

void showStatusMessage(const std::string &msg, const std::string &color, int wait_ms)
{
    setCursorPos(app.ui.rows - 2, 0);
    std::cout << ANSI::CLEAR_LINE;
    std::cout << color << msg << ANSI::RESET << std::flush;
    setDefaultCursorPos();

    if (wait_ms > 0) {
        std::this_thread::sleep_for(std::chrono::milliseconds(wait_ms));
        setCursorPos(app.ui.rows - 2, 0);
        std::cout << ANSI::CLEAR_LINE << std::flush;
        setDefaultCursorPos();
    }
}