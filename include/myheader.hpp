#ifndef MYHEADER_HPP
#define MYHEADER_HPP

#include "ansi_codes.hpp"
#include <algorithm>
#include <atomic>
#include <cctype>
#include <chrono>
#include <condition_variable>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <dirent.h>
#include <fcntl.h>
#include <filesystem>
#include <fstream>
#include <grp.h>
#include <iomanip>
#include <iostream>
#include <list>
#include <lmdb.h>
#include <map>
#include <mutex>
#include <nlohmann/json.hpp>
#include <pwd.h>
#include <queue>
#include <sstream>
#include <stack>
#include <string>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <termios.h>
#include <thread>
#include <unistd.h>
#include <unordered_map>
#include <unordered_set>
#include <vector>

using json = nlohmann::json;

namespace fs = std::filesystem;

// Extern Global Variables

struct NavState {
  std::string path; // The directory path
  int x_curr;        // Cursor row within the display window
  int up_screen;     // Starting index of files being displayed
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

class InvertedIndex {
  /* LMDB environment and sub-database handles */
  MDB_env *env = nullptr;
  MDB_dbi db_files;    // ino(u64)     → mtime(u64)      (unique, INTEGERKEY)
  MDB_dbi db_word_2_id;  // word(str)    → word_id(u32)    (unique)
  MDB_dbi db_id_2_word;  // word_id(u32) → word(str)       (unique, INTEGERKEY)
  MDB_dbi db_inverted; // word_id(u32) → ino(u64)        (DUPSORT | INTEGERKEY |
                      // DUPFIXED)
  MDB_dbi db_forward;  // ino(u64)     → word_id(u32)    (DUPSORT | INTEGERKEY |
                      // DUPFIXED)

  uint32_t next_word_id = 0; // monotonically increasing word ID
  uint64_t root_dev =
      0; // device ID of the indexing root (for /.vol/ resolution)

  /* Helper: resolve path to inode/dev */
  static void getStat(const std::string &path, uint64_t &ino, uint64_t &dev);
  static uint64_t getMtime(const std::string &path);

public:
  /* Lifecycle */
  void open(const std::string &dbDir); // call once at startup
  void close();                        // call at exit

  /* Accessors */
  MDB_env *getEnv() const { return env; }

  /* Core operations (same external API) */
  void indexPath(const std::string &path, MDB_txn *txn = nullptr);
  void updatePath(const std::string &path, MDB_txn *txn = nullptr); // Diffing update
  void search(const std::string &query);
  void removePath(const std::string &path, MDB_txn *txn = nullptr);

  /* Differential sync support */
  uint64_t getLastSyncTime();
  void setLastSyncTime(uint64_t ts);

  /* Debug: print full word dictionary to logs */
  void dumpWords();
};

struct NavigatorState {
  std::string root;
  std::string curr_path;
  std::string prev_path;

  std::vector<std::string> file_list;

  int x_curr = 1;
  int y_curr = 0;

  int up_screen = 0;
  int down_screen = 0;

  std::stack<NavState> back_stack;
  std::stack<NavState> forward_stack;
};

struct SearchState {
  std::vector<std::string> found_paths;
};

struct Config {
  int workers = 4;
  bool indexing_enabled = false;
  std::string indexing_root;
};

enum class WatcherEventType { CREATE, MODIFY, DELETE, RENAME };

struct WatcherEvent {
  WatcherEventType type;
  std::string path;
  std::string old_path; // only for RENAME
};

/* Base class for inotify/FSEvents */
class FileSystemWatcher {
public:
  virtual ~FileSystemWatcher() {}
  virtual bool start(const std::string &path) = 0;
  virtual void stop() = 0;
};

/* Factory function (implemented in platform-specific .cpp) */
FileSystemWatcher *createWatcher();

struct IndexingState {
  std::queue<WatcherEvent> event_queue; // Queue for real-time events
  std::queue<std::string> index_queue;  // Queue for startup crawl
  InvertedIndex index;

  std::atomic<bool> indexing_in_progress{false};
  std::atomic<bool> stop_indexer{false};
  std::thread worker;

  std::mutex mtx;
  std::condition_variable cv;

  FileSystemWatcher *watcher = nullptr;
};

struct SizeComputationState {
  std::atomic<bool> cancel_flag{false};
  std::atomic<bool> in_progress{false};
  std::atomic<off_t> last_size{0};

  std::thread worker;

  std::mutex mtx;
  std::condition_variable cv;
};

struct UIState {
  int rows = 0;
  int cols = 0;

