
# 🗂️ Terminal File Explorer

A **high-performance, terminal-based file explorer** written in **C++**, combining a Yazi-like navigation experience with a **custom-built forward & inverted indexing engine** for fast file and content search — **without heavyweight dependencies like Elasticsearch**.

---

## 📋 Table of Contents

- [Demo](#demo)
- [Features](#features)
- [System Calls Used](#system-calls-used)
- [How to Use](#how-to-use)
- [Indexing Architecture](#indexing-architecture)
- [Configuration](#configuration)
- [Performance Benchmarks](#performance-benchmarks)
- [Tested On](#tested-on)

---

## 🎬 Demo

> **Screenshots / GIFs**

```
[ Navigation ]
↑ ↓ → ←   |  Green = directories, White = files
[ Search ]
:find multi word query
[ Selection ]
Space → select | c → copy | p → paste | d → delete
```

*Add GIFs here (recommended):*
- Navigation & scrolling
- Multi-file selection
- `find` content search
- Command mode operations

---

## ✨ Features

### 🎯 Core System Features

- **Config-driven architecture** - Controls worker count for multi-threaded operations
- **Terminal resize detection** - Automatically detects window size changes using `ioctl`
- **Automatic UI redraw** - Refreshes display on window resize
- **Single-pass indexing** - Builds forward + inverted index at startup
- **Efficient ID-based indexing** - Uses integer IDs for files & words (memory efficient)
- **Directory caching** - Caches directory listings for faster navigation
- **Transaction-based operations** - Copy, paste, delete operations with rollback support

### 📁 File Navigation

- **Start from `/home`** - Begins traversal from home directory
- **Backstack navigation** - Maintains full path history from root → current directory
- **Arrow key navigation**:
  - **↑ / ↓** : Move cursor up/down
  - **→** : Enter directory (or open file with default app)
  - **←** : Go to parent directory
  - **Backspace** : Go back (same as ←)
- **Color coding**:
  - Directories shown in **green**
  - Files shown in **white**
  - Selected items highlighted with **reverse video**
- **File preview** - Shows file details and text content preview in side panel
- **Directory preview** - Shows directory contents in side panel

### 🎯 Selection System

- **Space** → Toggle selection of current file/directory
- **Multi-file selection** - Select multiple files/directories
- **Selected items highlighted** - Visual feedback with reverse video
- **u** → Clear all selections

### 📋 File Operations

| Key     | Action                        | Description                                    |
| ------- | ----------------------------- | ---------------------------------------------- |
| `c`     | Copy selected/current files   | Copies to clipboard (supports multi-select)   |
| `p`     | Paste copied files            | Pastes from clipboard to current directory     |
| `d`     | Delete selected/current files | Moves to trash then permanently deletes        |
| `Enter` | Open regular file             | Opens file with system default application     |

**Note**: All file operations (copy, paste, delete) support transaction-based rollback on failure.

### 🎮 Command Mode (`:`)

Enter command mode by pressing `:` key. All commands are executed in command mode.

#### File & Directory Commands

```text
:rename <new_name>
```
- Renames the currently selected file or directory
- Updates the index automatically
- Example: `:rename myfile_v2.txt`

```text
:create_file <name>
```
- Creates a new empty file in the current directory
- Example: `:create_file notes.txt`

```text
:create_dir <name>
```
- Creates a new directory in the current directory
- Example: `:create_dir projects`

```text
:cd <absolute_path>
```
- Navigates to an absolute path
- Rebuilds navigation stack automatically
- Example: `:cd /home/user/Documents`

#### Search Commands

##### Filename Search

```text
:search <name>
```
- Searches for files and directories matching the name (case-insensitive)
- Recursively searches from current directory
- Example: `:search readme`

```text
:search --file <name>
```
- Searches only files (excludes directories)
- Example: `:search --file config`

```text
:search --dir <name>
```
- Searches only directories (excludes files)
- Example: `:search --dir test`

##### Content Search (Indexed)

```text
:find <word1> <word2> ...
```
- Multi-word AND search using inverted index
- Searches file contents (not just filenames)
- Fast lookup using word IDs
- Example: `:find hello world` (finds files containing both "hello" AND "world")

```text
:find --dir <words>
```
- Same as `:find` but filters results to current directory only
- Example: `:find --dir python script`

**Features**:
- ✔ Multi-word AND search (all words must be present)
- ✔ Uses inverted index for O(1) word lookup
- ✔ Fast lookup using word IDs
- ✔ Stopword filtering (ignores common words like "the", "a", "is")
- ✔ Case-insensitive search
- ✔ Normalizes words (removes punctuation)

#### Help & Exit

```text
:help
:--help
```
- Displays comprehensive help menu with all available commands

```text
:q
```
- Exits command mode and returns to file explorer

### 📊 File Details Panel

When a file or directory is selected, the left panel displays:

- **File Name** - Name of the selected item
- **File Size** - Human-readable size (B, KB, MB, GB, TB)
  - For directories: Shows total size of all contents (calculated with multi-threading)
- **Ownership** - User and group names
- **Permissions** - Unix-style permissions (rwxrwxrwx)
- **Last Modified** - Date and time of last modification
- **Scan Time** - (For directories) Time taken to calculate size

### 📄 File Preview

- **Text files**: Displays file content in the right panel (truncated to fit)
- **Binary files**: Shows "Binary File" message
- **No permission**: Shows "No Read Permission" message
- **Directories**: Shows directory contents in the right panel

---

## 🔧 System Calls Used

This application uses the following POSIX system calls and library functions:

### File System Operations

| System Call | Purpose | Usage Location |
|------------|---------|---------------|
| `stat()` | Get file status (type, size, permissions, timestamps) | File type checking, file details, indexing |
| `lstat()` | Get file status without following symlinks | Multi-threaded folder size calculation |
| `open()` | Open file descriptor | Opening `/dev/null` for error suppression |
| `close()` | Close file descriptor | Cleanup after file operations |
| `read()` | Read from file descriptor | Reading file contents (via ifstream) |
| `write()` | Write to file descriptor | Writing to files (via ofstream) |
| `access()` | Check file permissions | Checking if file is readable |
| `mkdir()` | Create directory | Creating directories and trash bins |
| `rename()` | Rename/move file | File renaming and moving to trash |
| `remove()` | Delete file | Cleanup operations (uses `unlink()` internally) |

### Directory Operations

| System Call | Purpose | Usage Location |
|------------|---------|---------------|
| `opendir()` | Open directory stream | Directory traversal, indexing, search |
| `readdir()` | Read directory entry | Listing files, recursive search |
| `closedir()` | Close directory stream | Cleanup after directory operations |

### Process & Execution

| System Call | Purpose | Usage Location |
|------------|---------|---------------|
| `fork()` | Create child process | Opening files with default application |
| `execlp()` | Execute program | Launching `xdg-open` to open files |
| `getpid()` | Get process ID | Creating unique trash directories, logging |
| `system()` | Execute shell command | Copy operations, delete operations |

### Terminal & I/O Control

| System Call | Purpose | Usage Location |
|------------|---------|---------------|
| `ioctl()` | I/O control operations | Getting terminal window size (`TIOCGWINSZ`) |
| `tcgetattr()` | Get terminal attributes | Setting up raw input mode |
| `tcsetattr()` | Set terminal attributes | Enabling/disabling canonical mode, echo |
| `fileno()` | Get file descriptor | Converting FILE* to file descriptor |

### Signal Handling

| System Call | Purpose | Usage Location |
|------------|---------|---------------|
| `signal()` | Register signal handler | Handling `SIGINT` (Ctrl+C) and `SIGWINCH` (window resize) |

### File Descriptor Operations

| System Call | Purpose | Usage Location |
|------------|---------|---------------|
| `dup2()` | Duplicate file descriptor | Redirecting stderr to `/dev/null` |

### Path & Environment

| System Call | Purpose | Usage Location |
|------------|---------|---------------|
| `getcwd()` | Get current working directory | Initializing root directory path |

### User & Group Information

| Library Function | Purpose | Usage Location |
|-----------------|---------|---------------|
| `getpwuid()` | Get user info by UID | Displaying file ownership |
| `getgrgid()` | Get group info by GID | Displaying file group |

**Note**: `getpwuid()` and `getgrgid()` are library functions that internally use system calls to query `/etc/passwd` and `/etc/group`.

---

## 📖 How to Use

### Starting the Application

1. **Build the project**:
   ```bash
   make clean
   make
   ```

2. **Run the application**:
   ```bash
   ./bin/main
   ```
   Or with a starting directory:
   ```bash
   ./bin/main /path/to/directory
   ```

3. **Initial indexing**: On first run, the application will index all files under `/home` directory. This may take a few seconds depending on the number of files.

### Navigation

1. **Move cursor**: Use ↑ and ↓ arrow keys
2. **Enter directory**: Press → or Enter on a directory
3. **Go back**: Press ← or Backspace
4. **Scroll**: When there are more files than visible, the list scrolls automatically

### Selecting Files

1. **Select single file**: Move cursor to file and press `Space`
2. **Select multiple files**: Press `Space` on each file you want to select
3. **Clear selections**: Press `u` to deselect all

### File Operations

1. **Copy files**:
   - Select files (or just position cursor on a file)
   - Press `c`
   - Confirmation message shows number of items copied

2. **Paste files**:
   - Navigate to destination directory
   - Press `p`
   - Files are copied to current directory

3. **Delete files**:
   - Select files (or position cursor on a file)
   - Press `d`
   - Confirm with `y` or cancel with `n`
   - Files are moved to trash and permanently deleted

4. **Open file**:
   - Position cursor on a file
   - Press `Enter`
   - File opens with system default application

### Using Command Mode

1. **Enter command mode**: Press `:`
2. **Type command**: Enter any of the commands listed above
3. **Execute**: Press Enter
4. **Exit command mode**: Press `:q` or press Escape

### Search Examples

**Filename search**:
```
:search readme
:search --file config
:search --dir test
```

**Content search**:
```
:find python script
:find hello world
:find --dir database query
```

### Tips

- **Fast navigation**: Use `:cd` to jump to any directory quickly
- **Quick search**: Use `:find` for fast content search (uses pre-built index)
- **File preview**: Position cursor on any file to see its details and content
- **Multi-select**: Select multiple files before copy/delete operations
- **Window resize**: Application automatically adjusts to terminal size changes

---

## 🏗️ Indexing Architecture

### Data Structures

```text
File IDs:
fileToId   : path → file_id
idToFile   : file_id → path

Word IDs:
wordToId   : word → word_id
idToWord   : word_id → word

Inverted Index:
word_id → {file_id}

Forward Index:
file_id → {word_id}
```

### Index Lifecycle

1. **Initial Build**: Index built once at startup by traversing `/home` directory
2. **Incremental Updates**: Index is updated (rectified) on:
   - File/Directory creation
   - File/Directory copy
   - File/Directory rename
   - File/Directory delete
3. **ID Reuse**: Deleted file IDs are reused for new files (memory efficient)

### Word Normalization

- Converts to lowercase
- Removes punctuation
- Filters stopwords (common words like "the", "a", "is", etc.)
- Only alphanumeric characters are indexed

### Search Algorithm

1. **Query Processing**: Normalize and tokenize search query
2. **Word ID Lookup**: Map words to word IDs (O(1) lookup)
3. **File Intersection**: Find files containing ALL query words (AND semantics)
4. **Result Display**: Show matching file paths

---

## 🏗️ Architecture Diagram

```text
┌──────────────┐
│   Terminal   │
│   UI Layer   │
└──────┬───────┘
       │
┌──────▼────────┐
│ Input Handler │
│ (Keys + Cmds) │
└──────┬────────┘
       │
┌──────▼────────────┐
│ File Operations   │
│ (cp, mv, rm, cd)  │
└──────┬────────────┘
       │
┌──────▼──────────────┐
│ Inverted Index Core │
│  - File IDs         │
│  - Word IDs         │
│  - Forward Index    │
│  - Inverted Index   │
└─────────────────────┘
```

---

## 🚀 Build Instructions

### Requirements

- **OS**: Linux (Ubuntu 20.04+ recommended)
- **Compiler**: g++ with C++17 support
- **Terminal**: POSIX-compliant terminal (supports ANSI escape codes)
- **Dependencies**: Standard C++ libraries only (no external dependencies)

## ⚙️ Configuration

Create a `config.yml` file in the project root:

```yaml
performance:
  workers: 5
```

Or use the simple format:

```yaml
workers: 5
```

**Configuration Options**:

- `workers`: Number of worker threads for multi-threaded operations (default: 4)
  - Used for: Directory size calculation, indexing operations
  - Recommended: 4-8 (depending on CPU cores)

**Note**: If `config.yml` is not found, the application uses default values (4 workers).

---

## 📊 Performance Benchmarks

### Search Comparison

| Method                | Avg Time     | Notes                          |
| --------------------- | ------------ | ------------------------------ |
| `grep -R`             | ~300–800 ms  | Linear search through files    |
| Custom Inverted Index | **~5–20 ms** | O(1) word lookup, set intersection |

### 💾 Memory

- Only stores **unique word IDs** (not full words in index)
- Uses `unordered_set` to avoid duplicates
- File ID reuse for deleted files
- No external database required
- Directory caching for faster navigation

### ⚡ Indexing Performance

- **Initial indexing**: ~1-5 seconds for typical `/home` directory (depends on file count)
- **Incremental updates**: < 1ms per file operation
- **Search latency**: < 20ms for typical queries

---

## ✅ Tested On

- **Ubuntu 20.04+**
- **Linux terminal** (GNOME Terminal, Konsole, etc.)
- **macOS** (with POSIX compatibility)

---

## 🐛 Troubleshooting

### Terminal Size Issues

If the display looks incorrect:
- Resize the terminal window (application auto-detects)
- Ensure terminal supports ANSI escape codes

### Permission Errors

If you see "No Read Permission":
- Check file permissions with `ls -l`
- Some system files may not be readable

### Search Not Working

If `:find` returns no results:
- Ensure files were indexed (check logs/debug.log)
- Try `:search` for filename search instead
- Content search only works for text files

### File Opening Issues

If files don't open:
- Ensure `xdg-open` is installed (Linux)
- Check file associations in your system

---