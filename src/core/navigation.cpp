#include "myheader.hpp"
#include <functional>
#include <unordered_map>

struct KeyHandler {
  std::function<void()> action;
  bool interrupt_scan = false;
};

static std::unordered_map<KeyAction, KeyHandler> action_map;

termios initial_rsettings, new_rsettings;

fs::path getSelectedPath() noexcept {
  const std::string &selected_file =
      app.nav.file_list[app.nav.x_curr + app.nav.up_screen - 1];

  return fs::path(app.nav.curr_path) / selected_file;
}

void handleUpAction() {
  if (app.nav.x_curr > 1) {
    app.nav.x_curr--;
  } else if (app.nav.up_screen > 0) {
    app.nav.up_screen--;
    app.nav.down_screen++;
  }
}

void handleDownAction() {
  if (app.nav.x_curr < app.layout.row_size &&
      app.nav.x_curr < (int)app.nav.file_list.size()) {
    app.nav.x_curr++;
  } else if (app.nav.x_curr == app.layout.row_size && app.nav.down_screen > 0) {
    app.nav.up_screen++;
    app.nav.down_screen--;
  }
}

void enterDirectory(const std::string &new_path) {
  NavState current_state;
  current_state.path = app.nav.curr_path;
  current_state.x_curr = app.nav.x_curr;
  current_state.up_screen = app.nav.up_screen;

  app.nav.back_stack.push(current_state);

  app.nav.curr_path = new_path;

  openDirectory(app.nav.curr_path, app.nav.up_screen, app.nav.down_screen);

  app.nav.x_curr = 1;
}

void handleBackAction() {
  if (!app.nav.back_stack.empty()) {
    NavState prev = app.nav.back_stack.top();
    app.nav.back_stack.pop();

    app.nav.curr_path = prev.path;
    openCurrDirectory(app.nav.curr_path);

    app.nav.x_curr = prev.x_curr;
    app.nav.up_screen = prev.up_screen;
    app.nav.down_screen =
        (int)app.nav.file_list.size() - app.nav.up_screen - app.layout.row_size;
  }
}

void openFile(const std::string &path) noexcept {
#ifdef _WIN32
  std::string cmd = "start \"\" \"" + path + "\"";
#elif __APPLE__
  std::string cmd = "open \"" + path + "\"";
#else
  std::string cmd = "xdg-open \"" + path + "\"";
#endif

  std::system(cmd.c_str()); // simple and portable
}

void handleEnterAction() {
  fs::path new_path = getSelectedPath();

  if (isDirectory(new_path)) {
    openCurrDirectory(new_path.c_str());

    if (!app.nav.file_list.empty()) {
      enterDirectory(new_path);
    }
  } else if (isRegularFile(new_path)) {
    openFile(new_path);
  }
}

void handleRightAction() {
  app.selection.selected_files.clear();

  fs::path new_path = getSelectedPath();
  if (isDirectory(new_path)) {
    openCurrDirectory(new_path.string());
    enterDirectory(new_path);
  }
}

void handlePasteAction() {
  std::string pasted = paste();
  refreshCurrentDirectory();
  if (!pasted.empty())
    updatePosition(pasted);
}

void handleDeleteAction() {
  clearScreen();
  setCursorPos(1, 1);
  std::cout << "Are you sure you want to delete selected items? (y/n): "
            << std::flush;
  char confirm = std::cin.get();
  if (confirm == 'y' || confirm == 'Y') {
    deleteSelectedItems();
    refreshCurrentDirectory();
  }
}

void handleCommandAction() {
  renderUI();
  commandMode();
}

void handleClearSelectionAction() { app.selection.selected_files.clear(); }

void initActionMap() {
  action_map[KeyAction::UP] = {handleUpAction, true};
  action_map[KeyAction::DOWN] = {handleDownAction, true};
  action_map[KeyAction::RIGHT] = {handleRightAction, true};
  action_map[KeyAction::LEFT] = {handleBackAction, true};
  action_map[KeyAction::COPY] = {handleCopyAction, false};
  action_map[KeyAction::PASTE] = {handlePasteAction, true};
  action_map[KeyAction::DELETE] = {handleDeleteAction, true};
  action_map[KeyAction::COMMAND] = {handleCommandAction, false};
  action_map[KeyAction::TOGGLE_SELECT] = {handleToggleSelectAction, false};
  action_map[KeyAction::CLEAR_SELECTION] = {handleClearSelectionAction, false};
  action_map[KeyAction::BACK] = {handleBackAction, true};
  action_map[KeyAction::ENTER] = {handleEnterAction, true};
}

// Map raw input char to KeyAction
KeyAction mapKey(char ch, bool is_esc_sequence = false) {
  if (is_esc_sequence) { // Arrow keys after ESC + '['
    switch (ch) {
    case 'A':
      return KeyAction::UP;
    case 'B':
      return KeyAction::DOWN;
    case 'C':
      return KeyAction::RIGHT;
    case 'D':
      return KeyAction::LEFT;
    default:
      return KeyAction::UNKNOWN;
    }
  } else { // Normal keys
    switch (ch) {
    case 'c':
      return KeyAction::COPY;
    case 'p':
      return KeyAction::PASTE;
    case 'd':
      return KeyAction::DELETE;
    case ':':
      return KeyAction::COMMAND;
    case ' ':
      return KeyAction::TOGGLE_SELECT;
    case 'u':
      return KeyAction::CLEAR_SELECTION;
    case 127:
    case 8:
      return KeyAction::BACK;
    case '\n':
    case '\r':
      return KeyAction::ENTER;
    default:
      return KeyAction::UNKNOWN;
    }
  }
}

// Unified processKey function
void processKey(char ch) {
  KeyAction action = KeyAction::UNKNOWN;

  if (ch == 27) { // ESC sequence
    char next = std::cin.get();
    if (next == '[') {
      char arrow = std::cin.get();
      action = mapKey(arrow, true);
    }
  } else {
    action = mapKey(ch);
  }

  // Dispatch using the action_map with context-aware interruption
  auto it = action_map.find(action);
  if (it != action_map.end()) {
    if (it->second.interrupt_scan) {
      stopFolderScan();
    }
    it->second.action();
  }
}

void navigate() {

  app.nav.curr_path = app.nav.root;

  initActionMap();

  openCurrDirectory(app.nav.curr_path.c_str());

  scrollToIndex((int)app.nav.file_list.size() - 1, (int)app.nav.file_list.size(),
                app.layout.row_size, app.nav.up_screen, app.nav.down_screen,
                app.nav.x_curr);

  renderUI();
  setDefaultCursorPos();

  char ch;

  tcgetattr(fileno(stdin), &initial_rsettings);
  new_rsettings = initial_rsettings;

  new_rsettings.c_lflag &= ~(ICANON | ECHO);
  new_rsettings.c_lflag |= ECHOE;

  tcsetattr(fileno(stdin), TCSANOW, &new_rsettings);

  while (true) {

    handleResizeIfNeeded();

    if (app.ui.refresh) {
      renderLeftPanel();
      app.ui.refresh = false;
    }

    if (inputAvailable()) {
      ch = std::cin.get();

      processKey(ch);

      renderUI();
      setDefaultCursorPos();
    }
  }
}