  std::atomic<bool> refresh{false};
  std::mutex mtx;
};

struct SelectionState {
  std::vector<std::string> clipboard;
  std::unordered_set<std::string> selected_files;
};

struct CacheState {
  std::unordered_map<std::string, std::vector<std::string>> dir_cache;
  const int MAX_CACHE_ENTRIES = 1000000;
};

struct LayoutState {
  int resized = 0;

  int row_size = 0;
  int total_files = 0;
  int col_size = 0;

  int prev_up_screen = 0;
  int prev_down_screen = 0;
  int for_up_screen = 0;
  int for_down_screen = 0;
};

struct FileDetailsState {
  std::string file_name;
  std::string current_path;
  std::string user_name;
  std::string group_name;
  std::string permissions;
  std::string file_size;

  int last_scan_duration = 0;
  int col_size = 0;
  std::string time_buffer;
};

struct AppState {
  NavigatorState nav;
  SearchState search;
  Config config;
  IndexingState indexing;
  SizeComputationState size_state;
  UIState ui;

  SelectionState selection;
  CacheState cache;
  LayoutState layout;
  FileDetailsState file_details;
};

struct CommandResult {
  bool success = true;
  bool refresh = true;
  std::string message = "";
  std::string color = "\033[1;32m";
  std::string target_file = "";
};

extern termios initial_rsettings, new_rsettings;

extern AppState app;

constexpr int ESC = 27;
inline void clearScreen() noexcept {
  std::cout << ANSI::HOME << ANSI::CLEAR_SCREEN << std::flush;
}
inline void setDefaultCursorPos() noexcept {
  std::cout << ANSI::MOVE_TO(app.nav.x_curr, app.nav.y_curr) << std::flush;
}
inline void setCursorPos(int x, int y) noexcept {
  std::cout << ANSI::MOVE_TO(x, y) << std::flush;
}
inline void setCursorRed() noexcept {
  std::cout << ANSI::CURSOR_RED << std::flush;
}
inline void resetCursorColor() noexcept {
  std::cout << ANSI::CURSOR_RESET << std::flush;
}

// Global Method Declarations
std::vector<std::string> getDirectoryFiles(const fs::path &path);
void openDirectory(const std::string &path, int &up, int &down);
void openCurrDirectory(const std::string &path);
void navigate();
void display(const std::string &file_name, const fs::path &root) noexcept;
bool isDirectory(const fs::path &path) noexcept;
void invalidateDirCache(const std::string &dir_path);
void handleSigint(int signum);
void handleResize(int sig);
void handleCopyAction();
std::string paste();
void deleteSelectedItems();
void commandMode();
bool renameItem(const std::string &selected_file, const std::string &new_name);
bool createFile(const std::string &file_name);
bool createDirectory(const std::string &dir_name);
void navigateToAbsolutePath(const std::string &abs_path);
void searchAnything(const fs::path &dir, const std::string &file_name,
                    bool check_file, bool check_dir);
void searchCommand(bool check_dir, bool check_file, std::string file_name);
void showHelp();
void handleToggleSelectAction();
void normalizeCursor();
void loadConfig();
void displaySearchResults();
bool isReadable(const fs::path &path) noexcept;
bool isBinaryFile(const fs::path &path);
void displayTextFile(const std::string &file_path);
void normalizeRange(int total, int row_size, int &up, int &down, int &x);
void scrollToIndex(int index, int total, int row_size, int &up, int &down,
                   int &x);
void updatePosition(const std::string &file_name);
void logMessage(const std::string &message);
std::string getInput();
void getTerminalSize();
void getFileDetails(const std::string &path);
std::uintmax_t getFolderSizeMT(const std::string &root_path, int num_threads);
std::uintmax_t getFolderSize(const std::string &path);
std::string humanReadableSize(off_t size);
std::string normalizeWord(const std::string &input);
bool isUnderCurrentDir(const std::string &path);
void hideCursor();
void showCursor();
void stopFolderScan();
void runIndexingInBackground(const std::string root);
void renderUI();
bool inputAvailable();
void renderLeftPanel();
void cleanupAndExit();

void startIndexing();
void initializeNavigation(int argc, char *argv[]);
void renderMiddlePanel();
void renderRightPanel();
void renderFilePreview(const std::string &new_path, bool is_readable);
void renderDirectoryPreview(const std::string &new_path);
void jumpToSearchResult(const std::string &selected_path);
bool isRegularFile(const fs::path &path) noexcept;
std::string truncateStr(const std::string &str, size_t max_length);
void showStatusMessage(const std::string &msg, const std::string &color, int wait_ms = 0);
CommandResult processCommand(const std::string &command_line);
void handleResizeIfNeeded();
fs::path getSelectedPath() noexcept;
void refreshCurrentDirectory();
void openFile(const std::string &path) noexcept;
void handleEnterAction();
#endif // MYHEADER_HPP
