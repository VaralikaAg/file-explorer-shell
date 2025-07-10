#include "myheader.h"

string clipboard;
#define pos() fprintf(stdout, "\033[%d;%dH", xcurr, ycurr)  // Move cursor
#define posx(x, y) fprintf(stdout, "\033[%d;%dH", x, y)  // Move to (x, y)

string get_input(){
    string commandLine;
    char ch;
    while (true) {
        ch = std::cin.get();  // Read one character at a time

        // Handle backspace (delete the last character)
        if (ch == '\b' || ch == 127) {  // '\b' is backspace, 127 is ASCII DEL
            if (!commandLine.empty()) {
                std::cout << "\b \b";  // Move the cursor back, overwrite with space, and move back again
                commandLine.pop_back();  // Remove the last character from the string
            }
        } else if (ch == '\n') {
            break;  // Exit loop when user presses "Enter"
        } else {
            commandLine.push_back(ch);  // Append other characters to the string
            cout << ch;  // Echo the character to the screen
        }
    }
    return commandLine;
}

void copy(string selectedFile){
    string sourcePath = string(currPath) + "/" + selectedFile;
    clipboard = sourcePath;
    posx(rows-2, 0);
    printf("\033[K");
    printf("\033[1;33mCOPIED: %s \033[0m", sourcePath.c_str());
    pos();
}

void paste(){
    if(!clipboard.empty()){
        size_t posIdx = clipboard.find_last_of("/\\");
        string baseName = (posIdx != string::npos) ? clipboard.substr(posIdx + 1) : clipboard;
        
        string destPath = string(currPath) + "/" + baseName;
        string cmd;
        if (isDirectory(clipboard.c_str())) {
            cmd = "cp -r \"" + clipboard + "\" \"" + destPath + "\"";
        } else {
            cmd = "cp \"" + clipboard + "\" \"" + destPath + "\"";
        }
        system(cmd.c_str());
    }
}

void deleteItem(string selectedFile) {
    string targetPath = string(currPath) + "/" + selectedFile;

    // Clear screen and ask for confirmation
    system("clear");
    posx(1,1);
    cout<<"Are you sure you want to delete '"+(string)selectedFile.c_str()+"' ? (y/n): ";
    // printf("\033[1;33mAre you sure you want to delete '%s'? (y/n): \033[0m", selectedFile.c_str());

    string choice=get_input();
    // getline(cin, choice);

    if (choice == "y" || choice == "Y") {
        string cmd;
        if (isDirectory(targetPath.c_str())) {
            cmd = "rm -r \"" + targetPath + "\"";  // Delete directory recursively
        } else {
            cmd = "rm \"" + targetPath + "\"";  // Delete file
        }
        system(cmd.c_str());
    }
}

void renameItem(string selectedFile) {
    string oldPath = string(currPath) + "/" + selectedFile;

    // Clear screen and ask for new name
    posx(rows-2,0);
    printf("\033[K");
    // system("clear");
    printf("\033[1;33mEnter new name for '%s': \033[0m", selectedFile.c_str());

    string newName=get_input();
    // getline(cin >> ws, newName);  // Read the full input while ignoring leading spaces

    if (newName.empty()) {
        return;  // Exit if the input is empty
    }

    string newPath = string(currPath) + "/" + newName;

    if (rename(oldPath.c_str(), newPath.c_str()) == 0) {
        return;
    }
}

void createFile() {
    posx(rows-2, 0);
    printf("\033[K");
    printf("\033[1;33mEnter file name: \033[0m");

    string fileName=get_input();
    // getline(cin >> ws, fileName);  // Read the full input while ignoring leading spaces

    if (fileName.empty()) {
        return;  // Exit if the input is empty
    }

    string filePath = string(currPath) + "/" + fileName;
    ofstream file(filePath);
    if (file) {
        file.close();
    }
}

