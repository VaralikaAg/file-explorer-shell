#include "myheader.h"

void renderStatusBar() {
    setCursorPos(app.ui.rows-2, 0);
    std::cout << "\033[1;34mCurrent Path: " << app.nav.currPath << "\033[0m\n";

    setCursorPos(1,1);
    print_details();
}

void showStatusMessage(const std::string &msg, const std::string &color)
{
    setCursorPos(app.ui.rows - 2, 0);
    printf("\033[K");
    printf("%s%s\033[0m", color.c_str(), msg.c_str());
    setDefaultCursorPos();
}