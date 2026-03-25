#include "myheader.hpp"

CommandResult processCommand(const std::string &command_line) {
  CommandResult result;

  std::vector<std::string> args;
  std::stringstream ss(command_line);
  std::string word;

  while (ss >> word)
    args.push_back(word);

  if (args.empty()) {
    result.refresh = true;
    return result;
  }

  std::string command = args[0];

  try {

    // RENAME
    if (command == "rename") {
      if (args.size() <= 1) {
        result.success = false;
        result.message = "Usage: rename <new_name>";
        return result;
      }

      std::string selected_file =
          app.nav.file_list[app.nav.x_curr + app.nav.up_screen - 1];

      if (!renameItem(selected_file, args[1])) {
        result.success = false;
        result.message = "Rename failed";
      } else {
        result.message = "Renamed successfully";
        result.target_file = args[1];
      }
    }

    // CREATE FILE
    else if (command == "create_file") {
      if (args.size() <= 1) {
        result.success = false;
        result.message = "Usage: create_file <name>";
        return result;
      }
      if (!createFile(args[1])) {
        result.success = false;
        result.message = "File creation failed";
      } else {
        result.message = "File created";
        result.target_file = args[1];
      }
    }

    // CREATE DIR
    else if (command == "create_dir") {
      if (args.size() <= 1) {
        result.success = false;
        result.message = "Usage: create_dir <name>";
        return result;
      }
      if (!createDirectory(args[1])) {
        result.success = false;
        result.message = "Directory creation failed";
      } else {
        result.message = "Directory created";
        result.target_file = args[1];
      }
    }

    // CD
    else if (command == "cd") {
      if (args.size() <= 1) {
        result.success = false;
        result.message = "Usage: cd <path>";
        return result;
      }
      std::string abs_path = args[1];
      for (int i = 2; i < (int)args.size(); i++)
        abs_path += " " + args[i];

      navigateToAbsolutePath(abs_path);
      result.message = "Changed directory";
    }

    // SEARCH
    else if (command == "search") {
      if (args.size() <= 1) {
        result.success = false;
        result.message = "Usage: search [--dir|--file] <name>";
        return result;
      }
      std::string flag, file_name;
      bool check_dir = true, check_file = true;

      flag = args[1];
      if (flag == "--dir")
        check_file = false;
      else if (flag == "--file")
        check_dir = false;

      if (check_file && check_dir)
        file_name = args[1];
      else if (args.size() > 2)
        file_name = args[2];
      else {
        result.success = false;
        result.message = "Usage: search [--dir|--file] <name>";
        return result;
      }

      searchCommand(check_dir, check_file, file_name);

      result.refresh = false;
    }

    // FIND
    else if (command == "find") {
      if (args.size() <= 1) {
        result.success = false;
        result.message = "Usage: find <query>";
        return result;
      }
      bool dir_only = false;
      size_t i = 1;

      if (args[1] == "--dir") {
        dir_only = true;
        i = 2;
        if (args.size() <= 2) {
          result.success = false;
          result.message = "Usage: find --dir <query>";
          return result;
        }
      }

      std::string query;
      for (; i < args.size(); i++) {
        if (!query.empty())
          query += " ";
        query += args[i];
      }

      app.indexing.index.search(query);

      if (dir_only) {
        std::vector<std::string> filtered;
        for (auto &p : app.search.found_paths) {
          if (isUnderCurrentDir(p))
            filtered.push_back(p);
        }
        app.search.found_paths.swap(filtered);
      }

      displaySearchResults();

      result.refresh = false;
    }

    // HELP
    else if (command == "--help" || command == "help") {
      result.refresh = false;
      result.message = "SHOW_HELP";
    }

    // QUIT COMMAND MODE
    else if (command == "q") {
      result.message = "EXIT_COMMAND_MODE";
    }

    // EXIT APP
    else if (command == "exit") {
      cleanupAndExit();
    }

    // INVALID
    else {
      result.success = false;
      result.message = "Invalid command (:help)";
    }

  } catch (const std::exception &e) {
    result.success = false;
    result.message = e.what();
  }

  return result;
}