#include "myheader.h"

struct termios initialrsettings, newrsettings;

string getSelectedPath() {
    string selectedFile = app.nav.fileList[app.nav.xcurr + app.nav.up_screen - 1];
    string path = app.nav.currPath;
    if (path.back() != '/') path += "/";
    return path + selectedFile;
}

bool isCurrentSelectionDir() {
    struct stat st;
    string path = getSelectedPath();
    if (lstat(path.c_str(), &st) != 0) return false;
    return S_ISDIR(st.st_mode);
}

void moveUp() {
    if (app.nav.xcurr > 1) {
        app.nav.xcurr--;
    }
    else if (app.nav.up_screen > 0) {
        app.nav.up_screen--;
        app.nav.down_screen++;
    }
}

void moveDown() {
    if (app.nav.xcurr < app.layout.rowSize &&
        app.nav.xcurr < (int)app.nav.fileList.size()) {
        app.nav.xcurr++;
    }
    else if (app.nav.xcurr == app.layout.rowSize &&
             app.nav.down_screen > 0) {
        app.nav.up_screen++;
        app.nav.down_screen--;
    }
}

void enterDirectory(const string &newPath) {
    NavState currentState;
    currentState.path = app.nav.currPath;
    currentState.xcurr = app.nav.xcurr;
    currentState.up_screen = app.nav.up_screen;

    app.nav.backStack.push(currentState);

    app.nav.currPath = newPath;

    openDirectory(app.nav.currPath.c_str(),
                  app.nav.up_screen,
                  app.nav.down_screen);

    app.nav.xcurr = 1;
}

void goBack() {
    if (!app.nav.backStack.empty()) {
        NavState prev = app.nav.backStack.top();
        app.nav.backStack.pop();

        app.nav.currPath = prev.path;
        openCurrDirectory(app.nav.currPath.c_str());

        app.nav.xcurr = prev.xcurr;
        app.nav.up_screen = prev.up_screen;
        app.nav.down_screen =
            (int)app.nav.fileList.size() - app.nav.up_screen - app.layout.rowSize;
    }
}

void handleEnter() {
    string newPath = getSelectedPath();

    struct stat sb;
    stat(newPath.c_str(), &sb);

    if (S_ISDIR(sb.st_mode)) {
        openCurrDirectory(newPath.c_str());

        if (!app.nav.fileList.empty()) {
            enterDirectory(newPath);
        }
    }
    else if (S_ISREG(sb.st_mode)) {
        pid_t pid = fork();

        if (pid == 0) {
            int nullFile = open("/dev/null", O_WRONLY);
            dup2(nullFile, STDERR_FILENO);
            close(nullFile);

            execlp("xdg-open", "xdg-open", newPath.c_str(), NULL);
            exit(EXIT_FAILURE);
        }
    }
}

void handleArrowKeys() {
    char ch = cin.get();

    if (ch == 'A') {            // UP
        stopFolderScan();
        moveUp();
    }
    else if (ch == 'B') {       // DOWN
        stopFolderScan();
        moveDown();
    }
    else if (ch == 'C') {       // RIGHT
        stopFolderScan();
        app.selection.selectedFiles.clear();

        string newPath = getSelectedPath();

        if (isDirectory(newPath.c_str())) {
            openCurrDirectory(newPath.c_str());
            enterDirectory(newPath);
        }
    }
    else if (ch == 'D') {       // LEFT
        stopFolderScan();
        app.selection.selectedFiles.clear();
        goBack();
    }
}

void processKey(char ch) {

    if (ch == 27) {  // ESC
        cin.get();   // skip '['
        handleArrowKeys();
    }
    else if (ch == 'c') {
        copy();
    }
    else if (ch == 'p') {
        string pasted = paste();
        refreshCurrentDirectory();
        if (!pasted.empty()) update_position(pasted);
        
    }
    else if (ch == 'd') {
        deleteSelectedItems();
    }
    else if (ch == ':') {
        renderUI();
        commandMode();
    }
    else if (ch == ' ') {
        toggleSelect();
    }
    else if (ch == 'u') {
        app.selection.selectedFiles.clear();
    }
    else if (ch == 127 || ch == 8) {
        goBack();
    }
    else if (ch == '\n' || ch == '\r') {
        handleEnter();
    }
}

void navigate() {

    app.nav.currPath = app.nav.root;

    openDirectory(app.nav.currPath.c_str(),
                  app.nav.up_screen,
                  app.nav.down_screen);

    app.nav.up_screen =
        (int)app.nav.fileList.size() > app.layout.rowSize
            ? (int)app.nav.fileList.size() - app.layout.rowSize
            : 0;

    app.nav.down_screen =
        (int)app.nav.fileList.size() - app.nav.up_screen - app.layout.rowSize;

    app.nav.xcurr =
        app.layout.rowSize < (int)app.nav.fileList.size()
            ? app.layout.rowSize
            : (int)app.nav.fileList.size();

    renderUI();
    pos();

    char ch;

    tcgetattr(fileno(stdin), &initialrsettings);
    newrsettings = initialrsettings;

    newrsettings.c_lflag &= ~(ICANON | ECHO);
    newrsettings.c_lflag |= ECHOE;

    tcsetattr(fileno(stdin), TCSANOW, &newrsettings);

    while (true) {

        handleResizeIfNeeded();

        if (app.ui.refresh) {
            print_details();
            app.ui.refresh = false;
        }

        if (inputAvailable()) {
            ch = cin.get();

            processKey(ch);

            refreshCurrentDirectory();
            renderUI();
            pos();
        }
    }
}