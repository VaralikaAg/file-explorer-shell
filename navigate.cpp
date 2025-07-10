#include "myheader.h"

char *currPath;
char *prevPath;
unsigned int xcurr=1, ycurr=cols/3;  // Only display last 3 files
struct termios initialrsettings, newrsettings;

#define esc 27

// #define clear fputs("\033[2J", stdout)                  // Clear screen
#define clear fputs("\033[H\033[2J", stdout)
#define pos() fprintf(stdout, "\033[%d;%dH", xcurr, ycurr)  // Move cursor
#define posx(x, y) fprintf(stdout, "\033[%d;%dH", x, y)  // Move to (x, y)
#define setCursorRed() fprintf(stdout, "\033]12;#ff0000\007")
#define resetCursorColor() fprintf(stdout, "\033]12;?\007")


// Updating current path if backspace is pressed
void setBackPath(char *path) {
    string tempPath = string(path);
    size_t position = tempPath.find_last_of("/\\");
    string newPath = tempPath.substr(0, position);
    strcpy(currPath, newPath.c_str());
}

void commandMode()
{
    posx(rows-2,0);
    printf("\033[K");
    // system("clear");
    printf("\033[1;36m:\033[0m ");

    string commandLine;
    // getline(cin >> ws, commandLine);  // Read entire line, ignoring leading whitespace
    commandLine=get_input();

    vector<string> args;
    stringstream ss(commandLine);
    string word;

    while (ss >> word) {
        args.push_back(word);
    }

    if (args.empty()) return; // If no command entered, return

    string command = args[0];

    if (command == "rename" && args.size()==1) {
        string selectedFile = fileList[xcurr + up_screen - 1];
        renameItem(selectedFile);
    }
    else if (command == "create_file" && args.size()==1) {
        createFile();
    } 
    else if (command == "create_dir" && args.size()==1){
        createDirectory();
    }
    else if (command == "cd" && args.size()>1){
        string absPath=args[1];
        for (int i = 2; i < static_cast<int>(args.size()); i++) absPath+=" "+args[i];
        // logMessage(absPath);
        navigateToAbsolutePath(absPath);
    }
    else if (command == "search"){
        string flag;
        bool check_dir = true, check_file = true;

        if (args.size() > 1) {
            string flag = args[1];
            if (flag == "--dir") check_file = false;
            else if (flag == "--file") check_dir = false;
            else return;
        }
        
        searchCommand(check_dir, check_file);

    }
    else if(command == "q"){
        displayFiles();
    }
    return;
}

void displayFiles() {
    // system("clear");
    clear;
    setCursorRed();

    // Printing previous directory
    // if (backStack.empty()) {
    //     posx(1, 3);
    //     printf("\033[1;31mNo Items\033[0m\n");
    // }
    // else{
    //     NavState prevState = backStack.top();
    //     prevPath = new char[prevState.path.length() + 1];
    //     strcpy(prevPath, prevState.path.c_str()); 
    //     // logMessage(to_string(prevState.xcurr));
    //     // logMessage(to_string(prevState.up_screen));

    //     // openDirectory(prevPath, prev_up_screen, prev_down_screen);
    //     openCurrDirectory(prevPath);
    //     // xcurr = prevState.xcurr;
    //     // prev_up_screen = prevState.up_screen;
    //     prev_down_screen = fileList.size() - prev_up_screen - rowSize;
    //     for (unsigned int i = prev_up_screen, line=1; i < prev_up_screen + rowSize && i < fileList.size(); i++, line++) {
    //         posx(line, 3);
    //         display(fileList[i].c_str(), prevPath);
    //     }       
    // }

    openCurrDirectory(currPath);
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

    posx(1,1);
    getFileDetails(newPath);

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
    ycurr=colSize;
    pos();
    // logMessage(to_string(ycurr));
}

