#include "myheader.hpp"

void display(const std::string &file_name, const fs::path &root) noexcept {
  fs::path full_path = root / file_name;
  std::string display_file = file_name;

  bool selected = app.selection.selected_files.count(full_path.string());

  // truncate safely
  if ((int)display_file.length() > app.layout.col_size - 3) {
    display_file = display_file.substr(0, app.layout.col_size - 6) + "...";
  }

  // highlight selected
  if (selected)
    std::cout << ANSI::INVERSE;

  // directory coloring
  if (isDirectory(full_path)) {
    std::cout << ANSI::BOLD_GREEN << display_file << ANSI::RESET << "\n";
  } else {
    std::cout << display_file << "\n";
  }

  // reset highlight
  if (selected)
    std::cout << ANSI::RESET;
}

void displayTextFile(const std::string &file_path) {
  std::ifstream file(file_path);
  if (!file) {
    setCursorPos(1, 2 * app.layout.col_size);
    std::cout << ANSI::BOLD_RED << "Error Opening File" << ANSI::RESET << "\n";
    return;
  }

  std::string line;
  int y_pos = 1; // Start from the first line

  while (std::getline(file, line)) {
    if ((int)line.length() > app.layout.col_size - 3) {
      line = line.substr(0, app.layout.col_size - 7) +
             "..."; // Truncate after 30 chars
    }

    setCursorPos(
        y_pos++,
        2 * app.layout.col_size); // Print each line at a new y-coordinate
    std::cout << line << "\n";

    if (static_cast<int>(y_pos) >=
        app.ui.rows - 5) { // Avoid screen overflow (adjust as needed)
      setCursorPos(app.ui.rows - 5, 2 * app.layout.col_size);
      std::cout << ANSI::BOLD_YELLOW << "... Output Truncated" << ANSI::RESET
                << "\n";
      break;
    }
  }

  file.close();
}


void renderUI() {
  clearScreen();
  setCursorRed();

  /* -------- MIDDLE PANEL -------- */
  renderMiddlePanel();

  /* -------- RIGHT PANEL -------- */
  renderRightPanel();

  /* -------- STATUS BAR -------- */
  setCursorPos(app.ui.rows - 2, 0);
  std::cout << ANSI::BOLD_BLUE << "Current Path: " << app.nav.curr_path
            << ANSI::RESET << "\n";

  app.nav.y_curr = app.layout.col_size;
  setDefaultCursorPos();
}