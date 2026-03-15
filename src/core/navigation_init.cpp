#include "myheader.h"

void initializeNavigation(int argc, char *argv[]) {

    // ROOT SET
    if(argc==1){
        char *currentDir = getcwd(NULL, 0);
        app.nav.root = currentDir;
        free(currentDir);
    }
    else{
        app.nav.root = argv[1];
    }

    string absPath = app.nav.root;

    vector<string> pathParts;
    stringstream ss(absPath);
    string segment;

    while (getline(ss, segment, '/')) {
        if (!segment.empty()) {
            pathParts.push_back(segment);
        }
    }

    stack<NavState> tempStack, dummyStack;

    string newPath = "";

    for (size_t ind=0; ind<pathParts.size(); ind++) {
        const auto dir = pathParts[ind];
        newPath = newPath + "/" + dir;

        vector<string> currFileList;
        DIR *d = opendir(newPath.c_str());
        if (d) {
            struct dirent *entry;
            while ((entry = readdir(d)) != nullptr) {
                string name = entry->d_name;
                if (name == "." || name == "..") continue;
                currFileList.push_back(name);
            }
            closedir(d);
            sort(currFileList.begin(), currFileList.end());
        }

        int dirIndex = 0;
        if(ind<pathParts.size()-1){
            for (size_t i = 0; i < currFileList.size(); i++) {
                if (currFileList[i] == pathParts[ind+1]) {
                    dirIndex = i;
                    break;
                }
            }
        }

        if ((int)dirIndex < app.layout.rowSize)
        {
            app.nav.up_screen = 0;
            app.nav.xcurr = dirIndex + 1;
        }
        else
        {
            app.nav.up_screen = dirIndex - app.layout.rowSize+1;
            app.nav.xcurr = app.layout.rowSize;
        }

        NavState currState;
        currState.path = newPath;
        currState.xcurr = app.nav.xcurr;
        currState.up_screen = app.nav.up_screen;

        tempStack.push(currState);
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

    app.nav.backStack.pop();
    app.nav.currPath = app.nav.root;
}