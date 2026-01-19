#include "myheader.h"

vector<string> clipboard;
#define pos() fprintf(stdout, "\033[%d;%dH", xcurr, ycurr)  // Move cursor
#define posx(x, y) fprintf(stdout, "\033[%d;%dH", x, y)  // Move to (x, y)
#define clearScreen fputs("\033[H\033[2J", stdout)
#define setCursorRed() fprintf(stdout, "\033]12;#ff0000\007")

string get_input() {
    string commandLine;
    char ch;

    while (true) {
        ch = getchar();   // use getchar(), not cin

        /* ENTER */
        if (ch == '\n' || ch == '\r') {
            break;
        }

        /* BACKSPACE / DELETE */
        if (ch == 127 || ch == '\b') {
            if (!commandLine.empty()) {
                printf("\b \b");
                commandLine.pop_back();
            }
            continue;
        }

        /* ESC SEQUENCE (arrow keys, home, end, etc.) */
        if (ch == 27) {   // ESC
            getchar();    // skip '['
            getchar();    // skip final char (A/B/C/D)
            continue;     // IGNORE completely
        }

        /* Printable characters only */
        if (isprint(ch)) {
            commandLine.push_back(ch);
            putchar(ch);
        }
    }

    return commandLine;
}

void copy() {
    try {
        vector<string> tempClipboard;   // transactional buffer

        // -------- BEGIN TRANSACTION --------
        if (!selectedFiles.empty()) {
            for (auto &path : selectedFiles) {
                if (path.empty()) {
                    throw runtime_error("Empty path in selectedFiles");
                }
                tempClipboard.push_back(path);
            }
        } else {
            int index = xcurr + up_screen - 1;

            if (index < 0 || index >= (int)fileList.size()) {
                throw out_of_range("Invalid cursor index");
            }

            string file = fileList[index];
            tempClipboard.push_back(string(currPath) + "/" + file);
        }

        // -------- COMMIT --------
        clipboard.clear();
        clipboard = std::move(tempClipboard);

        posx(rows - 2, 0);
        printf("\033[K");
        printf("\033[1;33mCOPIED %zu item(s)\033[0m", clipboard.size());
        pos();
    }
    catch (const exception &e) {
        // -------- ROLLBACK --------
        clipboard.clear();
        selectedFiles.clear();

        logMessage(
            string("[COPY FAILED] ") +
            e.what() +
            " | FILE: " + __FILE__ +
            " | LINE: " + to_string(__LINE__)
        );

        posx(rows - 2, 0);
        printf("\033[K");
        printf("\033[1;31mCOPY FAILED — STATE RECOVERED\033[0m");
        pos();
    }
}


void paste() {
    if (clipboard.empty()) return;

    vector<string> pastedFiles;   // tracks successfully pasted items
    string cursor_path = "";

    try {
        for (auto &src : clipboard) {
            size_t posIdx = src.find_last_of("/\\");
            string baseName = (posIdx != string::npos)
                                ? src.substr(posIdx + 1)
                                : src;

            cursor_path = baseName;

            string destPath = string(currPath) + "/" + baseName;

            string cmd;
            if (isDirectory(src.c_str())) {
                cmd = "cp -r \"" + src + "\" \"" + destPath + "\"";
            } else {
                cmd = "cp \"" + src + "\" \"" + destPath + "\"";
            }

            int ret = system(cmd.c_str());
            if (ret != 0) {
                throw runtime_error("Copy failed for: " + src);
            }

            // record ONLY after successful copy
            pastedFiles.push_back(destPath);
        }

        // -------- COMMIT --------
        if (CONFIG_INDEXING) {
            globalIndex.rectifyIndex(
                RectifyAction::COPY,
                {},
                pastedFiles
            );
        }
        pastedFiles.clear();          // nothing to rollback now
        invalidateDirCache(currPath);
        selectedFiles.clear();

        openDirectory(currPath, up_screen, down_screen);
        update_position(cursor_path);
        displayFiles();
        pos();
    }
    catch (const exception &e) {
        // -------- ROLLBACK --------
        for (auto &path : pastedFiles) {
            if (isDirectory(path.c_str())) {
                string cmd = "rm -rf \"" + path + "\"";
                system(cmd.c_str());
            } else {
                remove(path.c_str());
            }
        }

        logMessage(
            string("[PASTE FAILED] ") +
            e.what() +
            " | FILE: " + __FILE__ +
            " | LINE: " + to_string(__LINE__)
        );

        posx(rows - 2, 0);
        printf("\033[K");
        printf("\033[1;31mPASTE FAILED — ROLLED BACK\033[0m");
        pos();
    }
}

