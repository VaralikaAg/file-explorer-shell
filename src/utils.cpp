#include "myheader.h"

void normalizeCursor() {
    int total = fileList.size();

    if (total == 0) {
        xcurr = 1;
        up_screen = 0;
        down_screen = 0;
        return;
    }

    int maxUp = (total > (int)rowSize)
                    ? total - rowSize
                    : 0;

    if ((int)up_screen < 0) up_screen = 0;
    if ((int)up_screen > maxUp) up_screen = maxUp;

    int visible = total - up_screen;
    int maxX = min((int)rowSize, visible);

    if ((int)xcurr < 1) xcurr = 1;
    if ((int)xcurr > maxX) xcurr = maxX;

    down_screen = total - up_screen - rowSize;
    if (down_screen < 0) down_screen = 0;
}

void update_position(string fileName){
    int idx = -1;
    for (size_t i = 0; i < fileList.size(); i++) {
        if (fileList[i] == fileName) {
            idx = i;
            break;
        }
    }
    if (idx != -1) {
        // Calculate cursor & scrolling
        if ((unsigned int)idx < rowSize) {
            up_screen = 0;
            xcurr = idx + 1;
        } else {
            up_screen = idx - rowSize + 1;
            xcurr = rowSize;
        }

        down_screen = fileList.size() - up_screen - rowSize;
        if (down_screen < 0) down_screen = 0;
    }
}

void toggleSelect() {
    string file = fileList[xcurr + up_screen - 1];
    string fullPath = string(currPath) + "/" + file;

    if (selectedFiles.count(fullPath))
        selectedFiles.erase(fullPath);
    else
        selectedFiles.insert(fullPath);
}

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
                CONFIG_WORKERS = std::stoi(line.substr(line.find(":") + 1));
                if (CONFIG_WORKERS <= 0) {
                    CONFIG_WORKERS = 4;
                }
                logMessage("Loaded workers = " + std::to_string(CONFIG_WORKERS));
            } catch (...) {
                logMessage("Invalid workers value, using default");
                CONFIG_WORKERS = 4;
            }
            break;
        }
    }

    file.close();
}