#include "myheader.h"

void displayFoundFiles(int file_up) {
    clearScreen();

    for (int i = file_up, line = 1; i < (int)app.search.foundPaths.size() && i < file_up + app.layout.rowSize; i++, line++)
    {
        std::string filePath = app.search.foundPaths[i];

        const size_t availableWidth = app.ui.cols - 3;

        if (filePath.length() > availableWidth) {
            filePath = "..." + filePath.substr(
                filePath.length() - (availableWidth - 3)
            );
        }

        setCursorPos(static_cast<int>(line), 3);
        printf("%s\n", filePath.c_str());
    }
}

void jumpToSearchResult(const std::string &selectedPath){
    while (!app.nav.backStack.empty())
    {
        app.nav.backStack.pop();
    }

    std::stringstream ss(selectedPath);
    std::string token;
    std::vector<std::string> pathSegments;

    while (getline(ss, token, '/'))
    {
        pathSegments.push_back(token);
    }

    std::string rebuiltPath = "";
    for (size_t i = 1; i < pathSegments.size() - 1; i++)
    {
        rebuiltPath += "/";
        rebuiltPath += pathSegments[i];

        NavState rePath;
        rePath.path = rebuiltPath;

        getDirectoryCount(rebuiltPath.c_str());

        auto it = find(app.nav.fileList.begin(), app.nav.fileList.end(), pathSegments[i + 1]);

        if (it != app.nav.fileList.end())
        {
            int dis = distance(app.nav.fileList.begin(), it) + 1;

            int dummy_down = 0;
            scrollToIndex(dis - 1, (int)app.nav.fileList.size(), app.layout.rowSize, rePath.up_screen, dummy_down, rePath.xcurr);
        }

        app.nav.backStack.push(rePath);
    }

    if (!app.nav.backStack.empty())
    {
        app.nav.currPath = app.nav.backStack.top().path;
    }

    std::string fileName = pathSegments.back();

    openCurrDirectory(app.nav.currPath.c_str());
    app.nav.backStack.pop();

    auto it = find(app.nav.fileList.begin(), app.nav.fileList.end(), fileName);

    if (it != app.nav.fileList.end())
    {
        int dis = distance(app.nav.fileList.begin(), it) + 1;

        scrollToIndex(dis - 1, (int)app.nav.fileList.size(), app.layout.rowSize, app.nav.up_screen, app.nav.down_screen, app.nav.xcurr);
    }
}

void displaySearchResults(){
    if ((int)app.search.foundPaths.size() == 0)
        return;

    char ch;

    int file_up = 0, file_down = 0, filecurr = 1;
    normalizeRange((int)app.search.foundPaths.size(), app.layout.rowSize, file_up, file_down, filecurr);
    displayFoundFiles(file_up);
    setCursorPos(1,1);
    while (true) {
        ch = std::cin.get();

        if (ch == 27) {  // Escape sequence
            clearScreen();
            displayFoundFiles(file_up);
            setCursorPos(filecurr, 1);
            ch = std::cin.get();
            if (ch == '\n' || ch == EOF) {
                renderUI();
                break;
            }
            ch = std::cin.get();

            if (ch == 'A') {  // Up arrow key
                if (filecurr > 1) {
                    filecurr--;
                } else if (file_up > 0) {
                    file_up--;
                }
            } else if (ch == 'B') { // Down arrow key
                if (filecurr < app.layout.rowSize && filecurr < (int)app.search.foundPaths.size()) {
                    filecurr++;
                } else if (file_down > 0) {
                    file_up++;
                }
            }
            normalizeRange((int)app.search.foundPaths.size(), app.layout.rowSize, file_up, file_down, filecurr);
            displayFoundFiles(file_up);
            setCursorPos(filecurr, 1);
        }
        else if (ch == '\n' || ch == '\r'){
            std::string selectedPath  = app.search.foundPaths[filecurr + file_up - 1];
            jumpToSearchResult(selectedPath);
            renderUI();
            return;
        }
        else if(ch=='q'){
            renderUI();
            return;
        }

    }

}
