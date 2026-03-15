#include "myheader.h"

void openDirectory(const char *path, int &up, int &down)
{
    app.layout.totalFiles = getDirectoryCount(path);
    if(app.layout.totalFiles == 0) {
        return;
    }

    up = (int)app.nav.fileList.size() > app.layout.rowSize 
        ? (int)app.nav.fileList.size() - app.layout.rowSize 
        : 0;

    down = max(0,(int)(app.nav.fileList.size() - up - app.layout.rowSize));
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
    int total = app.nav.fileList.size();

    if (total == 0)
    {
        app.nav.xcurr = 1;
        app.nav.up_screen = 0;
        app.nav.down_screen = 0;
        return;
    }

    int maxUp = (total > (int)app.layout.rowSize)
                    ? total - app.layout.rowSize
                    : 0;

    if ((int)app.nav.up_screen < 0)
        app.nav.up_screen = 0;
    if ((int)app.nav.up_screen > maxUp)
        app.nav.up_screen = maxUp;

    int visible = total - app.nav.up_screen;
    int maxX = min((int)app.layout.rowSize, visible);

    if ((int)app.nav.xcurr < 1)
        app.nav.xcurr = 1;
    if ((int)app.nav.xcurr > maxX)
        app.nav.xcurr = maxX;

    app.nav.down_screen = total - app.nav.up_screen - app.layout.rowSize;
    if (app.nav.down_screen < 0)
        app.nav.down_screen = 0;
}

void update_position(const string &fileName)
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
        // Calculate cursor & scrolling
        if ((int)idx < app.layout.rowSize)
        {
            app.nav.up_screen = 0;
            app.nav.xcurr = idx + 1;
        }
        else
        {
            app.nav.up_screen = idx - app.layout.rowSize + 1;
            app.nav.xcurr = app.layout.rowSize;
        }

        app.nav.down_screen = app.nav.fileList.size() - app.nav.up_screen - app.layout.rowSize;
        if (app.nav.down_screen < 0)
            app.nav.down_screen = 0;
    }
}

void toggleSelect()
{
    string file = app.nav.fileList[app.nav.xcurr + app.nav.up_screen - 1];
    string fullPath = app.nav.currPath + "/" + file;

    if (app.selection.selectedFiles.count(fullPath))
        app.selection.selectedFiles.erase(fullPath);
    else
        app.selection.selectedFiles.insert(fullPath);
}

bool isUnderCurrentDir(const string &path)
{
    string base(app.nav.currPath);
    if (base.back() != '/')
        base += '/';
    return path.rfind(base, 0) == 0;
}

void navigateToAbsolutePath(const string &absPath)
{

    if (absPath.empty() || absPath[0] != '/') {
        openCurrDirectory(app.nav.currPath.c_str());
        renderUI();
        pos();
        return;
    }

    vector<string> pathParts;
    stringstream ss(absPath);
    string segment;
    
    while (getline(ss, segment, '/')) {
        if (!segment.empty()) {
            pathParts.push_back(segment);
        }
    }

    string newPath = "/";
    stack<NavState> tempStack, dummyStack;

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
                if ((int)app.nav.fileList.size() <= app.layout.rowSize)
                { 
                    currState.up_screen = 0;
                    currState.xcurr = index; // Position at the exact index
                }
                else if ((int)app.nav.fileList.size() - index < app.layout.rowSize)
                {
                    // logMessage("hey 2");
                    currState.up_screen = max(0, (int)(app.nav.fileList.size() - app.layout.rowSize));
                    currState.xcurr = app.layout.rowSize - ((int)app.nav.fileList.size() - index);
                }
                else{
                    // logMessage("hey 3");
                    currState.xcurr = 1;
                    currState.up_screen = index-1;
                }

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
            pos();
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
    pos();
}
