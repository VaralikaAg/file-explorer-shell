# 📚 Refined Explorer — Complete Technical Documentation

> A Yazi-like terminal file explorer built in Modern C++ with a persistent LMDB-backed search engine.

---

## 📋 Table of Contents

1. [Project Overview](#1-project-overview)
2. [Architecture](#2-architecture)
3. [Dependencies](#3-dependencies)
4. [Installation & Build](#4-installation--build)
5. [Configuration](#5-configuration)
6. [Features Deep Dive](#6-features-deep-dive)
   - [3-Panel UI](#61-3-panel-terminal-ui)
   - [Navigation](#62-navigation-system)
   - [Directory Cache](#63-directory-cache)
   - [File Operations](#64-file-operations)
   - [Selection System](#65-selection-system)
   - [Command Mode](#66-command-mode)
   - [Filename Search](#67-filename-search-search)
   - [Content Search (Indexing)](#68-content-search--find)
   - [Real-Time Watcher](#69-real-time-filesystem-watcher)
7. [Indexing Architecture Deep Dive](#7-indexing-architecture-deep-dive)
8. [Complexity Analysis](#8-complexity-analysis)
9. [Testing](#9-testing)
10. [Project Structure](#10-project-structure)
11. [Troubleshooting & FAQs](#11-troubleshooting--faqs)
12. [Future Roadmap](#12-future-roadmap)

---

## 1. Project Overview

**Refined Explorer** is a high-performance, terminal-based file manager written in **C++17**. It is inspired by [Yazi](https://yazi-rs.github.io/) and [Ranger](https://ranger.github.io/) but goes further by embedding a **persistent, full-text search engine** directly into the file explorer — powered by [LMDB](https://www.symas.com/lmdb) (Lightning Memory-Mapped Database).

### What makes it unique?

| Feature | Standard TUI Explorers | Refined Explorer |
| :--- | :--- | :--- |
| Navigation | ✅ | ✅ |
| File Operations | ✅ | ✅ |
| Filename Search | Basic (`find`) | ✅ Recursive + Filtered |
| **Content Search** | ❌ Relies on `grep` | ✅ **Persistent inverted index (LMDB)** |
| Real-time Index Updates | ❌ | ✅ **FSEvents (macOS) / inotify (Linux)** |
| **Search Speed** | ~300-800ms (`grep -R`) | **~5-20ms (indexed)** |

---

## 2. Architecture

The application has a clean layered architecture:

```
┌────────────────────────────────────────────────────┐
│                  Terminal UI Layer                 │
│  (3 panels: metadata | file list | preview)        │
│                  src/tui/                          │
└──────────────────────┬─────────────────────────────┘
                       │ keypresses / commands
┌──────────────────────▼─────────────────────────────┐
│              Application Core Layer                │
│  Navigation · Commands · Selection · Dir Cache     │
│               src/core/                            │
└────────┬──────────────────────────┬────────────────┘
         │                          │
┌────────▼─────────┐   ┌────────────▼────────────────┐
│  Utility Layer   │   │    Indexing & Search Layer   │
│  text_utils.cpp  │   │  inverted_index.cpp          │
│  format.cpp      │   │  traversal.cpp               │
│  file_utils.cpp  │   │  watcher_mac/linux.cpp       │
│  src/utils/      │   │  src/index/                  │
└──────────────────┘   └─────────────────────────────┘
                                   │
                       ┌───────────▼──────────────┐
                       │   LMDB on-disk Database  │
                       │  ~/.cache/refined-explorer│
                       └──────────────────────────┘
```

### Core Data Flow

1. **User launches the app** → `main.cpp` runs `loadConfig()`, `startIndexing()`, `initializeNavigation()`, then `navigate()`.
2. **Background thread** starts crawling the `indexingRoot`, tokenizing files, and persisting them into LMDB.
3. **Watcher thread** listens for real-time filesystem events (file created/deleted/renamed/modified) and re-indexes changed files.
4. **User navigates** → the UI redraws from the directory cache or a fresh filesystem scan.
5. **User searches** → the query is tokenized, word IDs are looked up in LMDB, inode sets are intersected, and paths are resolved.

---

## 3. Dependencies

### Core Language & Standard Library

| Dependency | Version | Purpose |
| :--- | :--- | :--- |
| **C++ Standard** | C++17 | `std::filesystem`, structured bindings, `if constexpr` |
| **POSIX** | Standard | `stat`, `opendir`, `readdir`, `fcntl`, `signal`, `ioctl` |
| **pthreads** | POSIX | `std::thread` background workers |

### External Libraries

| Library | Install | Purpose |
| :--- | :--- | :--- |
| **LMDB** | Auto-fetched via `FetchContent` from LMDB_0.9.32 (no `brew install` needed) | Persistent B+ Tree key-value store for the inverted index |
| **nlohmann/json** | Auto-fetched via `FetchContent` (v3.11.3) | JSON config file parsing (`config.json`) |
| **CoreServices** (macOS only) | Included in Xcode | `FSEvents` API for real-time filesystem monitoring |
| **inotify** (Linux only) | Built into Linux kernel | Real-time filesystem event monitoring |

> [!NOTE]
> Both LMDB and nlohmann/json are **automatically downloaded and compiled** by CMake's `FetchContent` at configure time. You do **not** need `brew install lmdb` — the build is fully self-contained.

### Build Tools

| Tool | Version | Purpose |
| :--- | :--- | :--- |
| **CMake** | 3.10+ | Cross-platform build system |
| **Compiler** | Apple Clang (macOS) / GCC 11+ (Linux) | C++17 compilation |
| **GoogleTest** | v1.14.0 | Auto-fetched via `FetchContent` for unit testing |

> [!IMPORTANT]
> On macOS, you **must** use **Apple Clang** (not Homebrew GCC). Apple's `CoreServices.framework` uses "Blocks" syntax (`^`) which only Clang supports. Set your compiler with `CC=clang CXX=clang++ cmake ..`

---

## 4. Installation & Build

### Step 1 — Install Dependencies

**macOS:**
```bash
# Only cmake is needed — LMDB and nlohmann/json are auto-fetched by CMake
brew install cmake
# Apple Clang is already available via Xcode Command Line Tools
xcode-select --install
```

**Ubuntu/Debian:**
```bash
sudo apt-get update
# liblmdb-dev is NOT required — LMDB is built from source by FetchContent
sudo apt-get install -y cmake g++ build-essential
```

### Step 2 — Clone & Build

```bash
# Clone the repo
git clone https://github.com/your-username/refined-explorer.git
cd refined-explorer

# Create build directory
mkdir build && cd build

# Configure (macOS - use Apple Clang explicitly)
CC=clang CXX=clang++ cmake ..

# Build with parallel jobs (adjust -j to your CPU core count)
cmake --build . -j$(nproc)
```

The build outputs two binaries in `build/`:
- `refined_explorer` — the main application
- `tests_runner` — the automated test suite

### Step 3 — Run

```bash
# Start from current directory
./build/refined_explorer

# Start from a specific path
./build/refined_explorer /Users/you/Documents
```

> [!NOTE]
> On first run, LMDB opens at `~/.cache/refined-explorer/lmdb/`. The background indexer begins crawling the `indexing_root` defined in `config.json`. This is a one-time crawl; subsequent launches re-use the persistent index and only re-index changed files.

---

## 5. Configuration

Create or edit `config.json` in the project root (YAML is **not** used — the actual format is JSON):

```json
{
  "performance": {
    "workers": 5,
    "indexing": true,
    "indexing_root": "/Users/you/Developer"
  }
}
```

| Key | Type | Default | Description |
| :--- | :--- | :--- | :--- |
| `performance.workers` | int | 4 | Threads for `getFolderSizeMT()` (directory size computation) |
| `performance.indexing` | bool | false | Master switch for LMDB indexing |
| `performance.indexing_root` | string | `/home` | The root directory the indexer crawls |

The config is parsed at startup in `src/utils/config.cpp:loadConfig()` (not `system.cpp`). It uses **nlohmann/json** for parsing. If the file is missing or malformed, safe defaults are used (`workers=4`, `indexing=false`, `indexing_root=/home`).

---

## 6. Features Deep Dive

### 6.1 3-Panel Terminal UI

The UI is divided into three columns rendered with **ANSI escape codes** directly to `stdout`, with no ncurses dependency:

```
┌──────────────────┬────────────────────┬──────────────────────┐
│   LEFT PANEL     │   MIDDLE PANEL     │   RIGHT PANEL        │
│  File Metadata   │   File List        │   Preview            │
│  ─────────────── │   ───────────────  │   ───────────────    │
│  Name: main.cpp  │ > src/             │  #include "..."      │
│  Size: 712 B     │   include/         │  int main() {        │
│  User: varalika  │   tests/           │    loadConfig();     │
│  Perms: rwxr-xr-x│   CMakeLists.txt  │    startIndexing();  │
│  Modified: ...   │   README.md        │    navigate();       │
└──────────────────┴────────────────────┴──────────────────────┘
```

- **Left Panel**: Populated by `src/core/file_details.cpp` (metadata logic) and rendered by `src/tui/details_ui.cpp`. Calls `stat()` for file metadata (size, timestamps, permissions), `getpwuid()` for username, `getgrgid()` for group name. Also shows **real-time scan duration** (`Scan Time: X ms`) and live `Calculating...` updates for folder sizes.
- **Middle Panel**: Rendered from `app.nav.file_list` (snake_case field — the in-memory directory listing from the cache).
- **Right Panel**: Calls `isBinaryFile()` (reads first 512 bytes), then renders file content or directory listing.
- **Terminal Resize**: `SIGWINCH` signal is registered to `handleResize()`. The handler sets a flag, and the main loop calls `handleResizeIfNeeded()` (in `src/core/system.cpp`) which re-queries terminal size with `ioctl(TIOCGWINSZ)` and redraws.

### 6.2 Navigation System

Navigation state is managed in `NavigatorState` (from `myheader.h`):

```cpp
struct NavigatorState {
    std::string root;            // Startup root directory
    std::string curr_path;       // Current directory being viewed
    std::string prev_path;       // Previously visited directory
    std::vector<std::string> file_list; // Visible files in curr_path
    int x_curr   = 1;           // Cursor row on screen (1-indexed)
    int y_curr   = 0;           // Cursor column (panel column)
    int up_screen  = 0;         // Index of first visible file
    int down_screen = 0;        // Count of files scrolled past bottom
    std::stack<NavState> back_stack;    // History for ← navigation
    std::stack<NavState> forward_stack; // History for redo
};
```

**Cursor & Scroll Algorithm** (`navigator.cpp:normalizeRange`):

The cursor is clamped to always be within `[1, min(rowSize, visibleFiles)]`. The scroll offset `up_screen` is clamped to `[0, max(0, total - rowSize)]`.

```
if (up_screen < 0)        up_screen = 0
if (up_screen > maxUp)    up_screen = maxUp       // maxUp = total - rowSize
if (xcurr < 1)            xcurr = 1
if (xcurr > maxX)         xcurr = maxX            // maxX = min(rowSize, visible)
down_screen = max(0, total - up_screen - rowSize)
```

**Keybindings:**

| Key | Action | Implementation |
| :--- | :--- | :--- |
| `↑` | Cursor up (scroll if at top of window) | `navigation.cpp` |
| `↓` | Cursor down (scroll if at bottom of window) | `navigation.cpp` |
| `→` | Enter directory / open file | `navigation.cpp:handleEnterAction()` |
| `←` / `Backspace` | Go to parent | Pops `backStack` |
| `:cd <path>` | Jump to absolute path | `navigator.cpp:navigateToAbsolutePath()` |

### 6.3 Directory Cache

**File**: `src/core/dir_cache.cpp`

To avoid repeated filesystem syscalls (`opendir`/`readdir`) when entering the same directory multiple times, entries are cached in memory:

```cpp
struct CacheState {
    std::unordered_map<std::string, std::vector<std::string>> dir_cache; // snake_case
    const int MAX_CACHE_ENTRIES = 1000000;
};
```

**Cache Lookup Logic** (`getDirectoryCount`):
1. Check if `dirCache[path]` exists → if yes, return the cached listing immediately. **O(1) average**.
2. If not, call `fs::directory_iterator`, collect and **sort** filenames alphabetically, store in cache, return listing.
3. For operations that change directory contents (create, delete, rename, paste), `invalidateDirCache(path)` removes the stale entry so the next visit does a fresh scan.

**Complexity**: Cache hit → **O(1)**. Cache miss → **O(N log N)** where N = number of files in directory (for sorting).

### 6.4 File Operations

All operations are implemented in `src/core/commands.cpp`:

#### Copy (`c` key)
```
If selectedFiles is non-empty:
    clipboard ← all paths in selectedFiles
Else:
    clipboard ← [currPath/fileAtCursor]
```
- Uses `app.selection.clipboard` (a `std::vector<std::string>`)
- **Complexity**: O(S) where S = number of selected files

#### Paste (`p` key)
- Uses `std::filesystem::copy` with `overwrite_existing` flag.
- Recursive directory copy via `copy_options::recursive`.
- **Complexity**: O(F) where F = total files being copied

#### Delete (`d` key)
- Uses `std::filesystem::remove_all` for full recursive deletion.
- Clears the selection state after.
- **Complexity**: O(F) where F = total files being deleted

#### Rename (`:rename <new_name>`)
- Uses POSIX `rename()` syscall — atomic on the same filesystem.
- **Complexity**: O(1)

#### Create File (`:create_file <name>`)
- Uses `std::ofstream` to create an empty file.
- **Complexity**: O(1)

#### Create Directory (`:create_dir <name>`)
- Uses POSIX `mkdir()` syscall with permissions `0777`.
- **Complexity**: O(1)

### 6.5 Selection System

The selection state is a `std::unordered_set<std::string>` of absolute file paths :

```cpp
struct SelectionState {
    std::vector<std::string> clipboard;
    std::unordered_set<std::string> selected_files; // snake_case
};
```

| Key | Action |
| :--- | :--- |
| `Space` | Toggle current file in/out of `selectedFiles` |
| `u` | Clear all selections (`selectedFiles.clear()`) |
| `c` / `d` | Operate on all `selectedFiles` |

**Toggle complexity**: O(1) average (hash set insert/erase).

### 6.6 Command Mode

Press `:` to enter command mode. The input loop reads a line of text and calls `processCommand(commandLine)` in `src/core/command_processor.cpp`.

The processor splits the input into tokens and dispatches:

```cpp
std::vector<std::string> args;
std::stringstream ss(commandLine);
while (ss >> word) args.push_back(word);

std::string command = args[0];
```

**Full Command Reference:**

| Command | Example | Description |
| :--- | :--- | :--- |
| `:rename <new>` | `:rename report_v2.md` | Rename the file under cursor |
| `:create_file <n>` | `:create_file todo.txt` | Create a new empty file |
| `:create_dir <n>` | `:create_dir projects` | Create a new directory |
| `:cd <path>` | `:cd /Users/me/docs` | Jump to an absolute path (supports spaces) |
| `:search <q>` | `:search readme` | Recursive filename search |
| `:search --file <q>` | `:search --file config` | Search files only |
| `:search --dir <q>` | `:search --dir test` | Search directories only |
| `:find <tokens>` | `:find async lambda` | Content search via LMDB index (AND) |
| `:find --dir <tokens>` | `:find --dir python script` | Content search, filtered to current dir |
| `:help` | `:help` | Show in-app help |
| `:q` | `:q` | Exit command mode |
| `:exit` | `:exit` | Quit the application |

> [!NOTE]
> `:cd` supports paths with spaces because the parser joins all tokens after `cd` with a space: `for (int i = 2; i < args.size(); i++) absPath += " " + args[i];`

### 6.7 Filename Search (`:search`)

**File**: `src/core/search_engine.cpp`, `src/core/commands.cpp:searchCommand`

This is a **recursive directory crawl** starting from `app.nav.currPath`. It does **not** use the index.

**Process:**
1. Lowercase the query: `transform(..., ::tolower)`
2. Call `searchAnything(path, filename, check_file, check_dir)` which uses `fs::recursive_directory_iterator`.
3. Time the search and log it.
4. Display results with `displaySearchResults()`.

**Flags:**
- `:search <q>` → searches both files and directories
- `:search --file <q>` → `check_dir = false`
- `:search --dir <q>` → `check_file = false`

**Complexity**: **O(N)** where N = total files and directories under current path (linear scan).

### 6.8 Content Search (`:find`)

**File**: `src/index/inverted_index.cpp:search()`

This uses the **persistent LMDB inverted index** for fast multi-word AND semantic content search.

**Step-by-step process:**

**Step 1 — Tokenize the query:**
```cpp
// Query: "async lambda move"
// After normalizeWord():  tokens = ["async", "lambda", "move"]
```

**Step 2 — Open a read-only LMDB transaction**

**Step 3 — For each token, look up its ID and collect matching inodes:**
```
db_word2id["async"] → word_id = 42
db_inverted[42]     → {ino=1001, ino=2040, ino=5500}  // files containing "async"

db_word2id["lambda"] → word_id = 71
db_inverted[71]      → {ino=2040, ino=5500}  // files containing "lambda"
```

A `fileCounter[ino]++` map counts how many query words each inode matches.

**Step 4 — Intersect results (AND semantics):**
```cpp
int required = tokens.size(); // 2
for (auto& [ino, cnt] : fileCounter)
    if (cnt == required)  // only files matching ALL words
        resolve_path(ino);
```

**Step 5 — Resolve inode → path (macOS):**
```cpp
// macOS-specific volfs path resolution
std::string volPath = "/.vol/" + rootDev + "/" + ino;
fcntl(fd, F_GETPATH, pathBuf);  // kernel resolves inode to absolute path
```

**Complexity Summary:**

| Operation | Complexity | Note |
| :--- | :--- | :--- |
| Word ID lookup | **O(log N)** | LMDB B+ tree lookup |
| Inode set retrieval | **O(K)** | K = number of files with this word |
| Inode intersection | **O(W × K)** | W = words in query, K = avg posting list size |
| Path resolution | **O(R)** | R = matching results |
| **Total search** | **≈ O(W × K)** | Sub-millisecond for typical queries |

### 6.9 Real-Time Filesystem Watcher

The explorer tracks filesystem changes so the index stays fresh when you add, delete, or rename files.

**Platform-specific implementation:**

| Platform | API Used | File |
| :--- | :--- | :--- |
| **macOS** | `FSEvents` (Apple CoreServices) | `watcher_mac.cpp` |
| **Linux** | `inotify` | `watcher_linux.cpp` |

**Why not `kqueue` on macOS?**  
`kqueue` requires one open file descriptor per watched directory. For large trees (100k+ directories), this hits OS FD limits instantly. `FSEvents` monitors entire directory subtrees with a single handle.

**macOS `FSEvents` Flow:**
1. `FSEventStreamCreate()` — registers callback for the `indexingRoot`.
2. Runs on a dedicated `CFRunLoop` thread.
3. Events are pushed into `app.indexing.eventQueue` (mutex-protected).
4. Background worker thread consumes events and calls `index.indexPath()` or `index.removePath()`.

**Event Types:**

| Flag | Mapped To |
| :--- | :--- |
| `kFSEventStreamEventFlagItemCreated` | `WatcherEventType::CREATE` |
| `kFSEventStreamEventFlagItemRemoved` | `WatcherEventType::DELETE` |
| `kFSEventStreamEventFlagItemRenamed` | `WatcherEventType::RENAME` |
| `kFSEventStreamEventFlagItemModified` | `WatcherEventType::MODIFY` |

---

## 7. Indexing Architecture Deep Dive

### LMDB Database Schema

The index is stored in a single LMDB environment at `~/.cache/refined-explorer/lmdb/` with **5 named sub-databases** 

```
db_files     :  ino    (uint64) → mtime   (uint64)    [unique, INTEGERKEY]
db_word_2_id :  word   (str)    → word_id (uint32)    [unique]               ← was "db_word2id"
db_id_2_word :  word_id(uint32) → word    (str)       [unique, INTEGERKEY]   ← was "db_id2word"
db_inverted  :  word_id(uint32) →+ ino   (uint64)    [DUPSORT | INTEGERKEY | DUPFIXED]
db_forward   :  ino    (uint64) →+ word_id(uint32)   [DUPSORT | INTEGERKEY | DUPFIXED]
```

The `→+` notation means one key maps to **multiple sorted values** (`DUPSORT`). The LMDB on-disk **named database keys** are `"files"`, `"word2id"`, `"id2word"`, `"inverted"`, `"forward"` — the C++ member variable names use underscores (`db_word_2_id`), but the LMDB string names do not.

### Why Inodes Instead of Paths?

Storing full path strings for every file was the main source of index bloat. Instead, we store **inodes** (the OS-assigned unique file ID):

- Each inode is a `uint64_t` (8 bytes) vs a full path string (50-200+ bytes).
- Inodes survive renames — if you rename a file, its inode doesn't change.
- On macOS, inodes are resolved to paths at query time using `/.vol/<dev>/<ino>` and `fcntl(F_GETPATH)`.

### Word Normalization Pipeline

Every token from a filename or file content goes through `normalizeWord()`:

```
Input: "Hello_World!"
  ↓ character filter (keep alnum + [@#_-$&])
  "Hello_World"   ← '!' is rejected → entire word rejected
```

Wait, the filter is stricter: if **any** character is disallowed, the **entire word is rejected**:

```cpp
for (char c : word) {
    if (!(isalnum(c) || c == '@' || ... ))
        return "";  // reject entire word
}
```

Then:
- Lowercased: `"hello_world"`
- Stopword check: removed if in the 100+ word stopword list
- Length limit: rejected if ≥ 128 characters

### Differential Indexing (Startup Crawl)

On every launch, the indexer (in `src/core/system.cpp:runIndexingInBackground`) does a **differential crawl** instead of a full re-index:

1. `getLastSyncTime()` reads the last indexed timestamp from `db_files[ino=0]` (a sentinel entry using inode 0, which is reserved and can never collide).
2. During traversal via `fs::recursive_directory_iterator`, each file's `mtime` is compared to the last sync timestamp.
3. Only **new or modified files** are re-indexed (using `removePath` + `indexPath` for updates).
4. `setLastSyncTime(now)` updates the sentinel after the crawl.
5. The following directories are **automatically skipped** during crawl: `.git`, `.svn`, `.hg`, `node_modules`, `build`, `dist`, `.env`, `env`, `venv`, `.venv`, `bin`, `obj`, `.idea`, `.vscode`.

**Note**: The recursive crawl is implemented directly in `system.cpp:runIndexingInBackground()`.

### updatePath() — True Differential

Beyond `indexPath`/`removePath`, there is now an `updatePath()` method in `inverted_index.cpp` that performs a **true diff** (set subtraction) when a file is modified:

1. Re-tokenizes the updated file content → `current_word_ids` (a `std::set<uint32_t>`).
2. Reads all existing word IDs for this inode from `db_forward` → `old_word_ids`.
3. **Removes** word IDs in `old_word_ids` but not in `current_word_ids` (words removed from file).
4. **Adds** word IDs in `current_word_ids` but not in `old_word_ids` (new words in file).

This is more efficient than a full `removePath` + `indexPath` cycle because only the _delta_ is written to LMDB. The watcher thread uses `updatePath()` for `CREATE` and `MODIFY` events.

This makes subsequent startups very fast — only changed files are processed.

---

## 8. Complexity Analysis

### Summary Table

| Feature | Time Complexity | Space Complexity | Notes |
| :--- | :--- | :--- | :--- |
| Directory listing (cache hit) | **O(1)** | O(N) cached | N = files in dir |
| Directory listing (cache miss) | **O(N log N)** | O(N) | N = files in dir (sort) |
| Cursor move Up/Down | **O(1)** | O(1) | Simple counter update |
| `:cd` navigate absolute | **O(P)** | O(P) | P = path segments |
| `:search` (filename) | **O(N)** | O(R) | N = all files in subtree |
| `:find` (content, LMDB) | **O(W × K)** | O(R) | W = words, K = avg posting size |
| Word ID lookup (LMDB) | **O(log V)** | — | V = vocab size, B+ tree |
| Add file to index | **O(T × log V)** | O(T) | T = tokens in file |
| Remove file from index | **O(T × log V)** | — | Scans forward index |
| Copy N files | **O(F)** | — | F = total bytes |
| Delete N files | **O(F)** | — | Recursive remove |
| Toggle selection | **O(1)** avg | — | Hash set |
| Binary file detection | **O(1)** | — | Reads first 512 bytes only |

### Search Performance vs grep

```
Query: "async lambda" (in a project with 50,000 files)

grep -r "async" . | grep "lambda"   →  ~400ms   (reads every file)
:find async lambda                  →  ~8ms     (LMDB O(log N) lookup)
```

---

## 9. Testing

The project uses **GoogleTest** (v1.14.0) for automated unit testing.

### Running Tests

```bash
cd build
./tests_runner
# or with JSON output:
./tests_runner --gtest_output=json:test_results.json
```

Tests also run automatically after every `cmake --build .` via the `POST_BUILD` CMake hook:

```cmake
add_custom_command(TARGET tests_runner POST_BUILD
    COMMAND tests_runner --gtest_output=json:test_results.json
    COMMAND ${CMAKE_COMMAND} -E remove_directory "${CMAKE_SOURCE_DIR}/tests/dummy"
    COMMENT "Running automated tests with JSON output and cleaning up dummy folder..."
)
```

### Test Coverage (110 Cases, 10 Modules)

| Module | File | Cases | What's Tested |
| :--- | :--- | :--- | :--- |
| **TextUtils** | `test_utils.cpp` | 24 | `normalizeWord`: symbols, stopwords, case, length limits |
| **FormatUtils** | `test_utils.cpp` | 13 | `humanReadableSize` (B→TB), `truncateStr` edge cases (note: `truncateStr` pads short strings) |
| **FileUtils** | `test_file_utils.cpp` | 12 | `isReadable`, `isBinaryFile`, `isDirectory`, permissions |
| **Navigation** | `test_navigation.cpp` | 13 | `normalizeRange`, `scrollToIndex`, `isUnderCurrentDir` |
| **DirCache** | `test_dir_cache.cpp` | 6 | Cache hit/miss, `invalidateCache`, sorted order |
| **Commands** | `test_commands.cpp` | 15 | `cd` with spaces, create/rename/delete, paste collision |
| **Search** | `test_search.cpp` | 6 | Partial match, case insensitive, `--file`/`--dir` flags |
| **InvertedIndex** | `test_index.cpp` | 12 | LMDB open/close, multi-word AND, persistence, word limits, `updatePath` diffing |
| **Config** | `test_config.cpp` | 3 | Default values, toggle indexing, worker count |
| **Selection** | `test_selection.cpp` | 6 | Single/multi select, clear, clipboard update |
| **TOTAL** | | **110** | **100% Pass Rate ✅** |

### Sandbox Directory

Tests use an isolated `tests/dummy/` directory created and filled at test start. This directory is **automatically deleted** after tests complete (via the CMake `POST_BUILD` hook), so it never pollutes the workspace.

---

## 10. Project Structure

```
refined-explorer/
├── CMakeLists.txt          # Build system: core_lib, refined_explorer, tests_runner
├── config.json             # Runtime configuration (JSON format, NOT YAML)
├── main.cpp                # Entry point: loadConfig → startIndexing → initializeNavigation → navigate
├── include/
│   ├── myheader.hpp        # Single project-wide header: all structs, enums, extern declarations
│   └── ansi_codes.hpp      # ANSI escape code constants (included by myheader.hpp)
├── src/
│   ├── core/               # Application logic
│   │   ├── command_processor.cpp  # Parses & dispatches : commands
│   │   ├── commands.cpp           # copy, paste, delete, rename, search wrappers
│   │   ├── dir_cache.cpp          # Directory listing cache (unordered_map)
│   │   ├── file_details.cpp       # getFolderSizeMT, getFileDetails, startFolderSizeWorker
│   │   ├── file_utils.cpp         # isDirectory, isRegularFile, isReadable, isBinaryFile
│   │   ├── navigation.cpp         # Key event loop, scroll logic, enter/back
│   │   ├── navigation_init.cpp    # Builds back_stack from startup path
│   │   ├── navigator.cpp          # normalizeRange, scrollToIndex, navigateToAbsolutePath
│   │   ├── search_engine.cpp      # :search recursive crawl
│   │   └── system.cpp             # handleResizeIfNeeded, runIndexingInBackground, signal logic
│   ├── index/              # Indexing engine
│   │   ├── inverted_index.cpp     # LMDB: open/close/indexPath/updatePath/removePath/search
│   │   ├── index_runner.cpp       # startIndexing: opens LMDB, starts watcher + worker
│   │   ├── watcher_mac.cpp        # FSEvents watcher (macOS)
│   │   └── watcher_linux.cpp      # inotify watcher (Linux)
│   │   # NOTE: traversal.cpp does NOT exist. Crawl logic is in system.cpp.
│   ├── tui/                # Terminal UI rendering (10 files)
│   │   ├── command_ui.cpp         # Command mode input prompt rendering
│   │   ├── details_ui.cpp         # Left panel: file name, size, ownership, permissions
│   │   ├── help_ui.cpp            # In-app help screen (:help)
│   │   ├── input.cpp              # Raw terminal input (getInput)
│   │   ├── panels.cpp             # Middle/right panel rendering
│   │   ├── renderer.cpp           # renderUI() top-level compositor
│   │   ├── search_ui.cpp          # Search results display & jump-to-result
│   │   ├── signals.cpp            # SIGWINCH / SIGINT handler setup
│   │   ├── status.cpp             # Status bar / message display
│   │   └── terminal.cpp           # Terminal init (raw mode, alternate screen)
│   └── utils/              # Shared utilities
│       ├── config.cpp             # loadConfig() — JSON parsing via nlohmann/json
│       ├── format.cpp             # humanReadableSize, truncateStr
│       ├── logger.cpp             # logMessage() — writes to logs/debug.log
│       └── text_utils.cpp         # normalizeWord, STOPWORDS set
├── tests/
│   ├── test_main.cpp       # GTest environment setup
│   ├── stubs.cpp           # Stubs for functions requiring a live terminal
│   ├── test_commands.cpp
│   ├── test_config.cpp
│   ├── test_dir_cache.cpp
│   ├── test_file_utils.cpp
│   ├── test_index.cpp
│   ├── test_navigation.cpp
│   ├── test_search.cpp
│   ├── test_selection.cpp
│   └── test_utils.cpp
├── build/                  # CMake build artifacts (generated, not committed)
│   ├── refined_explorer    # Main binary
│   ├── tests_runner        # Test binary
│   └── test_results.json   # Latest test report
├── logs/
│   └── debug.log           # Runtime log output from logMessage()
├── README.md               # Quick-start guide
├── DOCUMENTATION.md        # This file
```

---

## 11. Troubleshooting & FAQs

### "LMDB: cannot open environment"
- Ensure `~/.cache/refined-explorer/` is writable.
- Run: `mkdir -p ~/.cache/refined-explorer/lmdb`

### `:find` returns no results
- Check that `"indexing": true` is set in `config.json` (JSON format, not YAML).
- Check `logs/debug.log` for "LMDB Inode-Index opened" — if absent, LMDB failed to open.
- Wait ~5-30 seconds after launch for the initial crawl to complete.
- Try `:search` as a fallback — it doesn't need the index.

### "Binary File" shown in preview
- The file's first 512 bytes contain a null byte or non-printable character.
- `isBinaryFile()` uses this heuristic. This is expected for images, `.pdf`, compiled binaries, etc.

### Build fails on macOS with CoreServices error
- Switch from Homebrew GCC to Apple Clang: `CC=clang CXX=clang++ cmake ..`

### `refined_explorer` binary was accidentally deleted
- Simply rebuild: `cd build && cmake .. && cmake --build . -j8`

### Why is the `build/` folder ~38MB?
| What | Size | Needed? |
| :--- | :--- | :--- |
| `_deps/` (GoogleTest source) | ~23 MB | Only for re-building tests |
| `CMakeFiles/` (object files) | ~7.6 MB | Only for incremental builds |
| `lib/` (compiled static libs) | ~2.2 MB | Only for linking |
| Binaries (`refined_explorer`, `tests_runner`) | ~2.5 MB | **Yes, to run** |

You can safely delete everything except the binaries and `test_results.json` if you're not doing active development.

---

## 12. Future Roadmap

### 🔥 High Priority

#### SIMD-Accelerated Set Intersection
Instead of iterating through posting lists with a hash counter, use SIMD (AVX2/NEON) instructions to do bitwise AND across compressed inode bitsets — targeting **sub-millisecond** query latency for queries spanning millions of files.

#### PDF / DOCX Format Support
Integrate `Poppler` (PDF) or a lightweight XML parser (DOCX is ZIP+XML) to extract text content and tokenize it for indexing. This turns the explorer into a **universal document search engine**.

#### Delta Inode Encoding
Instead of storing raw 8-byte inodes, store the **delta between consecutive sorted inodes**:
```
Full:  [1001, 1006, 1015, 1100]
Delta: [1001,    5,    9,   85]
```
Small deltas → better compression → smaller index. Especially effective when inodes are clustered.

### 🧠 Future Vision

#### Trie-Based Prefix Search (Autocomplete)
Build a **Trie** over the word dictionary:
```
         root
          ├── a
          │   └── s
          │       └── y
          │           └── n
          │               └── c  ← word_id=42 (isEnd=true)
          └── l
              └── ...
```
A prefix query like `"asy"` traverses the trie and collects all `word_id`s below that node. This enables instant **search-as-you-type** autocomplete in command mode.

#### Cross-Volume Indexing
Currently limited to a single `indexingRoot`. Future: support multiple roots with separate LMDB environments or a partition table.

---
