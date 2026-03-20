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
#include <lmdb.h>

namespace fs = std::filesystem;

// Extern Global Variables

struct NavState
{
    std::string path;   // The directory path
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

enum class KeyAction {
    ESC,
    UP,
    DOWN,
    LEFT,
    RIGHT,
    COPY,
    PASTE,
    DELETE,
    COMMAND,
    TOGGLE_SELECT,
    CLEAR_SELECTION,
    BACK,
    ENTER,
    UNKNOWN
};

/* Stored as binary value in db_files: 12 bytes total */
struct FileRecord {
    uint32_t file_id;
    uint64_t mtime;      // fs modification time (seconds since epoch)
} __attribute__((packed));

class InvertedIndex
{
    /* LMDB environment and sub-database handles */
    MDB_env *env        = nullptr;
    MDB_dbi  db_files;    // path(str)   → FileRecord  (unique, stores file_id + mtime)
    MDB_dbi  db_word2id;  // word(str)   → word_id(u32) (unique)
    MDB_dbi  db_id2word;  // word_id(u32)→ word(str)    (unique, INTEGERKEY)
    MDB_dbi  db_inverted; // word_id(u32)→ file_id(u32) (DUPSORT | INTEGERKEY | DUPFIXED)
    MDB_dbi  db_forward;  // file_id(u32)→ word_id(u32) (DUPSORT | INTEGERKEY | DUPFIXED)
    MDB_dbi  db_id2path;  // file_id(u32)→ path(str)    (unique, INTEGERKEY)

    uint32_t next_file_id = 0;  // monotonically increasing file ID
    uint32_t next_word_id = 0;  // monotonically increasing word ID

    /* Helper: encode mtime from filesystem */
    static uint64_t getMtime(const std::string &path);

public:
    /* Lifecycle */
    void open(const std::string &dbDir);   // call once at startup
    void close();                          // call at exit

    /* Core operations (same external API) */
    void indexPath(const std::string &path);
    void indexAllOnce(std::queue<std::string> &paths);
    void search(const std::string &query);
    void removePath(const std::string &path);
    void rectifyIndex(
        RectifyAction type,
        const std::vector<std::string> &oldPaths = {},
        const std::vector<std::string> &newPaths = {});

    /* Differential sync support */
    uint64_t getLastSyncTime();
    void     setLastSyncTime(uint64_t ts);
};

struct NavigatorState
{
    std::string root;
    std::string currPath;
    std::string prevPath;

    std::vector<std::string> fileList;

    int xcurr = 1;
    int ycurr = 0;

    int up_screen = 0;
    int down_screen = 0;

    std::stack<NavState> backStack;
    std::stack<NavState> forwardStack;
};

struct SearchState
{
    std::vector<std::string> foundPaths;
};

struct Config
{
    int workers = 4;
    bool indexingEnabled = false;
    std::string indexingRoot;
};

struct IndexingState
{
    std::queue<std::string> indexQueue;
    InvertedIndex index;
    /* freeFileIds removed: LMDB handles storage; next_file_id is monotonic */

    std::atomic<bool> indexingInProgress{false};
    std::thread worker;
};

struct SizeComputationState
{
    std::atomic<bool> cancelFlag{false};
    std::atomic<bool> inProgress{false};
    std::atomic<off_t> lastSize{0};

    std::thread worker;

    std::mutex mtx;
    std::condition_variable cv;
};

struct UIState
{
    int rows = 0;
    int cols = 0;

    std::atomic<bool> refresh{false};
    std::mutex mtx;
};

struct SelectionState
{
    std::vector<std::string> clipboard;
    std::unordered_set<std::string> selectedFiles;
};

struct CacheState
{
    std::unordered_map<std::string, std::vector<std::string>> dirCache;
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
    std::string fileName;
    std::string userName;
    std::string groupName;
    std::string permissions;
    std::string fileSize;

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
    std::string message = "";
    std::string color = "\033[1;32m";
    std::string targetFile = "";
};

extern struct termios initialrsettings, newrsettings;

extern AppState app;

constexpr int ESC = 27;
inline void clearScreen() noexcept {
    fputs("\033[H\033[2J", stdout);
}
inline void setDefaultCursorPos() noexcept {
    std::cout << "\033[" << app.nav.xcurr << ";" << app.nav.ycurr << "H" << std::flush;
}
inline void setCursorPos(int x, int y) noexcept {
    std::cout << "\033[" << x << ";" << y << "H" << std::flush;
}
inline void setCursorRed() noexcept {
    fprintf(stdout, "\033]12;#ff0000\007");
}
inline void resetCursorColor() noexcept {
    fprintf(stdout, "\033]112;\007");
}

// Global Method Declarations
int getDirectoryCount(const fs::path &path);
void openDirectory(const char *path, int &up, int &down);
void openCurrDirectory(const char *path);
void navigate();
void display(const std::string &fileName, const fs::path &root) noexcept;
bool isDirectory(const fs::path &path) noexcept;
void invalidateDirCache(const std::string &dirPath);
void handleSigint(int signum);
void handleResize(int sig);
void copy();
std::string paste();
void deleteSelectedItems();
void commandMode();
bool renameItem(const std::string &selectedFile, const std::string &newName);
bool createFile(const std::string &fileName);
bool createDirectory(const std::string &dirName);
void navigateToAbsolutePath(const std::string &absPath);
void searchAnything(const fs::path &dir, const std::string &filename, bool check_file, bool check_dir);
void searchCommand(bool check_dir, bool check_file, std::string filename);
void showHelp();
void toggleSelect();
void normalizeCursor();
void loadConfig();
void displaySearchResults();
bool isReadable(const fs::path &path) noexcept;
bool isBinaryFile(const fs::path &path);
void displayTextFile(const std::string &filepath);
void normalizeRange(int total, int rowSize, int &up, int &down, int &x);
void scrollToIndex(int index, int total, int rowSize, int &up, int &down, int &x);
void update_position(const std::string &fileName);
void logMessage(const std::string &message);
std::string get_input();
void get_terminal_size();
void getFileDetails(const std::string &path);
std::uintmax_t getFolderSizeMT(const std::string &rootPath, int numThreads);
std::string humanReadableSize(off_t size);
std::string normalizeWord(const std::string &input);
void traverse(const std::string &root);
bool isUnderCurrentDir(const std::string &path);
void hideCursor();
void showCursor();
void showTempMessage(const std::string &msg, int wait_ms);
void stopFolderScan();
void runIndexingInBackground(const std::string root);
void renderUI();
bool inputAvailable();
void print_details();
void cleanupAndExit();

void startIndexing();
void initializeNavigation(int argc, char *argv[]);
void renderMiddlePanel();
void renderRightPanel();
void renderStatusBar();
void renderFilePreview(const std::string &newPath, bool isReadable);
void renderDirectoryPreview(const std::string &newPath);
void jumpToSearchResult(const std::string &selectedPath);
bool isRegularFile(const fs::path &path) noexcept;
std::string truncateStr(const std::string &str, size_t maxLength);
void showStatusMessage(const std::string &msg, const std::string &color);
CommandResult processCommand(const std::string &commandLine);
void handleResizeIfNeeded();
fs::path getSelectedPath() noexcept;
void refreshCurrentDirectory();
void openFile(const std::string &path) noexcept;
void handleEnter();