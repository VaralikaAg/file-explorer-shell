// Including the header file
#include "myheader.h"

#define resetCursorColor() fprintf(stdout, "\033]12;?\007")
#define pos() fprintf(stdout, "\033[%d;%dH", xcurr, ycurr)
unsigned int rows,cols, rowSize, colSize;
int resized, CONFIG_WORKERS;
bool CONFIG_INDEXING;
string CONFIG_INDEXING_ROOT;
queue<string> indexQueue;
atomic<bool> indexingInProgress{false};
thread indexingThread;


char *root;

void get_terminal_size() {
    struct winsize w;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
    rows = w.ws_row;  // Number of rows
    cols = w.ws_col;  // Number of columns
}


// Main method
int main(int argc, char *argv[]){

    loadConfig();

    // signal(SIGINT, handleSigint);
    signal(SIGINT, SIG_IGN);
    signal(SIGWINCH, handleResize);
    get_terminal_size();
    rowSize=rows-10;
    colSize=cols/3;
    resized=0;
    // logMessage(to_string(colSize))

    // setvbuf(stdout, nullptr, _IOFBF, 1 << 20); // 1MB buffer

    /*********** INDEXING *********/
    string s = "Process ID: " + to_string(getpid());
    logMessage(s);
    // cout << "Process ID: " << getpid() << endl;
    string main_root = CONFIG_INDEXING_ROOT;

    if (CONFIG_INDEXING) {
        indexingThread = std::thread(runIndexingInBackground, main_root);
        indexingThread.detach();
    } else {
        logMessage("Indexing skipped (disabled in config)");
    }

    if(argc==1){
        char *currentDir = getcwd(NULL, 0);
        root=currentDir;
    }
    else{
		root = argv[1];
    }
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

    for (size_t ind=0; ind<pathParts.size(); ind++) {
        const auto dir = pathParts[ind];
        newPath = newPath + "/" + dir;

        // Get the file list of the current directory
        vector<string> currFileList;
        DIR *d = opendir(newPath.c_str());
        if (d) {
            struct dirent *entry;
            while ((entry = readdir(d)) != nullptr) {
                string name = entry->d_name;
                if (name == "." || name == "..") continue;
                currFileList.push_back(name);
            }
            closedir(d);
            sort(currFileList.begin(), currFileList.end());
        }

        // Find the index of 'dir' in the parent's file list
        int dirIndex = 0;
        if(ind<pathParts.size()-1){
            for (size_t i = 0; i < currFileList.size(); i++) {
                if (currFileList[i] == pathParts[ind+1]) {
                    dirIndex = i;
                    break;
                }
            }
        }

        // Compute xcurr and up_screen
        if (dirIndex != -1) {
            // Calculate cursor & scrolling
            if ((unsigned int)dirIndex < rowSize) {
                up_screen = 0;
                xcurr = dirIndex + 1;
            } else {
                up_screen = dirIndex - rowSize + 1;
                xcurr = rowSize;
            }
        }

        // Push NavState
        NavState currState;
        currState.path = newPath;
        currState.xcurr = xcurr;
        currState.up_screen = up_screen;

        tempStack.push(currState);
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

    navigate();

    return 0;
}