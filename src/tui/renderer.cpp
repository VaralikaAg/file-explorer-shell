#include "myheader.h"

void display(const std::string &fileName,const fs::path &root) noexcept
{
    fs::path fullPath = root / fileName;
    std::string displayFile = fileName;

    bool selected = app.selection.selectedFiles.count(fullPath.string());

    // 🔹 truncate safely
    if ((int)displayFile.length() > app.layout.colSize - 3) {
        displayFile = displayFile.substr(0, app.layout.colSize - 6) + "...";
    }

    // 🔹 highlight selected
    if (selected)
        std::cout << "\033[7m";

    // 🔹 directory coloring
    if (isDirectory(fullPath)) {
        std::cout << "\033[1;32m" << displayFile << "\033[0m\n";
    } else {
        std::cout << displayFile << "\n";
    }

    // 🔹 reset highlight
    if (selected)
        std::cout << "\033[0m";
}

void displayTextFile(const std::string &filepath) {
    std::ifstream file(filepath);
    if (!file) {
        setCursorPos(1, 2*app.layout.colSize);
        printf("\033[1;31mError Opening File\033[0m\n");
        return;
    }

    std::string line;
    int yPos = 1;  // Start from the first line

    while (std::getline(file, line))
    {
        if ((int)line.length() > app.layout.colSize - 3)
        {
            line = line.substr(0, app.layout.colSize-7) + "...";  // Truncate after 30 chars
        }

        setCursorPos(yPos++, 2*app.layout.colSize);  // Print each line at a new y-coordinate
        printf("%s\n", line.c_str());

        if (static_cast<int>(yPos) >= app.ui.rows - 5) {  // Avoid screen overflow (adjust as needed)
            setCursorPos(app.ui.rows-5, 2*app.layout.colSize);
            printf("\033[1;33m... Output Truncated\033[0m\n");
            break;
        }
    }

    file.close();
}

void showTempMessage(const std::string &msg, int wait_ms=1000) {
    setCursorPos(app.ui.rows - 2, 0);
    printf("\033[K");
    printf("\033[1;31m%s\033[0m", msg.c_str());
    fflush(stdout);

    // Wait either for key press OR timeout
    fd_set set;
    struct timeval timeout;

    FD_ZERO(&set);
    FD_SET(STDIN_FILENO, &set);

    timeout.tv_sec = wait_ms / 1000;
    timeout.tv_usec = (wait_ms % 1000) * 1000;

    select(STDIN_FILENO + 1, &set, NULL, NULL, &timeout);

    // Clear message line
    setCursorPos(app.ui.rows - 2, 0);
    printf("\033[K");
    fflush(stdout);
}

void renderUI()
{
    clearScreen();
    setCursorRed();

    /* -------- MIDDLE PANEL -------- */
    renderMiddlePanel();

    /* -------- RIGHT PANEL -------- */
    renderRightPanel();

    /* -------- STATUS BAR -------- */
    setCursorPos(app.ui.rows - 2, 0);
    printf("\033[1;34mCurrent Path: %s\033[0m\n", app.nav.currPath.c_str());

    /* -------- RESTORE STATE -------- */
    openCurrDirectory(app.nav.currPath.c_str());

    app.nav.ycurr = app.layout.colSize;
    setDefaultCursorPos();
}