#include "myheader.h"

void loadConfig() {
    std::ifstream file("config.yml");
    if (!file.is_open()) {
        logMessage("config.yml not found, using default workers = 4");
        return;
    }

    std::string line;
    while (std::getline(file, line)) {
        // remove spaces
        line.erase(remove_if(line.begin(), line.end(), ::isspace), line.end());

        if (line.find("workers:") != std::string::npos) {
            try {
                app.config.workers = std::stoi(line.substr(line.find(":") + 1));
                if (app.config.workers <= 0) {
                    app.config.workers = 4;
                }
                logMessage("Loaded workers = " + std::to_string(app.config.workers));
            } catch (...) {
                logMessage("Invalid workers value, using default");
                app.config.workers = 4;
            }
        }
        /* indexing flag */
        else if (line.find("indexing:") != std::string::npos) {
            std::string val = line.substr(line.find(":") + 1);

            if (val == "true" || val == "1" || val == "yes") {
                app.config.indexingEnabled = true;
            } else {
                app.config.indexingEnabled = false;
            }

            logMessage(
                std::string("Indexing ") +
                (app.config.indexingEnabled ? "ENABLED" : "DISABLED")
            );
        }
        else if (line.find("indexing_root:") != std::string::npos) {

            std::string root = line.substr(line.find(":") + 1);

            if (!root.empty() && root[0] == '/') {
                app.config.indexingRoot = root;
                logMessage("Indexing root set to: " + app.config.indexingRoot);
            } else {
                logMessage("Invalid indexing_root, using default: /home");
                app.config.indexingRoot = "/home";
            }
        }

    }

    file.close();
}
