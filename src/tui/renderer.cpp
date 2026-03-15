#include "myheader.h"

void display(string fileName, string root)
{
    string path = root + '/' + fileName;
    string displayFile = fileName;

    bool selected = app.selection.selectedFiles.count(path);

    if ((int)displayFile.length() > app.layout.colSize - 3)
        displayFile = displayFile.substr(0, app.layout.colSize-6) + "...";

    if (selected)
        printf("\033[7m");

    if(isDirectory(path.c_str())){
        printf("\033[1;32m%s\033[0m\n", displayFile.c_str());
    }
    else {
        printf("%s\n", displayFile.c_str());
    }

    if (selected)
        printf("\033[0m");
}

void displayTextFile(const string &filepath) {
    ifstream file(filepath);
    if (!file) {
        posx(1, 2*app.layout.colSize);
        printf("\033[1;31mError Opening File\033[0m\n");
        return;
    }

    string line;
    int yPos = 1;  // Start from the first line

    while (getline(file, line)) {
        if ((int)line.length() > app.layout.colSize - 3)
        {
            line = line.substr(0, app.layout.colSize-7) + "...";  // Truncate after 30 chars
        }

        posx(yPos++, 2*app.layout.colSize);  // Print each line at a new y-coordinate
        printf("%s\n", line.c_str());

        if (static_cast<int>(yPos) >= app.ui.rows - 5) {  // Avoid screen overflow (adjust as needed)
            posx(app.ui.rows-5, 2*app.layout.colSize);
            printf("\033[1;33m... Output Truncated\033[0m\n");
            break;
        }
    }

    file.close();
}

void showTempMessage(const string &msg, int wait_ms=1000) {
    posx(app.ui.rows - 2, 0);
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
    posx(app.ui.rows - 2, 0);
    printf("\033[K");
    fflush(stdout);
}

void renderUI()
{
    clearScreen;
    setCursorRed();

    /* -------- MIDDLE PANEL -------- */
    openCurrDirectory(app.nav.currPath.c_str());

    if (app.nav.fileList.empty()) {
        posx(1, app.layout.colSize + 2);
        printf("\033[1;31mNo Items\033[0m\n");
    }
    else {
        for (int i = app.nav.up_screen, line = 1;
             i < app.nav.up_screen + app.layout.rowSize &&
             i < (int)app.nav.fileList.size();
             i++, line++)
        {
            posx(line, app.layout.colSize + 2);
            display(app.nav.fileList[i], app.nav.currPath);
        }
    }

    /* -------- RIGHT PANEL -------- */
    if (!app.nav.fileList.empty())
    {
        string newPath = getSelectedPath();

        struct stat st;
        if (lstat(newPath.c_str(), &st) == 0)
        {
            bool isDir = S_ISDIR(st.st_mode);
            bool isReadable = (st.st_mode & S_IRUSR);

            if (!isDir) {
                renderFilePreview(newPath, isReadable);
            } else {
                renderDirectoryPreview(newPath);
            }

            /* -------- FILE DETAILS -------- */
            posx(1,1);
            getFileDetails(newPath);
            print_details();
        }
    }

    /* -------- STATUS BAR -------- */
    posx(app.ui.rows-2, 0);
    printf("\033[1;34mCurrent Path: %s\033[0m\n", app.nav.currPath.c_str());

    /* -------- RESTORE STATE -------- */
    openCurrDirectory(app.nav.currPath.c_str());

    app.nav.ycurr = app.layout.colSize;
    pos();
}