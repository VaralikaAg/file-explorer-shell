// Including the header file
#include "myheader.h"

#define resetCursorColor() fprintf(stdout, "\033]12;?\007")
unsigned int rows,cols, rowSize, colSize;

char *root;

void logMessage(const std::string& message) {
    std::ofstream logFile("debug.log", std::ios_base::app); // Open log file in append mode
    if (logFile.is_open()) {
        logFile << message << std::endl; // Write message to file
    } else {
        std::cerr << "Error opening log file!" << std::endl;
    }
}

void get_terminal_size() {
    struct winsize w;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
    rows = w.ws_row;  // Number of rows
    cols = w.ws_col;  // Number of columns
}


// Main method
int main(int argc, char *argv[]){

    signal(SIGINT, handleSigint);
    get_terminal_size();
    // logMessage(to_string(rows));
    rowSize=rows-10;
    colSize=cols/3;
    // logMessage(to_string(colSize));

    // printf("Hello main!!\n");
    if(argc==1){
        // printf("Started!!\n");
        string s = ".";
		char *path = new char[2];
		strcpy(path, s.c_str());
		root = path;
        // root=".";
        char *currentDir = getcwd(NULL, 0);
        root=currentDir;
        string absPath=root;

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
        // bool found = false;

        for (const auto& dir : pathParts) {
            // Simulate getting the contents of the directory
            newPath=newPath + "/" + dir;
            // Create a new navigation state and push it to the stack
            NavState currState;
            currState.path = newPath;
            currState.xcurr = 1; // Simulating the cursor or position
            currState.up_screen = 0; // You can modify this logic as per your need

            tempStack.push(currState); // Push the state to the stack
            // found = true;

            // Update the path for the next iteration
            // newPath = newPath + "/" + dir;
                
        }
        

        while (!backStack.empty()) backStack.pop();
        while (!tempStack.empty()) {
            dummyStack.push(tempStack.top());
            tempStack.pop();
        }
        while (!dummyStack.empty()) {
            backStack.push(dummyStack.top());
            dummyStack.pop();
        }
        backStack.pop();

        printf("Opening directory: %s\n", root);
		openDirectory(root, up_screen, down_screen);
    }

    navigate();

    return 0;
}