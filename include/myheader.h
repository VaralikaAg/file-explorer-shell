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
#include <map>
#include <thread>
#include <atomic>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <chrono>

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
extern vector<string> clipboard;
extern unsigned int xcurr, ycurr;
extern vector<string> foundPaths;
extern unsigned int rows,cols;
extern unordered_map<string, vector<string>> dirCache;
extern unordered_set<string> selectedFiles;
extern int resized;
extern int CONFIG_WORKERS;

// Global Method Declarations
int getDirectoryCount(const char *path);
void openDirectory(const char *path, int &up, int &down);
void openCurrDirectory(const char *path);
void navigate();
void displayFiles();
void display(const char *fileName, const char *root);
bool isDirectory(const char *newPath);
void invalidateDirCache(const string &dirPath);
void handleSigint(int signum);
void handleResize(int sig);
void copy();
void paste();
void deleteSelectedItems();
void commandMode();
void renameItem(string selectedFile, string newName);
void createFile(string fileName);
void createDirectory(string dirName);
void navigateToAbsolutePath(string absPath);
void searchanything(char *path, string filename, bool check_file, bool check_dir);
void searchCommand(bool check_dir, bool check_file, string filename);
void showHelp();
void toggleSelect();
void normalizeCursor();
void loadConfig();
void displaySearchResults();
bool isReadableFile(const string &filepath);
bool isBinaryFile(const string &filepath);
void displayTextFile(const string &filepath);
void update_position(string fileName);
void logMessage(const std::string& message);
string get_input();
void get_terminal_size();
void getFileDetails(const string &path);
off_t getFolderSizeMT(const string &rootPath, int numThreads);
string humanReadableSize(off_t size);