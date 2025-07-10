
# Enhanced Terminal File Explorer

A modern, fast, and efficient terminal-based file explorer written in **C++**, inspired by the **Yazi** file manager. This project provides core file navigation and management features and supports two interactive modes: **Normal Mode** and **Command Mode**, similar to `vim`.

---

##  Features

### Normal Mode (GUI)
- Interactive terminal interface to explore the filesystem
- Lists files with:
  - Name
  - Human-readable size (e.g., like `ls -lh`)
  - Owner and group permissions
  - Last modified timestamp
- Navigation:
  - **Arrow Keys**: Navigate files within the directory
  - **Enter**: Open file or enter directory
  - **Backspace**: Move to parent directory
  - **Left/Right Arrows**: Navigate backward/forward in history
  - **c**: Copy the selected file or directory
  - **p**: Paste the copied content into the current path
  - **d**: Delete the selected file or directory

### Command Mode (`:`)
Enter Command Mode by pressing `:`. Supports:
- Rename files or directories
- Create or delete files and directories
- Navigate to an absolute path
- Recursive search (results shown in GUI; press `ESC` and `ENTER` to return)

To return to Normal Mode, type `:q`.

---

## Technologies & Dependencies

- **C++** 
- **ANSI Escape Codes** for terminal rendering
- **Non-canonical Mode** for real-time key handling
- **Makefile** for build automation

#### Key System Calls / Functions
| Function | Purpose |
|---------|---------|
| `opendir()`, `readdir()` | Directory traversal |
| `getpwuid()`, `getgrgid()` | File ownership info |
| `fork()` | Spawn child processes |
| `tcgetattr()`, `tcsetattr()` | Terminal input configuration |
| `signal()` | Handle interrupts gracefully |

---

## How to Build & Run

### Prerequisites
- Linux-based environment (native or via VM)
- `g++` and `make` installed

### Build
```bash
make clean && make
```

### Run
```bash
./main
```

To quit the explorer: Press `CTRL+C` and hit Enter.

---

## Command Examples (in Command Mode)

```bash
:rename new_name.txt           # Rename selected file
:create_file newfile.md        # Create a new file
:create_dir newdir             # Create a new directory
:cd /home/user/Documents       # Change to absolute path
:search --file filename        # Search for file recursively
:search --dir dirname          # Search for directory recursively
:search keyword                # General search
:q                             # Quit to Normal Mode
```

---

## How It Works — Step by Step

1. **Terminal Setup**: The terminal is set to non-canonical mode using `termios` to read keypresses in real-time. Echo is disabled for clean input handling.
2. **Rendering Engine**: Uses ANSI escape sequences to update the terminal dynamically and provide a seamless UI.
3. **Directory Listing**: Reads directory contents with `opendir()` and `readdir()` and fetches metadata for display.
4. **Navigation History**: Maintains a history of visited paths for back-and-forth navigation.
5. **Command Parsing**: Inputs in Command Mode are parsed and mapped to internal functions.
6. **Recursive Search**: Implements depth-first traversal for file/directory search.
7. **Clipboard Feature**: Supports copying and pasting files/directories across paths.
8. **Modular Design**: Cleanly structured for future extensions (e.g., compression, tagging, etc.).
9. **Error Handling**: Handles invalid inputs and system errors with helpful messages.

---