#include "myheader.hpp"

bool inputAvailable()
{
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(STDIN_FILENO, &fds);

    timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 50000; // 50 ms wait

    return select(STDIN_FILENO + 1, &fds, nullptr, nullptr, &tv) > 0;
}

std::string getInput() {
    std::string command_line;
    char ch;

    while (true) {
        ch = std::cin.get();   // use std::cin.get(), not cin >>

        /* ENTER */
        if (ch == '\n' || ch == '\r') {
            break;
        }

        /* BACKSPACE / DELETE */
        if (ch == 127 || ch == '\b') {
            if (!command_line.empty()) {
                std::cout << "\b \b" << std::flush;
                command_line.pop_back();
            }
            continue;
        }

        /* ESC SEQUENCE (arrow keys, home, end, etc.) */
        if (ch == 27) {   // ESC
            std::cin.get();    // skip '['
            std::cin.get();    // skip final char (A/B/C/D)
            continue;     // IGNORE completely
        }

        /* Printable characters only */
        if (isprint(ch)) {
            command_line.push_back(ch);
            std::cout << ch << std::flush;
        }
    }

    return command_line;
}