#include "myheader.hpp"

void searchAnything(const fs::path &dir, const std::string &file_name,
                    bool check_file, bool check_dir) {
  for (const auto &entry : fs::directory_iterator(dir)) {
    fs::path full_path = entry.path();

    std::string entry_name = full_path.filename().string();
    std::transform(entry_name.begin(), entry_name.end(), entry_name.begin(),
                   ::tolower);

    bool is_dir = isDirectory(full_path);

    std::string query = file_name;
    std::transform(query.begin(), query.end(), query.begin(), ::tolower);

    // Match logic (no duplication)
    if ((is_dir && check_dir) || (!is_dir && check_file)) {
      if (entry_name.find(query) != std::string::npos) {
        app.search.found_paths.push_back(full_path.string());
      }
    }

    // Recurse
    if (is_dir) {
      searchAnything(full_path, file_name, check_file, check_dir);
    }
  }
}
