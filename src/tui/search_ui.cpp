#include "myheader.h"

void displayFoundFiles(int file_up) {
    clearScreen;

    for (int i = file_up, line = 1; i < (int)app.search.foundPaths.size() && i < file_up + app.layout.rowSize; i++, line++)
    {
        string filePath = app.search.foundPaths[i];

        const size_t availableWidth = app.ui.cols - 3;

        if (filePath.length() > availableWidth) {
            filePath = "..." + filePath.substr(
                filePath.length() - (availableWidth - 3)
            );
        }

        posx(static_cast<int>(line), 3);
        printf("%s\n", filePath.c_str());
    }
}

void jumpToSearchResult(const string &selectedPath){
    while (!app.nav.backStack.empty())
    {
        app.nav.backStack.pop();
    }

    stringstream ss(selectedPath);
    string token;
    vector<string> pathSegments;

    while (getline(ss, token, '/'))
    {
        pathSegments.push_back(token);
    }

    string rebuiltPath = "";
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

            if ((int)app.nav.fileList.size() <= app.layout.rowSize)
            {
                rePath.up_screen = 0;
                rePath.xcurr = dis;
            }
            else if ((int)app.nav.fileList.size() - dis < app.layout.rowSize)
            {
                rePath.up_screen = max(0, (int)(app.nav.fileList.size() - app.layout.rowSize));
                rePath.xcurr = app.layout.rowSize - ((int)app.nav.fileList.size() - dis);
            }
            else
            {
                rePath.xcurr = 1;
                rePath.up_screen = dis - 1;
            }
        }

        app.nav.backStack.push(rePath);
    }

    if (!app.nav.backStack.empty())
    {
        app.nav.currPath = app.nav.backStack.top().path;
    }

    string fileName = pathSegments.back();

    openCurrDirectory(app.nav.currPath.c_str());
    app.nav.backStack.pop();

    auto it = find(app.nav.fileList.begin(), app.nav.fileList.end(), fileName);

    if (it != app.nav.fileList.end())
    {
        int dis = distance(app.nav.fileList.begin(), it) + 1;

        if ((int)app.nav.fileList.size() <= app.layout.rowSize)
        {
            app.nav.up_screen = 0;
            app.nav.xcurr = dis;
        }
        else if ((int)app.nav.fileList.size() - dis < app.layout.rowSize)
        {
            app.nav.up_screen = max(0, (int)(app.nav.fileList.size() - app.layout.rowSize));
            app.nav.xcurr = app.layout.rowSize - ((int)app.nav.fileList.size() - dis);
        }
        else
        {
            app.nav.xcurr = 1;
            app.nav.up_screen = dis - 1;
        }

        app.nav.down_screen = (int)app.nav.fileList.size() - app.nav.up_screen - app.layout.rowSize;
    }
}

void displaySearchResults(){
    if ((int)app.search.foundPaths.size() == 0)
        return;

    char ch;

    int file_up=0, file_down=0, filecurr=1;
    file_down = (int)app.search.foundPaths.size() - file_up - app.layout.rowSize;
    displayFoundFiles(file_up);
    posx(1,1);
    while (true) {
        ch = cin.get();

        if (ch == 27) {  // Escape sequence
            clearScreen;
            displayFoundFiles(file_up);
            posx(filecurr, 1);
            ch = cin.get();
            if (ch == '\n' || ch == EOF) {
                renderUI();
                break;
            }
            ch = cin.get();

            if (ch == 'A') {  // Up arrow key
                if(filecurr>1){
                    filecurr--;
                    displayFoundFiles(file_up);
                    posx(filecurr, 1);
                }
                else if (filecurr==1 && file_up > 0) {
                    file_up--;
                    file_down++;
                    displayFoundFiles(file_up);
                }
                else{
                    displayFoundFiles(file_up);
                    posx(filecurr, 1);
                }
            }

            else if(ch=='B'){
                openCurrDirectory(app.nav.currPath.c_str());
                if (filecurr < app.layout.rowSize && filecurr < (int)app.search.foundPaths.size())
                {
                    filecurr++;
                    displayFoundFiles(file_up);
                    posx(filecurr, 1);
                }
                else if(filecurr==app.layout.rowSize && file_down>0){
                    file_up++;
                    file_down--;
                    displayFoundFiles(file_up);
                    posx(app.layout.rowSize, 1);
                }
                else{
                    displayFoundFiles(file_up);
                    posx(min((int)(app.search.foundPaths.size()), (int)app.layout.rowSize), 1);
                }
            }
        }
        else if (ch == '\n' || ch == '\r'){
            string selectedPath  = app.search.foundPaths[filecurr + file_up - 1];
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
