#include "myheader.h"

void print_details() {

    posx(1,1);

    std::string s = "";

    cout << truncateStr("File Name: " + app.fileDetails.fileName, app.fileDetails.colSize - 4) << endl;

    if (app.sizeState.inProgress){
        s = "File Size: Calculating";
        cout << truncateStr(s, app.fileDetails.colSize - 4) << endl;
    }
    else
        cout << truncateStr("File Size: " + humanReadableSize(app.sizeState.lastSize), app.fileDetails.colSize - 4) << endl;

    cout << truncateStr("Ownership: " + app.fileDetails.userName + " (User)", app.fileDetails.colSize - 4) << endl;
    cout << truncateStr(app.fileDetails.groupName + " (Group)", app.fileDetails.colSize - 4) << endl;
    cout << truncateStr("Permissions: " + app.fileDetails.permissions, app.fileDetails.colSize - 4) << endl;
    cout << truncateStr("Last Modified: " + string(app.fileDetails.timeBuffer), app.fileDetails.colSize - 4) << endl;

    if (app.fileDetails.lastScanDuration >= 0)
        cout << truncateStr("Scan Time: " + to_string(app.fileDetails.lastScanDuration) + " ms", app.fileDetails.colSize - 4) << endl;
    else{
        s = "Scan Time: Calculating";
        cout << truncateStr(s, app.fileDetails.colSize - 4) << endl;
    }
    pos();
}