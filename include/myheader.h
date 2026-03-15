#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <stack>
#include <list>
#include <algorithm>
#include <iostream>
#include <iomanip>
#include <string>
#include <termios.h>
#include <dirent.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <fstream>
#include <sstream>
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
#include <cstring>
#include <filesystem>

using namespace std;
namespace fs = std::filesystem;

// Extern Global Variables

struct NavState
{
    string path;   // The directory path
    int xcurr;     // Cursor row within the display window
    int up_screen; // Starting index of files being displayed
};

enum class RectifyAction
{
    CREATE,
    COPY,
    RENAME,
    DELETE
};

class InvertedIndex
{
    /* FILE ID MAPPING */
    unordered_map<string, int> fileToId;
    vector<string> idToFile;

    /* WORD ID MAPPING */
    unordered_map<string, int> wordToId;
    vector<string> idToWord;

    /* INVERTED INDEX (word_id → set<file_id>) */
    unordered_map<int, unordered_set<int>> invertedIndex;

    /* FORWARD INDEX (file_id → set<word_id>) */
    unordered_map<int, unordered_set<int>> forwardIndex;

public:
    void indexPath(const string &path);
    void indexAllOnce(queue<string> &paths);
    void search(const string &word);
    int getWordId(const string &word);
    void rectifyIndex(
        RectifyAction type,
        const vector<string> &newPaths = {},
        const vector<string> &oldPaths = {});
    void removePath(const string &path);
};

struct NavigatorState
{
    string root;
    string currPath;
    string prevPath;

    vector<string> fileList;

    int xcurr = 1;
    int ycurr = 0;

    int up_screen = 0;
    int down_screen = 0;

    stack<NavState> backStack;
    stack<NavState> forwardStack;
};

struct SearchState
{
    vector<string> foundPaths;
};

struct Config
{
    int workers = 4;
    bool indexingEnabled = false;
    string indexingRoot;
};

struct IndexingState
{
    queue<string> indexQueue;
    InvertedIndex index;
    vector<int> freeFileIds;

    atomic<bool> indexingInProgress{false};
    thread worker;
};

struct SizeComputationState
{
    atomic<bool> cancelFlag{false};
    atomic<bool> inProgress{false};
    atomic<off_t> lastSize{0};

    thread worker;

    mutex mtx;
    condition_variable cv;
};

struct UIState
{
    int rows = 0;
    int cols = 0;

    atomic<bool> refresh{false};
    mutex mtx;
};

struct SelectionState
{
    vector<string> clipboard;
    unordered_set<string> selectedFiles;
};

struct CacheState
{
    unordered_map<string, vector<string>> dirCache;
    const int max_cache_entries = 1000000;
};

struct LayoutState
{
    int resized = 0;

    int rowSize = 0;
    int totalFiles = 0;
    int colSize = 0;

    int prev_up_screen = 0;
    int prev_down_screen = 0;
    int for_up_screen = 0;
    int for_down_screen = 0;
};

struct FileDetailsState
{
    string fileName;
    string userName;
    string groupName;
    string permissions;
    string fileSize;

    int lastScanDuration = 0;
    int colSize = 0;
    char timeBuffer[80];
};

struct AppState
{
    NavigatorState nav;
    SearchState search;
    Config config;
    IndexingState indexing;
    SizeComputationState sizeState;
    UIState ui;

    SelectionState selection;
    CacheState cache;
    LayoutState layout;
    FileDetailsState fileDetails;
};

struct CommandResult {
    bool success = true;
    bool refresh = true;
    string message = "";
    string color = "\033[1;32m";
    string targetFile = "";
};

extern struct termios initialrsettings, newrsettings;

extern AppState app;

#define esc 27
#define clearScreen fputs("\033[H\033[2J", stdout)
#define pos() cout << "\033[" << app.nav.xcurr << ";" << app.nav.ycurr << "H" << flush
#define posx(x, y) cout << "\033[" << x << ";" << y << "H" << flush
#define setCursorRed() fprintf(stdout, "\033]12;#ff0000\007")
#define resetCursorColor() fprintf(stdout, "\033]112\007")

// Global Method Declarations
int getDirectoryCount(const fs::path &path);
void openDirectory(const char *path, int &up, int &down);
void openCurrDirectory(const char *path);
void navigate();
void display(string fileName, string root);
bool isDirectory(const char *newPath);
void invalidateDirCache(const string &dirPath);
void handleSigint(int signum);
void handleResize(int sig);
void copy();
string paste();
void deleteSelectedItems();
void commandMode();
bool renameItem(const string &selectedFile, const string &newName);
bool createFile(const string &fileName);
bool createDirectory(const string &dirName);
void navigateToAbsolutePath(const string &absPath);
void searchanything(const char *path, string filename, bool check_file, bool check_dir);
void searchCommand(bool check_dir, bool check_file, string filename);
void showHelp();
void toggleSelect();
void normalizeCursor();
void loadConfig();
void displaySearchResults();
bool isReadableFile(const string &filepath);
bool isBinaryFile(const string &filepath);
void displayTextFile(const string &filepath);
void update_position(const string &fileName);
void logMessage(const string &message);
string get_input();
void get_terminal_size();
void getFileDetails(const string &path);
std::uintmax_t getFolderSizeMT(const string &rootPath, int numThreads);
string humanReadableSize(off_t size);
string normalizeWord(const string &input);
void traverse(const string &root);
bool isUnderCurrentDir(const string &path);
bool isValidDirectory(const string &path);
void hideCursor();
void showCursor();
void showTempMessage(const string &msg, int wait_ms);
void stopFolderScan();
void runIndexingInBackground(const string root);
void renderUI();
bool inputAvailable();
void print_details();
void cleanupAndExit();

void startIndexing();
void initializeNavigation(int argc, char *argv[]);
void renderMiddlePanel();
void renderRightPanel();
void renderStatusBar();
void renderFilePreview(const string &newPath, bool isReadable);
void renderDirectoryPreview(const string &newPath);
void jumpToSearchResult(const string &selectedPath);
bool isDirectoryPath(const string &path);
bool isRegularFile(const string &path);
string truncateStr(const string &str, size_t maxLength);
void showStatusMessage(const string &msg, const string &color);
CommandResult processCommand(const string &commandLine);
void handleResizeIfNeeded();
string getSelectedPath();
void refreshCurrentDirectory();