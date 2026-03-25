// Including the header file
#include "myheader.hpp"
AppState app;

// Main method
int main(int argc, char *argv[])
{
    // Alternate Screen + Mouse support
    std::cout << "\033[?1049h\033[?1000h" << std::flush;

    loadConfig();

    signal(SIGINT, SIG_IGN);
    signal(SIGWINCH, handleResize);
    signal(SIGCHLD, SIG_IGN);
    getTerminalSize();
    app.layout.row_size = app.ui.rows - 10;
    app.layout.col_size = app.ui.cols / 3;
    app.layout.resized = 0;

    startIndexing();

    initializeNavigation(argc, argv);

    openDirectory(app.nav.root, app.nav.up_screen, app.nav.down_screen);

    navigate();

    return 0;
}