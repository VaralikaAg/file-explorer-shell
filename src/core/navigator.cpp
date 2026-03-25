#include "myheader.hpp"

void normalizeRange(int total, int row_size, int &up, int &down, int &x) {
  if (total == 0) {
    up = 0;
    down = 0;
    x = 1;
    return;
  }

  int max_up = (total > row_size) ? total - row_size : 0;
  if (up < 0)
    up = 0;
  if (up > max_up)
    up = max_up;

  int visible = total - up;
  int max_x = std::min(row_size, visible);
  if (x < 1)
    x = 1;
  if (x > max_x)
    x = max_x;

  down = std::max(0, total - up - row_size);
}

void scrollToIndex(int index, int total, int row_size, int &up, int &down,
                   int &x) {
  if (index < 0 || index >= total || total == 0)
    return;

  if (index < row_size) {
    up = 0;
    x = index + 1;
  } else {
    up = index - row_size + 1;
    x = row_size;
  }
  down = std::max(0, total - up - row_size);
}

void openDirectory(const std::string &path, int &up, int &down) {
  std::vector<std::string> files = getDirectoryFiles(path);
  app.layout.total_files = files.size();
  int dummy_x = 1;

  normalizeRange((int)files.size(), app.layout.row_size, up, down, dummy_x);
}

void openCurrDirectory(const std::string &path) {
  if ((int)app.cache.dir_cache.size() > app.cache.MAX_CACHE_ENTRIES) {
    app.cache.dir_cache.erase(app.cache.dir_cache.begin());
  }

  app.nav.file_list = getDirectoryFiles(path);
  app.layout.total_files = app.nav.file_list.size();
}

void normalizeCursor() {
  normalizeRange((int)app.nav.file_list.size(), app.layout.row_size,
                 app.nav.up_screen, app.nav.down_screen, app.nav.x_curr);
}

void updatePosition(const std::string &file_name) {
  int idx = -1;
  for (size_t i = 0; i < app.nav.file_list.size(); i++) {
    if (app.nav.file_list[i] == file_name) {
      idx = i;
      break;
    }
  }
  if (idx != -1) {
    scrollToIndex(idx, (int)app.nav.file_list.size(), app.layout.row_size,
                  app.nav.up_screen, app.nav.down_screen, app.nav.x_curr);
  }
}

void handleToggleSelectAction() {
  std::string file = app.nav.file_list[app.nav.x_curr + app.nav.up_screen - 1];
  std::string full_path = app.nav.curr_path + "/" + file;

  if (app.selection.selected_files.count(full_path))
    app.selection.selected_files.erase(full_path);
  else
    app.selection.selected_files.insert(full_path);
}

bool isUnderCurrentDir(const std::string &path) {
  std::string base(app.nav.curr_path);
  if (base.back() != '/')
    base += '/';
  return path.rfind(base, 0) == 0;
}

void navigateToAbsolutePath(const std::string &abs_path) {
  if (abs_path.empty() || abs_path[0] != '/' || !fs::exists(abs_path) ||
      !fs::is_directory(abs_path)) {
    openCurrDirectory(app.nav.curr_path);
    renderUI();
    setDefaultCursorPos();
    return;
  }

  // Clear history stack and set new path
  while (!app.nav.back_stack.empty())
    app.nav.back_stack.pop();

  app.nav.curr_path = abs_path;
  app.nav.up_screen = 0;
  app.nav.x_curr = 1;

  // Refresh contents
  app.nav.file_list = getDirectoryFiles(app.nav.curr_path);
  app.layout.total_files = app.nav.file_list.size();
  app.nav.down_screen = (int)app.nav.file_list.size() - app.layout.row_size;
  if (app.nav.down_screen < 0)
    app.nav.down_screen = 0;

  renderUI();
  setDefaultCursorPos();
}