void createDirectory() {
    posx(rows-2, 0);
    printf("\033[K");
    printf("\033[1;33mEnter directory name: \033[0m");

    string dirName=get_input();
    // getline(cin >> ws, dirName);  // Read the full input while ignoring leading spaces

    if (dirName.empty()) {
        return;  // Exit if the input is empty
    }

    string dirPath = string(currPath) + "/" + dirName;
    if (mkdir(dirPath.c_str(), 0777) == 0) {
        return;
    }
}

void navigateToAbsolutePath(string absPath) {
    // posx(10, 0);
    // printf("\033[K");
    // printf("\033[1;33mEnter absolute path: \033[0m");

    // string absPath;
    // getline(cin >> ws, absPath);
    if (absPath.empty() || absPath[0] != '/') {
        // If path is empty or not absolute, do nothing
        // openDirectory(currPath, up_screen, down_screen);
        openCurrDirectory(currPath);
        displayFiles();
        pos();
        return;
    }

    // Split the path into directories
    vector<string> pathParts;
    stringstream ss(absPath);
    string segment;
    
    while (getline(ss, segment, '/')) {
        if (!segment.empty()) {
            pathParts.push_back(segment);
        }
    }

    // Start from root
    // string newPath = "/";
    // string newPath = string(root);
    string newPath = "/home";

    stack<NavState> tempStack, dummyStack;


    for (const auto& dir : pathParts) {
        getDirectoryCount(newPath.c_str()); // Get directory contents
        bool found = false;
        int i=0;

        // Check if directory exists in fileList
        for (const auto& item : fileList) {
            i++;
            if (item == dir) {
                // auto it = find(fileList.begin(), fileList.end(), item);
                // int index = distance(fileList.begin(), it)+1;
                int index=i;
                // logMessage("index: "+to_string(index));
                // logMessage("item name: "+item);
                NavState currState;
                currState.path = newPath;
                if (fileList.size() <= rowSize) {  // Edge case: fewer files than rowSize
                    // logMessage("hey 1");
                    currState.up_screen = 0;
                    currState.xcurr = index; // Position at the exact index
                } 
                else if(fileList.size()-index<rowSize){
                    // logMessage("hey 2");
                    currState.up_screen = max(0, (int)(fileList.size() - rowSize));
                    currState.xcurr = rowSize - ((int)fileList.size() - index);
                }
                else{
                    // logMessage("hey 3");
                    currState.xcurr = 1;
                    currState.up_screen = index-1;
                    
                    // string msg = to_string(up_screen) + " " + to_string(down_screen);
                    // logMessage(msg);
                }
                // currState.xcurr = 1;
                // currState.up_screen = fileList.size() > rowSize ? fileList.size() - rowSize : 0;
                // currState.up_screen = 0;

                tempStack.push(currState);
                // logMessage(currState.path);
                // logMessage(to_string(currState.xcurr));
                // logMessage(to_string(currState.up_screen));

                found = true;
                newPath = newPath + "/" + dir ;

                break;
            }
        }

        if (!found) {
            // If any directory is missing, revert to current directory
            while (!tempStack.empty()) tempStack.pop(); // Clear stack
            // openDirectory(currPath, up_screen, down_screen);
            openCurrDirectory(currPath);
            displayFiles();
            pos();
            return;
        }
    }

    // Clear backStack and push visited directories
    while (!backStack.empty()) backStack.pop();
    while (!tempStack.empty()) {
        dummyStack.push(tempStack.top());
        tempStack.pop();
    }
    while (!dummyStack.empty()) {
        backStack.push(dummyStack.top());
        dummyStack.pop();
    }


    // Update current path and open the final directory
    delete[] currPath;  
    currPath = new char[newPath.length() + 1];
    strcpy(currPath, newPath.c_str());

    // logMessage(currPath);
	// openDirectory(currPath, up_screen, down_screen);
	openCurrDirectory(currPath);
    up_screen=0;
    down_screen = fileList.size() - rowSize;
    xcurr=1;
    displayFiles();
    // xcurr=1;
    pos();
}