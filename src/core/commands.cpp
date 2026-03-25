#include "myheader.hpp"

void searchCommand(bool check_dir, bool check_file, std::string file_name) {
  if (file_name.empty())
    return;

  std::string path = app.nav.curr_path;

  app.search.found_paths.resize(0);
  transform(file_name.begin(), file_name.end(), file_name.begin(), ::tolower);

  auto start = std::chrono::high_resolution_clock::now();
  searchAnything(path, file_name, check_file, check_dir);
  auto end = std::chrono::high_resolution_clock::now();
  auto elapsed =
      std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
  logMessage("Search took: " + std::to_string(elapsed.count()) + " ms");
  clearScreen();
  displaySearchResults();
}

void handleCopyAction() {
  std::vector<std::string> temp_clipboard;

  if (!app.selection.selected_files.empty()) {
    for (auto &path : app.selection.selected_files) {
      if (path.empty())
        throw std::runtime_error("Empty path in selected_files");

      temp_clipboard.push_back(path);
    }
  } else {
    int index = app.nav.x_curr + app.nav.up_screen - 1;

    if (index < 0 || index >= (int)app.nav.file_list.size())
      throw std::out_of_range("Invalid cursor index");

    std::string file = app.nav.file_list[index];
    temp_clipboard.push_back(app.nav.curr_path + "/" + file);
  }

  app.selection.clipboard = std::move(temp_clipboard);
}

std::string paste() {
  if (app.selection.clipboard.empty())
    return "";

  std::vector<std::string> pasted_files;
  std::string file_name = "";
  for (auto &src : app.selection.clipboard) {
    fs::path source_path(src);
    fs::path dest = fs::path(app.nav.curr_path) / source_path.filename();

    if (fs::is_directory(source_path)) {
      fs::copy(source_path, dest,
               fs::copy_options::recursive |
                   fs::copy_options::overwrite_existing);
    } else {
      fs::copy_file(source_path, dest, fs::copy_options::overwrite_existing);
    }

    pasted_files.push_back(dest.string());
    if (file_name.length() == 0)
      file_name = dest.filename().string();
  }

  app.selection.selected_files.clear();
  return file_name;
}

void deleteSelectedItems() {
  std::vector<std::string> targets;

  if (!app.selection.selected_files.empty()) {
    for (auto &p : app.selection.selected_files)
      targets.push_back(p);
  } else {
    std::string file = app.nav.file_list[app.nav.x_curr + app.nav.up_screen - 1];
    targets.push_back(app.nav.curr_path + "/" + file);
  }

  for (auto &path : targets) {
    fs::remove_all(path);
  }

  app.selection.selected_files.clear();
}

bool renameItem(const std::string &selected_file, const std::string &new_name) {
  if (new_name.empty())
    return false;

  std::string old_path = app.nav.curr_path + "/" + selected_file;
  std::string new_path = app.nav.curr_path + "/" + new_name;

  std::error_code ec;
  fs::rename(old_path, new_path, ec);
  if (ec)
    return false;

  return true;
}

bool createFile(const std::string &file_name) {
  if (file_name.empty())
    return false;

  std::string file_path = app.nav.curr_path + "/" + file_name;

  std::ofstream file(file_path);
  if (!file)
    return false;
  file.close();

  return true;
}

bool createDirectory(const std::string &dir_name) {
  if (dir_name.empty())
    return false;

  std::string dir_path = app.nav.curr_path + "/" + dir_name;

  std::error_code ec;
  if (!fs::create_directory(dir_path, ec))
    return false;

  return true;
}