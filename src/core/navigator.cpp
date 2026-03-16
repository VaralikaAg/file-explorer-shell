#include "myheader.h"

void normalizeRange(int total, int rowSize, int &up, int &down, int &x)
{
    if (total == 0)
    {
        up = 0;
        down = 0;
        x = 1;
        return;
    }

    int maxUp = (total > rowSize) ? total - rowSize : 0;
    if (up < 0)
        up = 0;
    if (up > maxUp)
        up = maxUp;

    int visible = total - up;
    int maxX = std::min(rowSize, visible);
    if (x < 1)
        x = 1;
    if (x > maxX)
        x = maxX;

    down = std::max(0, total - up - rowSize);
}

void scrollToIndex(int index, int total, int rowSize, int &up, int &down, int &x)
{
    if (index < 0 || total == 0)
        return;

    if (index >= total)
        index = total - 1;

    if (index < rowSize)
    {
        up = 0;
        x = index + 1;
    }
    else
    {
        up = index - rowSize + 1;
        x = rowSize;
    }
    down = std::max(0, total - up - rowSize);
}

void openDirectory(const char *path, int &up, int &down)
{
    app.layout.totalFiles = getDirectoryCount(path);
    int dummy_x = 1;

    normalizeRange((int)app.nav.fileList.size(), app.layout.rowSize, up, down, dummy_x);
}

void openCurrDirectory(const char *path)
{
    if ((int)app.cache.dirCache.size() > app.cache.max_cache_entries)
    {
        app.cache.dirCache.erase(app.cache.dirCache.begin());
    }

    app.layout.totalFiles = getDirectoryCount(path);

    if(app.layout.totalFiles == 0) {
        return;
    }
}

void normalizeCursor()
{
    normalizeRange((int)app.nav.fileList.size(), app.layout.rowSize, app.nav.up_screen, app.nav.down_screen, app.nav.xcurr);
}

void update_position(const std::string &fileName)
{
    int idx = -1;
    for (size_t i = 0; i < app.nav.fileList.size(); i++)
    {
        if (app.nav.fileList[i] == fileName)
        {
            idx = i;
            break;
        }
    }
    if (idx != -1)
    {
        scrollToIndex(idx, (int)app.nav.fileList.size(), app.layout.rowSize, app.nav.up_screen, app.nav.down_screen, app.nav.xcurr);
    }
}

void toggleSelect()
{
    std::string file = app.nav.fileList[app.nav.xcurr + app.nav.up_screen - 1];
    std::string fullPath = app.nav.currPath + "/" + file;

    if (app.selection.selectedFiles.count(fullPath))
        app.selection.selectedFiles.erase(fullPath);
    else
        app.selection.selectedFiles.insert(fullPath);
}

bool isUnderCurrentDir(const std::string &path)
{
    std::string base(app.nav.currPath);
    if (base.back() != '/')
        base += '/';
    return path.rfind(base, 0) == 0;
}

void navigateToAbsolutePath(const std::string &absPath)
{

    if (absPath.empty() || absPath[0] != '/') {
        openCurrDirectory(app.nav.currPath.c_str());
        renderUI();
        setDefaultCursorPos();
        return;
    }

    std::vector<std::string> pathParts;
    std::stringstream ss(absPath);
    std::string segment;
    
    while (getline(ss, segment, '/')) {
        if (!segment.empty()) {
            pathParts.push_back(segment);
        }
    }

    std::string newPath = "/";
    std::stack<NavState> tempStack, dummyStack;

    for (const auto& dir : pathParts) {
        getDirectoryCount(newPath.c_str()); // Get directory contents
        bool found = false;
        int i=0;

        for (const auto &item : app.nav.fileList)
        {
            i++;
            if (item == dir) {
                int index=i;
                NavState currState;
                currState.path = newPath;
                int dummy_down = 0;
                scrollToIndex(index - 1, (int)app.nav.fileList.size(), app.layout.rowSize, currState.up_screen, dummy_down, currState.xcurr);

                tempStack.push(currState);

                found = true;
                newPath = newPath + "/" + dir ;

                break;
            }
        }

        if (!found) {
            while (!tempStack.empty()) tempStack.pop(); // Clear stack
            openCurrDirectory(app.nav.currPath.c_str());
            renderUI();
            setDefaultCursorPos();
            return;
        }
    }

    while (!app.nav.backStack.empty())
        app.nav.backStack.pop();
    while (!tempStack.empty()) {
        dummyStack.push(tempStack.top());
        tempStack.pop();
    }
    while (!dummyStack.empty()) {
        app.nav.backStack.push(dummyStack.top());
        dummyStack.pop();
    }

    app.nav.currPath = newPath;

    openCurrDirectory(app.nav.currPath.c_str());
    app.nav.up_screen = 0;
    app.nav.down_screen = (int)app.nav.fileList.size() - app.layout.rowSize;
    app.nav.xcurr = 1;
    renderUI();
    setDefaultCursorPos();
}
