#include "myheader.hpp"

void renderMiddlePanel() {

  for (int i = app.nav.up_screen, line = 1;
       i < app.nav.up_screen + app.layout.row_size &&
       i < (int)app.nav.file_list.size();
       i++, line++) {
    setCursorPos(line, app.layout.col_size + 2);
    display(app.nav.file_list[i], app.nav.curr_path);
  }
}

void renderRightPanel() {
  if (app.nav.file_list.empty())
    return;

  // Get selected file path
  std::string selected_file =
      app.nav.file_list[app.nav.x_curr + app.nav.up_screen - 1];
  fs::path next_path = fs::path(app.nav.curr_path) / selected_file;

  bool readable = isReadable(next_path);

  if (isDirectory(next_path)) {
    renderDirectoryPreview(next_path.string());
  } else if (isRegularFile(next_path)) {
    renderFilePreview(next_path.string(), readable);
  }

  /* -------- FILE DETAILS -------- */
  setCursorPos(1, 1);
  getFileDetails(next_path.string());
  renderLeftPanel();
}

void renderFilePreview(const std::string &next_path, bool is_readable) {
  if (is_readable) {
    if (!isBinaryFile(next_path)) {
      displayTextFile(next_path);
    } else {
      setCursorPos(1, 2 * app.layout.col_size);
      std::cout << ANSI::BOLD_RED << "Binary File" << ANSI::RESET << "\n";
    }
  } else {
    setCursorPos(1, 2 * app.layout.col_size);
    std::cout << ANSI::BOLD_RED << "No Read Permission" << ANSI::RESET << "\n";
  }
}

void renderDirectoryPreview(const std::string &next_path) {
  std::vector<std::string> files = getDirectoryFiles(next_path);
  int for_up = 0, for_down = 0, dummy_x = 1;
  normalizeRange((int)files.size(), app.layout.row_size, for_up, for_down,
                 dummy_x);

  if (files.empty()) {
    setCursorPos(1, 2 * app.layout.col_size);
    std::cout << ANSI::BOLD_RED << "No Items" << ANSI::RESET << "\n";
  } else {
    for (int i = for_up, line = 1;
         i < for_up + app.layout.row_size && i < (int)files.size();
         i++, line++) {
      setCursorPos(line, 2 * app.layout.col_size);
      display(files[i], next_path);
    }
  }
}

void refreshCurrentDirectory() {
  invalidateDirCache(app.nav.curr_path);
  openCurrDirectory(app.nav.curr_path);

  normalizeCursor();
}