void navigate() {
    printf("Inside navigate()\n");
    // logMessage(to_string(ycurr)+to_string(cols/3));

    if (root == nullptr) {
        fprintf(stderr, "Error: root is NULL\n");
        return;
    }

    currPath = new char[strlen(root) + 1];
    strcpy(currPath, root);
    printf("Current path: %s\n", currPath);

    if (fileList.empty()) {
        printf("Error: No files found in directory!\n");
        return;
    }

    // Start displaying only last five files
    openDirectory(currPath, up_screen, down_screen);
    up_screen = fileList.size() > rowSize ? fileList.size() - rowSize : 0;
    down_screen = fileList.size() - up_screen - rowSize;
    xcurr=rowSize<fileList.size()? rowSize:fileList.size();
    displayFiles();
    // xcurr=rowSize<fileList.size()? rowSize:fileList.size();
    pos();

    char ch;
    tcgetattr(fileno(stdin), &initialrsettings);

    newrsettings = initialrsettings;
    newrsettings.c_lflag &= ~(ICANON);
    newrsettings.c_lflag &= ~ECHO;
    // newrsettings.c_lflag |= ECHO;
    newrsettings.c_lflag |= ECHOE;

    tcsetattr(fileno(stdin), TCSANOW, &newrsettings);

    while (true) {
        ch = cin.get();

        if (ch == 27) {  // Escape sequence
            ch = cin.get();
            ch = cin.get();

            if (ch == 'A') {  // Up arrow key
                // printf("Up arrow key used\n");
                if(xcurr>1){
                    xcurr--;
                    displayFiles();
                    pos();
                }
                else if (xcurr==1 && up_screen > 0) {
                    up_screen--;
                    down_screen++;
                    displayFiles();
                }
                else{
                    displayFiles();
                }
            }

            else if(ch=='B'){
                // printf("Down arrow key used\n");
                openCurrDirectory(currPath);
                if(xcurr<rowSize && xcurr<fileList.size()){
                    xcurr++;
                    displayFiles();
                    pos();
                }
                else if(xcurr==rowSize && down_screen>0){
                    up_screen++;
                    down_screen--;
                    displayFiles();
                }
                else{
                    displayFiles();
                }
            }

            else if(ch=='C'){
                // printf("Right arrow used\n");
                string selectedFile = fileList[xcurr + up_screen - 1];
                string tempPath = string(currPath);
                if (tempPath.back() != '/') tempPath += "/";
                tempPath+= selectedFile;
                char *newPath = new char[tempPath.length() + 1]; 
                strcpy(newPath, tempPath.c_str()); 

                // if (isDirectory(tempPath.c_str())) {
                if (isDirectory(newPath)) {
                    openCurrDirectory(newPath);
                    // if(fileList.size()!=0){
                        NavState currentState;
                        currentState.path = string(currPath);
                        currentState.xcurr = xcurr;
                        currentState.up_screen = up_screen;
                        backStack.push(currentState);

                        delete[] currPath;
                        currPath = new char[strlen(newPath) + 1];
                        strcpy(currPath, newPath);
                        openDirectory(currPath, up_screen, down_screen); 
                        xcurr = 1;  
                        displayFiles();
                        pos();
                    // }
                    // else{
                    //     displayFiles();
                    // }
                }
                else{
                    // logMessage((string)newPath);
                    displayFiles();
                    pos();
                }
            }

            else if(ch == 'D'){
                // printf("Left arrow key used\n");
                if(!backStack.empty()){
                    NavState prevState = backStack.top();
                    backStack.pop();

                    delete[] currPath;
                    currPath = new char[prevState.path.length() + 1];
                    strcpy(currPath, prevState.path.c_str()); 

                    openCurrDirectory(currPath);
                    xcurr = prevState.xcurr;
                    up_screen = prevState.up_screen;
                    down_screen = fileList.size() - up_screen - rowSize;

                    // logMessage(prevState.path);
                    // logMessage(to_string(prevState.xcurr));
                    // logMessage(to_string(prevState.up_screen));

                    displayFiles();
                    pos();
                }
                else{
                    displayFiles();
                }
            }

        }
        else if (ch=='c'){
            string selectedFile = fileList[xcurr + up_screen - 1];
            displayFiles();
            copy(selectedFile);
        }
        else if (ch=='p'){
            paste();
            openDirectory(currPath, up_screen, down_screen);
            displayFiles();
            pos();
        }
        else if (ch == 'd') {
            string selectedFile = fileList[xcurr + up_screen - 1];
            displayFiles();
            deleteItem(selectedFile);
            openDirectory(currPath, up_screen, down_screen);
            displayFiles();
            pos();
        }
        else if (ch == ':') {
            displayFiles();
            commandMode();
            // openCurrDirectory(currPath);
            // displayFiles();
            // pos();
        }

        else if (ch == 127 || ch == 8){
            // printf("Backspace key used\n");
            if(!backStack.empty()){
                NavState prevState = backStack.top();
                backStack.pop();

                delete[] currPath;
                currPath = new char[prevState.path.length() + 1];
                strcpy(currPath, prevState.path.c_str()); 

                // openDirectory(currPath, up_screen, down_screen);
                openCurrDirectory(currPath);
                xcurr = prevState.xcurr;
                up_screen = prevState.up_screen;
                down_screen = fileList.size() - up_screen - rowSize;
                displayFiles();
                pos();
            }
        }
        else if (ch == '\n' || ch == '\r'){
            // printf("Enter used\n");
            string selectedFile = fileList[xcurr + up_screen - 1];
            string tempPath = string(currPath) + '/' + selectedFile;
            char *newPath = new char[tempPath.length() + 1]; 
            strcpy(newPath, tempPath.c_str()); 

            struct stat sb;
			stat(newPath, &sb);

            if (isDirectory(newPath)) {
                openCurrDirectory(newPath);
                if(fileList.size()!=0){
                    NavState currentState;
                    currentState.path = string(currPath);
                    currentState.xcurr = xcurr;
                    currentState.up_screen = up_screen;
                    backStack.push(currentState);

                    strcpy(currPath, newPath);
                    openDirectory(currPath, up_screen, down_screen); 
                    xcurr = 1;  
                    displayFiles();
                    pos();
                }
            }
            else if (S_ISREG(sb.st_mode))  // Check if it's a regular file
            {
                // Suppress error messages by redirecting stderr to /dev/null
                int nullFile = open("/dev/null", O_WRONLY);
                dup2(nullFile, STDERR_FILENO);
                close(nullFile);

                // Create a new process
                pid_t pid = fork();

                if (pid == 0)  // If this is the child process
                {
                    // Open the file using the system's default application
                    execlp("xdg-open", "xdg-open", newPath, NULL);

                    // If execlp fails, exit the child process
                    exit(EXIT_FAILURE);
                }
            }

            else
            {
                printf("Unknown File !!! :::::");
            }
        }
        else displayFiles();
    }

    // tcsetattr(fileno(stdin), TCSANOW, &initialrsettings);
    delete[] currPath;
}


// Signal handler for SIGINT (Ctrl+C)
void handleSigint(int signum) {
    
    tcsetattr(fileno(stdin), TCSANOW, &initialrsettings);
    printf("\033[0m"); 
    // printf("\033]12;white\007"); 
    fputs("\033[2J", stdout) ;
    fprintf(stdout, "\033[%d;%dH", 0, 0);

    // Reset the cursor color before exiting.
    // resetCursorColor();
    
    // Exit with the signal number.
    // cout << "Exited gracefully." << endl;
    exit(signum);
}
