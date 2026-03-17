#include "myheader.h"

struct termios initialrsettings, newrsettings;

fs::path getSelectedPath() noexcept {
    const std::string& selectedFile =
        app.nav.fileList[app.nav.xcurr + app.nav.up_screen - 1];

    return fs::path(app.nav.currPath) / selectedFile;
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

void enterDirectory(const std::string &newPath) {
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

void openFile(const std::string& path) noexcept {
#ifdef _WIN32
    std::string cmd = "start \"\" \"" + path + "\"";
#elif __APPLE__
    std::string cmd = "open \"" + path + "\"";
#else
    std::string cmd = "xdg-open \"" + path + "\"";
#endif

    std::system(cmd.c_str()); // simple and portable
}

void handleEnter() {
    fs::path newPath = getSelectedPath();

    if (isDirectory(newPath)) {
        openCurrDirectory(newPath.c_str());

        if (!app.nav.fileList.empty()) {
            enterDirectory(newPath);
        }
    }
    else if (isRegularFile(newPath)) {
        openFile(newPath);
    }
}

// Map raw input char to KeyAction
KeyAction mapKey(char ch, bool isEscSequence = false) {
    if (isEscSequence) { // Arrow keys after ESC + '['
        switch (ch) {
            case 'A': return KeyAction::UP;
            case 'B': return KeyAction::DOWN;
            case 'C': return KeyAction::RIGHT;
            case 'D': return KeyAction::LEFT;
            default:  return KeyAction::UNKNOWN;
        }
    } else { // Normal keys
        switch (ch) {
            case 'c': return KeyAction::COPY;
            case 'p': return KeyAction::PASTE;
            case 'd': return KeyAction::DELETE;
            case ':': return KeyAction::COMMAND;
            case ' ': return KeyAction::TOGGLE_SELECT;
            case 'u': return KeyAction::CLEAR_SELECTION;
            case 127:
            case 8: return KeyAction::BACK;
            case '\n':
            case '\r': return KeyAction::ENTER;
            default: return KeyAction::UNKNOWN;
        }
    }
}

// Unified processKey function
void processKey(char ch) {
    KeyAction action = KeyAction::UNKNOWN;

    if (ch == 27) { // ESC sequence
        char next = std::cin.get();
        if (next == '[') {
            char arrow = std::cin.get();
            action = mapKey(arrow, true);
        }
    } else {
        action = mapKey(ch);
    }

    // Handle the action
    switch (action) {
        case KeyAction::UP: 
            stopFolderScan(); 
            moveUp(); 
            break;

        case KeyAction::DOWN: 
            stopFolderScan(); 
            moveDown(); 
            break;

        case KeyAction::RIGHT: 
        {
            stopFolderScan();
            app.selection.selectedFiles.clear();

            fs::path newPath = getSelectedPath();
            if (isDirectory(newPath)) {
                openCurrDirectory(newPath.c_str());
                enterDirectory(newPath);
            }
            break;
        }

        case KeyAction::LEFT: 
        {
            stopFolderScan();
            app.selection.selectedFiles.clear();
            goBack();
            break;
        }

        case KeyAction::COPY: copy(); break;

        case KeyAction::PASTE: 
        {
            std::string pasted = paste();
            refreshCurrentDirectory();
            if (!pasted.empty()) update_position(pasted);
            break;
        }

        case KeyAction::DELETE: 
        {
            clearScreen();
            setCursorPos(1, 1);
            std::cout << "Are you sure you want to delete selected items? (y/n): " << std::flush;
            char confirm = getchar();
            if (confirm == 'y' || confirm == 'Y'){
                deleteSelectedItems();
                refreshCurrentDirectory();
            }
            break;
        }

        case KeyAction::COMMAND: renderUI(); commandMode(); break;

        case KeyAction::TOGGLE_SELECT: toggleSelect(); break;

        case KeyAction::CLEAR_SELECTION: app.selection.selectedFiles.clear(); break;

        case KeyAction::BACK: goBack(); break;

        case KeyAction::ENTER: handleEnter(); break;

        default: break; // UNKNOWN keys ignored
    }
}

void navigate() {

    app.nav.currPath = app.nav.root;

    openCurrDirectory(app.nav.currPath.c_str());

    scrollToIndex((int)app.nav.fileList.size() - 1, (int)app.nav.fileList.size(), app.layout.rowSize, app.nav.up_screen, app.nav.down_screen, app.nav.xcurr);

    renderUI();
    setDefaultCursorPos();


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
            ch = std::cin.get();

            processKey(ch);

            renderUI();
            setDefaultCursorPos();
        }
    }
}