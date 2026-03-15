#include "myheader.h"

// Signal handler for SIGINT (Ctrl+C)
void handleSigint(int signum) {
    
    tcsetattr(fileno(stdin), TCSANOW, &initialrsettings);
    printf("\033[0m"); 
    resetCursorColor();
    // printf("\033]12;white\007"); 
    fputs("\033[2J", stdout) ;
    fprintf(stdout, "\033[%d;%dH", 0, 0);

    exit(signum);
}

void handleResize(int sig) {
    app.layout.resized=1;
}

void cleanupAndExit() {
    tcsetattr(STDIN_FILENO, TCSANOW, &initialrsettings);
    resetCursorColor();
    printf("\033[0m");
    printf("\033[2J");
    printf("\033[1;1H");
    fflush(stdout);
    exit(0);
}
