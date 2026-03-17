// Including the header file
#include "myheader.h"
AppState app;

// Main method
int main(int argc, char *argv[])
{
    // Alternate Screen + Mouse support
    printf("\033[?1049h\033[?1000h");
    fflush(stdout);

    loadConfig();

    signal(SIGINT, SIG_IGN);
    signal(SIGWINCH, handleResize);
    signal(SIGCHLD, SIG_IGN);
    get_terminal_size();
    app.layout.rowSize = app.ui.rows - 10;
    app.layout.colSize = app.ui.cols / 3;
    app.layout.resized = 0;

    startIndexing();

    initializeNavigation(argc, argv);

    std::cout << "Opening directory: " << app.nav.currPath << "\n ";
    openDirectory(app.nav.root.c_str(), app.nav.up_screen, app.nav.down_screen);

    navigate();

    return 0;
}