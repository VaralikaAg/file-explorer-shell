#include "myheader.h"

void renderStatusBar() {
    posx(app.ui.rows-2, 0);
    cout << "\033[1;34mCurrent Path: " << app.nav.currPath << "\033[0m\n";

    posx(1,1);
    print_details();
}

void showStatusMessage(const string &msg, const string &color)
{
    posx(app.ui.rows - 2, 0);
    printf("\033[K");
    printf("%s%s\033[0m", color.c_str(), msg.c_str());
    pos();
}