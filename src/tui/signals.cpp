#include "myheader.hpp"

// Signal handler for SIGINT (Ctrl+C)
void handleSigint(int signum) {
    
    tcsetattr(fileno(stdin), TCSANOW, &initial_rsettings);
    std::cout << ANSI::RESET << std::flush; 
    resetCursorColor();
    std::cout << ANSI::CLEAR_SCREEN << ANSI::MOVE_TO(0, 0) << std::flush;

    exit(signum);
}

void handleResize(int sig) {
    app.layout.resized=1;
}

void cleanupAndExit() {
    tcsetattr(STDIN_FILENO, TCSANOW, &initial_rsettings);
    resetCursorColor();
    std::cout << ANSI::RESET;
    std::cout << ANSI::ALT_SCREEN_OFF << ANSI::MOUSE_TRACK_OFF;
    std::cout << ANSI::CLEAR_SCREEN;
    std::cout << ANSI::HOME << std::flush;
    fflush(stdout);
    exit(0);
}
