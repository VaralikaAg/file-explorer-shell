#include "myheader.h"

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

std::string get_input() {
    std::string commandLine;
    char ch;

    while (true) {
        ch = getchar();   // use getchar(), not cin

        /* ENTER */
        if (ch == '\n' || ch == '\r') {
            break;
        }

        /* BACKSPACE / DELETE */
        if (ch == 127 || ch == '\b') {
            if (!commandLine.empty()) {
                printf("\b \b");
                commandLine.pop_back();
            }
            continue;
        }

        /* ESC SEQUENCE (arrow keys, home, end, etc.) */
        if (ch == 27) {   // ESC
            getchar();    // skip '['
            getchar();    // skip final char (A/B/C/D)
            continue;     // IGNORE completely
        }

        /* Printable characters only */
        if (isprint(ch)) {
            commandLine.push_back(ch);
            putchar(ch);
        }
    }

    return commandLine;
}