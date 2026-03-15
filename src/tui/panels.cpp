#include "myheader.h"

void renderMiddlePanel()
{
    openCurrDirectory(app.nav.currPath.c_str());

    for (int i = app.nav.up_screen, line = 1;
         i < app.nav.up_screen + app.layout.rowSize &&
         i < (int)app.nav.fileList.size();
         i++, line++)
    {
        posx(line, app.layout.colSize + 2);
        display(app.nav.fileList[i].c_str(), app.nav.currPath.c_str());
    }
}

void renderRightPanel() {
    string selectedFile = app.nav.fileList[app.nav.xcurr + app.nav.up_screen - 1];
    string newPath = app.nav.currPath + "/" + selectedFile;

    struct stat st;
    if (lstat(newPath.c_str(), &st) != 0) return;

    bool isDir = S_ISDIR(st.st_mode);
    bool isReadable = (st.st_mode & S_IRUSR);

    if (!isDir) {
        renderFilePreview(newPath, isReadable);
    } else {
        renderDirectoryPreview(newPath);
    }
}

void renderFilePreview(const string &newPath, bool isReadable) {
    if (isReadable)
    {
        if (!isBinaryFile(newPath))
        {
            displayTextFile(newPath);
        }
        else {
            posx(1, 2*app.layout.colSize);
            printf("\033[1;31mBinary File\033[0m\n");
        }
    }
    else {
        posx(1, 2*app.layout.colSize);
        printf("\033[1;31mNo Read Permission\033[0m\n");
    }
}

void renderDirectoryPreview(const string &newPath) {
    openDirectory(newPath.c_str(), app.layout.for_up_screen, app.layout.for_down_screen); 

    if(app.nav.fileList.size() == 0){
        posx(1, 2*app.layout.colSize);
        printf("\033[1;31mNo Items\033[0m\n");
    }
    else{
        for (int i = app.layout.for_up_screen, line = 1;
             i < app.layout.for_up_screen + app.layout.rowSize &&
             i < (int)app.nav.fileList.size();
             i++, line++)
        {
            posx(line, 2*app.layout.colSize);
            display(app.nav.fileList[i].c_str(), newPath);
        }
    }
}

void refreshCurrentDirectory()
{
    invalidateDirCache(app.nav.currPath);
    openCurrDirectory(app.nav.currPath.c_str());

    /* Fix cursor overflow */
    if (app.nav.xcurr > (int)app.nav.fileList.size()) {
        app.nav.xcurr = app.nav.fileList.size();
    }

    if (app.nav.xcurr <= 0) {
        app.nav.xcurr = 1;
    }

    /* Fix scrolling bounds */
    if (app.nav.up_screen > (int)app.nav.fileList.size()) {
        app.nav.up_screen = 0;
    }

    app.nav.down_screen =
        (int)app.nav.fileList.size() -
        app.nav.up_screen -
        app.layout.rowSize;

    if (app.nav.down_screen < 0)
        app.nav.down_screen = 0;
}