void deleteSelectedItems() {
    vector<string> targets;

    if (!selectedFiles.empty()) {
        for (auto &p : selectedFiles)
            targets.push_back(p);
    } else {
        string file = fileList[xcurr + up_screen - 1];
        targets.push_back(string(currPath) + "/" + file);
    }

    // ---- Confirmation ----
    system("clear");
    posx(1,1);
    cout << "Delete " << targets.size() << " item(s)? (y/n): ";

    string choice = get_input();
    if (choice != "y" && choice != "Y") {
        displayFiles();
        pos();
        return;
    }

    string binDir = string(currPath) + "/.trash_" + to_string(getpid());
    vector<pair<string, string>> movedItems;

    try {
        // ---- Create bin directory ----
        if (mkdir(binDir.c_str(), 0700) != 0) {
            throw runtime_error("Failed to create bin directory");
        }

        // ---- Move items to bin ----
        for (auto &path : targets) {
            size_t posIdx = path.find_last_of("/\\");
            string baseName = path.substr(posIdx + 1);

            string binPath = binDir + "/" + baseName;

            if (rename(path.c_str(), binPath.c_str()) != 0) {
                throw runtime_error("Failed to move: " + path);
            }

            movedItems.emplace_back(path, binPath);
        }

        // ---- Permanently delete bin ----
        string cmd = "rm -rf \"" + binDir + "\"";
        system(cmd.c_str());

        // ---- Commit ----
        if (CONFIG_INDEXING) {
            globalIndex.rectifyIndex(
                RectifyAction::DELETE,
                targets,
                {}
            );
        }

        selectedFiles.clear();
        invalidateDirCache(currPath);
        xcurr=1;
        up_screen=0;
        down_screen = fileList.size() - up_screen - rowSize;
        openDirectory(currPath, up_screen, down_screen);
        displayFiles();
        pos();
    }
    catch (const exception &e) {
        // ---- ROLLBACK ----
        for (auto &p : movedItems) {
            // move back: bin → original location
            rename(p.second.c_str(), p.first.c_str());
        }

        // cleanup bin if exists
        string cleanupCmd = "rm -rf \"" + binDir + "\"";
        system(cleanupCmd.c_str());

        logMessage(
            string("[DELETE FAILED] ") +
            e.what() +
            " | FILE: " + __FILE__ +
            " | LINE: " + to_string(__LINE__)
        );

        posx(rows - 2, 0);
        printf("\033[K");
        printf("\033[1;31mDELETE FAILED — RECOVERED\033[0m");
        pos();
    }
}

void renameItem(string selectedFile, string newName) {
    string oldPath = string(currPath) + "/" + selectedFile;

    // Clear screen and ask for new name
    // posx(rows-2,0);
    // printf("\033[K");
    // // system("clear");
    // printf("\033[1;33mEnter new name for '%s': \033[0m", selectedFile.c_str());

    // string newName=get_input();
    // getline(cin >> ws, newName);  // Read the full input while ignoring leading spaces

    if (newName.empty()) {
        return;  // Exit if the input is empty
    }

    string newPath = string(currPath) + "/" + newName;

    if (rename(oldPath.c_str(), newPath.c_str()) == 0) {
        if (CONFIG_INDEXING) {
            globalIndex.rectifyIndex(
                RectifyAction::RENAME,
                { oldPath },
                { newPath }
            );
        }

        invalidateDirCache(currPath);
        openDirectory(currPath, up_screen, down_screen);
        update_position(newName);
        displayFiles();
        pos();
        // displayFiles();
        // return;
    }
}

