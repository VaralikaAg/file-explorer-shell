#include "myheader.hpp"

void getTerminalSize() {
    winsize w;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
    app.ui.rows = w.ws_row;
    app.ui.cols = w.ws_col;
}

void hideCursor() {
    std::cout << ANSI::HIDE_CURSOR << std::flush;
}

void showCursor() {
    std::cout << ANSI::SHOW_CURSOR << std::flush;
}