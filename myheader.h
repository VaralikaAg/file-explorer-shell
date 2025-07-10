#include <bits/stdc++.h>
#include <iostream>
#include <iomanip>
#include <string>
#include <termios.h>
#include <dirent.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <fstream>
#include <cctype>
#include <cstdlib>
#include <sys/ioctl.h>
#include <pwd.h>
#include <grp.h>
#include <ctime>
using namespace std;

// Extern Global Variables


struct NavState {
    std::string path;        // The directory path
    unsigned int xcurr;      // Cursor row within the display window
    unsigned int up_screen;  // Starting index of files being displayed
};

extern char *root;
extern char *currPath;
extern char *prevPath;
extern int up_screen, down_screen, prev_up_screen, prev_down_screen, for_up_screen, for_down_screen;
extern unsigned int rowSize, totalFiles, colSize;
extern vector<string> fileList;
extern stack<NavState> backStack;
extern stack<NavState> forwardStack;
extern string clipboard;
extern unsigned int xcurr, ycurr;
extern vector<string> foundPaths;
extern unsigned int rows,cols;


// Global Method Declarations
int getDirectoryCount(const char *path);
void openDirectory(const char *path, int &up, int &down);
void openCurrDirectory(const char *path);
void navigate();
void displayFiles();
void display(const char *fileName, const char *root);
bool isDirectory(const char *newPath);
void handleSigint(int signum);
void copy(string selectedFile);
void paste();
void deleteItem(string selectedFile);
void commandMode();
void renameItem(string selectedFile);
void createFile();
void createDirectory();
void navigateToAbsolutePath(string absPath);
void searchanything(char *path, string filename, bool check_file, bool check_dir);
void searchCommand(bool check_dir, bool check_file);
void displaySearchResults();
bool isReadableFile(const string &filepath);
bool isBinaryFile(const string &filepath);
void displayTextFile(const string &filepath);
void logMessage(const std::string& message);
string get_input();
void get_terminal_size();
void getFileDetails(const string &path);
string humanReadableSize(off_t size);