void createFile(string fileName) {
    // posx(rows-2, 0);
    // printf("\033[K");
    // printf("\033[1;33mEnter file name: \033[0m");

    // string fileName=get_input();
    // getline(cin >> ws, fileName);  // Read the full input while ignoring leading spaces

    if (fileName.empty()) {
        return;  // Exit if the input is empty
    }

    string filePath = string(currPath) + "/" + fileName;
    ofstream file(filePath);
    if (file) {
        file.close();
    }
    if (CONFIG_INDEXING) {
        globalIndex.rectifyIndex(
            RectifyAction::CREATE,
            {},
            { filePath }
        );
    }
    // displayFiles();
    invalidateDirCache(currPath);
    openDirectory(currPath, up_screen, down_screen);
    update_position(fileName);
    displayFiles();
    pos();
}

void createDirectory(string dirName) {
    // posx(rows-2, 0);
    // printf("\033[K");
    // printf("\033[1;33mEnter directory name: \033[0m");

    // string dirName=get_input();
    // getline(cin >> ws, dirName);  // Read the full input while ignoring leading spaces

    if (dirName.empty()) {
        return;  // Exit if the input is empty
    }

    string dirPath = string(currPath) + "/" + dirName;
    if (mkdir(dirPath.c_str(), 0777) == 0) {
        if (CONFIG_INDEXING) {
            globalIndex.rectifyIndex(
                RectifyAction::CREATE,
                {},
                { dirPath }
            );
        }
        invalidateDirCache(currPath);
        openDirectory(currPath, up_screen, down_screen);
        update_position(dirName);
        displayFiles();
        pos();
        // displayFiles();
        // return;
    }
}

void showHelp() {
    clearScreen;

    posx(1, 5);
    printf("\033[1;33mCOMMAND MODE HELP (:)\033[0m");

    int r = 3;

    posx(r++, 5);
    printf("\033[1;36mrename <new_name>\033[0m");
    printf("        Rename selected file / directory");

    posx(r++, 5);
    printf("\033[1;36mcreate_file <name>\033[0m");
    printf("       Create a new file in current directory");

    posx(r++, 5);
    printf("\033[1;36mcreate_dir <name>\033[0m");
    printf("        Create a new directory");

    posx(r++, 5);
    printf("\033[1;36mcd <absolute_path>\033[0m");
    printf("       Change directory using absolute path");

    posx(r++, 5);
    printf("\033[1;36msearch <name>\033[0m");
    printf("            Search files & directories");

    posx(r++, 5);
    printf("\033[1;36msearch --file <name>\033[0m");
    printf("     Search only files");

    posx(r++, 5);
    printf("\033[1;36msearch --dir <name>\033[0m");
    printf("      Search only directories");

    posx(r++, 5);
    printf("\033[1;36mfind <keywords>\033[0m");
    printf("          Find paths using indexed search");

    posx(r++, 5);
    printf("\033[1;36mfind --dir <keywords>\033[0m");
    printf("    Find directories under current path");

    posx(r++, 5);
    printf("\033[1;36mhelp | --help\033[0m");
    printf("            Show this help screen");

    posx(r++, 5);
    printf("\033[1;36mq\033[0m");
    printf("                        Exit command mode");

    posx(rows - 2, 5);
    printf("\033[1;32mPress any key to return...\033[0m");

    fflush(stdout);
    getchar();

    displayFiles();
    pos();
}


void navigateToAbsolutePath(string absPath) {

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