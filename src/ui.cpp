#include "myheader.h"

#define clearScreen fputs("\033[H\033[2J", stdout)
#define pos() fprintf(stdout, "\033[%d;%dH", xcurr, ycurr)  // Move cursor
#define posx(x, y) fprintf(stdout, "\033[%d;%dH", x, y)  // Move to (x, y)
#define setCursorRed() fprintf(stdout, "\033]12;#ff0000\007")
#define resetCursorColor() fprintf(stdout, "\033]12;?\007")

bool inputAvailable() {
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(STDIN_FILENO, &fds);

    timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 0; // no blocking

    return select(STDIN_FILENO + 1, &fds, nullptr, nullptr, &tv) > 0;
}

void renderUI() {
    // system("clear");
    clearScreen;
    setCursorRed();

    openCurrDirectory(currPath);
    if(fileList.empty()) {
        // Do nothing, directory is empty
        return;
    }
    // logMessage(currPath);
    for (unsigned int i = up_screen, line=1; i < up_screen + rowSize && i < fileList.size(); i++, line++) {
        posx(line, colSize+2);
        display(fileList[i].c_str(), currPath);
    }
    // if(xcurr>fileList.size()) xcurr=fileList.size();

    string selectedFile = fileList[xcurr + up_screen - 1];
    string tempPath = string(currPath) + "/" + selectedFile;
    char *newPath = new char[tempPath.length() + 1]; 
    strcpy(newPath, tempPath.c_str());

    // posx(1,1);
    // getFileDetails(newPath);

    if(!isDirectory(newPath)){
        // posx(1, 45);
        // printf("\033[1;31mNo Items\033[0m\n");
        if (isReadableFile(newPath)) {
             if (!isBinaryFile(newPath)) {
                displayTextFile(newPath);
            }
            // previewFile(newPath);
            else {
                posx(1, 2*colSize);
                printf("\033[1;31mBinary File\033[0m\n");
            }
        }
        else {
            posx(1, 2*colSize);
            printf("\033[1;31mNo Read Permission\033[0m\n");
        }
    }
    else{
        openDirectory(newPath, for_up_screen, for_down_screen); 
        if(fileList.size()==0){
            posx(1,2*colSize);
            printf("\033[1;31mNo Items\033[0m\n");
        }
        else{
            for (unsigned int i = for_up_screen, line=1; i < for_up_screen + rowSize && i < fileList.size(); i++, line++) {
                posx(line, 2*colSize);
                display(fileList[i].c_str(), newPath);
            } 
        }
    }
    posx(rows-2, 0);
    printf("\033[1;34mCurrent Path: %s\033[0m\n", currPath);
    openCurrDirectory(currPath);

    posx(1,1);
    print_details();

    ycurr=colSize;
    pos();
    // logMessage(to_string(ycurr));
}
