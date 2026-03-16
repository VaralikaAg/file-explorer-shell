#include "myheader.h"

void print_details() {

    setCursorPos(1,1);

    std::string s = "";

    std::cout << truncateStr("File Name: " + app.fileDetails.fileName, app.fileDetails.colSize - 4) << std::endl;

    if (app.sizeState.inProgress){
        s = "File Size: Calculating";
        std::cout << truncateStr(s, app.fileDetails.colSize - 4) << std::endl;
    }
    else
        std::cout << truncateStr("File Size: " + humanReadableSize(app.sizeState.lastSize), app.fileDetails.colSize - 4) << std::endl;

    std::cout << truncateStr("Ownership: " + app.fileDetails.userName + " (User)", app.fileDetails.colSize - 4) << std::endl;
    std::cout << truncateStr(app.fileDetails.groupName + " (Group)", app.fileDetails.colSize - 4) << std::endl;
    std::cout << truncateStr("Permissions: " + app.fileDetails.permissions, app.fileDetails.colSize - 4) << std::endl;
    std::cout << truncateStr("Last Modified: " + std::string(app.fileDetails.timeBuffer), app.fileDetails.colSize - 4) << std::endl;

    if (app.fileDetails.lastScanDuration >= 0)
        std::cout << truncateStr("Scan Time: " + std::to_string(app.fileDetails.lastScanDuration) + " ms", app.fileDetails.colSize - 4) << std::endl;
    else{
        s = "Scan Time: Calculating";
        std::cout << truncateStr(s, app.fileDetails.colSize - 4) << std::endl;
    }
    setDefaultCursorPos();
}