#include "myheader.h"

CommandResult processCommand(const std::string &commandLine)
{
    CommandResult result;

    std::vector<std::string> args;
    std::stringstream ss(commandLine);
    std::string word;

    while (ss >> word)
        args.push_back(word);

    if (args.empty()) {
        result.refresh = true;
        return result;
    }

    std::string command = args[0];

    try {

        // 🔹 RENAME
        if (command == "rename" && args.size() > 1) {
            std::string selectedFile =
                app.nav.fileList[app.nav.xcurr + app.nav.up_screen - 1];

            if (!renameItem(selectedFile, args[1])) {
                result.success = false;
                result.message = "Rename failed";
            } else {
                result.message = "Renamed successfully";
                result.targetFile = args[1];
            }
        }

        // 🔹 CREATE FILE
        else if (command == "create_file" && args.size() > 1) {
            if (!createFile(args[1])) {
                result.success = false;
                result.message = "File creation failed";
            } else {
                result.message = "File created";
                result.targetFile = args[1];
            }
        }

        // 🔹 CREATE DIR
        else if (command == "create_dir" && args.size() > 1) {
            if (!createDirectory(args[1])) {
                result.success = false;
                result.message = "Directory creation failed";
            } else {
                result.message = "Directory created";
                result.targetFile = args[1];
            }
        }

        // 🔹 CD
        else if (command == "cd" && args.size() > 1) {
            std::string absPath = args[1];
            for (int i = 2; i < (int)args.size(); i++)
                absPath += " " + args[i];

            navigateToAbsolutePath(absPath);
            result.message = "Changed directory";
        }

        // 🔹 SEARCH
        else if (command == "search" && args.size() > 1) {
            std::string flag, filename;
            bool check_dir = true, check_file = true;

            flag = args[1];
            if (flag == "--dir") check_file = false;
            else if (flag == "--file") check_dir = false;

            if (check_file && check_dir)
                filename = args[1];
            else if (args.size() > 2)
                filename = args[2];
            else {
                result.success = false;
                result.message = "Invalid search usage";
                return result;
            }

            searchCommand(check_dir, check_file, filename);

            result.refresh = false;
        }

        // 🔹 FIND
        else if (command == "find" && args.size() > 1) {
            bool dirOnly = false;
            size_t i = 1;

            if (args[1] == "--dir") {
                dirOnly = true;
                i = 2;
            }

            std::string query;
            for (; i < args.size(); i++) {
                if (!query.empty()) query += " ";
                query += args[i];
            }

            app.indexing.index.search(query);

            if (dirOnly) {
                std::vector<std::string> filtered;
                for (auto &p : app.search.foundPaths) {
                    if (isUnderCurrentDir(p))
                        filtered.push_back(p);
                }
                app.search.foundPaths.swap(filtered);
            }

            displaySearchResults();

            result.refresh = false;
        }

        // 🔹 HELP
        else if (command == "--help" || command == "help") {
            result.refresh = false;
            result.message = "SHOW_HELP";
        }

        // 🔹 QUIT COMMAND MODE
        else if (command == "q") {
            result.message = "EXIT_COMMAND_MODE";
        }

        // 🔹 EXIT APP
        else if (command == "exit") {
            cleanupAndExit();
        }

        // 🔹 INVALID
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