#include "myheader.h"

void renderMiddlePanel()
{
    openCurrDirectory(app.nav.currPath.c_str());

    for (int i = app.nav.up_screen, line = 1;
         i < app.nav.up_screen + app.layout.rowSize &&
         i < (int)app.nav.fileList.size();
         i++, line++)
    {
        setCursorPos(line, app.layout.colSize + 2);
        display(app.nav.fileList[i].c_str(), app.nav.currPath.c_str());
    }
}

void renderRightPanel() {
    if (app.nav.fileList.empty())
        return;

    // Get selected file path
    std::string selectedFile = app.nav.fileList[app.nav.xcurr + app.nav.up_screen - 1];
    fs::path newPath = fs::path(app.nav.currPath) / selectedFile;

    bool readable = isReadable(newPath);

    if (isDirectory(newPath)) {
        renderDirectoryPreview(newPath.string());
    } else if (isRegularFile(newPath)) {
        renderFilePreview(newPath.string(), readable);
    }

    /* -------- FILE DETAILS -------- */
    setCursorPos(1, 1);
    getFileDetails(newPath.string());
    print_details();
}

void renderFilePreview(const std::string &newPath, bool isReadable) {
    if (isReadable)
    {
        if (!isBinaryFile(newPath))
        {
            displayTextFile(newPath);
        }
        else {
            setCursorPos(1, 2*app.layout.colSize);
            printf("\033[1;31mBinary File\033[0m\n");
        }
    }
    else {
        setCursorPos(1, 2*app.layout.colSize);
        printf("\033[1;31mNo Read Permission\033[0m\n");
    }
}

void renderDirectoryPreview(const std::string &newPath) {
    openDirectory(newPath.c_str(), app.layout.for_up_screen, app.layout.for_down_screen); 

    if(app.nav.fileList.size() == 0){
        setCursorPos(1, 2*app.layout.colSize);
        printf("\033[1;31mNo Items\033[0m\n");
    }
    else{
        for (int i = app.layout.for_up_screen, line = 1;
             i < app.layout.for_up_screen + app.layout.rowSize &&
             i < (int)app.nav.fileList.size();
             i++, line++)
        {
            setCursorPos(line, 2*app.layout.colSize);
            display(app.nav.fileList[i].c_str(), newPath);
        }
    }
}

void refreshCurrentDirectory()
{
    invalidateDirCache(app.nav.currPath);
    openCurrDirectory(app.nav.currPath.c_str());

    normalizeRange((int)app.nav.fileList.size(), app.layout.rowSize, app.nav.up_screen, app.nav.down_screen, app.nav.xcurr);
}