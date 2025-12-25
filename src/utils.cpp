#include "myheader.h"

void hideCursor() {
    printf("\033[?25l");
}

void showCursor() {
    printf("\033[?25h");
}


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
bool isUnderCurrentDir(const string &path) {
    string base(currPath);
    if (base.back() != '/') base += '/';
    return path.rfind(base, 0) == 0;
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
        }
        /* indexing flag */
        else if (line.find("indexing:") != std::string::npos) {
            std::string val = line.substr(line.find(":") + 1);

            if (val == "true" || val == "1" || val == "yes") {
                CONFIG_INDEXING = true;
            } else {
                CONFIG_INDEXING = false;
            }

            logMessage(
                std::string("Indexing ") +
                (CONFIG_INDEXING ? "ENABLED" : "DISABLED")
            );
        }
        else if (line.find("indexing_root:") != std::string::npos) {

            std::string root = line.substr(line.find(":") + 1);

            if (!root.empty() && root[0] == '/') {
                CONFIG_INDEXING_ROOT = root;
                logMessage("Indexing root set to: " + CONFIG_INDEXING_ROOT);
            } else {
                logMessage("Invalid indexing_root, using default: /home");
                CONFIG_INDEXING_ROOT = "/home";
            }
        }

    }

    file.close();
}

bool isValidDirectory(const string &path) {
    struct stat st;
    if (stat(path.c_str(), &st) != 0)
        return false;
    return S_ISDIR(st.st_mode);
}

void logMessage(const std::string& message) {
    std::ofstream logFile("logs/debug.log", std::ios_base::app); // Open log file in append mode
    if (logFile.is_open()) {
        logFile << message << std::endl; // Write message to file
    } else {
        std::cerr << "Error opening log file!" << std::endl;
    }
}

void traverse(const string &root) {
    DIR *dir = opendir(root.c_str());
    if (!dir) return;

    struct dirent *entry;
    while ((entry = readdir(dir)) != nullptr) {
        string name = entry->d_name;

        // Skip . and ..
        if (name == "." || name == "..") continue;

        string fullPath = root + "/" + name;
        indexQueue.push(fullPath);

        struct stat st;
        if (stat(fullPath.c_str(), &st) == 0 && S_ISDIR(st.st_mode)) {
            traverse(fullPath); // recursive traversal
        }
    }

    closedir(dir);
}

static const unordered_set<string> STOPWORDS = {
    "a","an","the","and","or","but","if","while","is","am","are","was","were",
    "to","of","in","on","for","with","as","by","at","from","this","that",
    "it","be","been","being","have","has","had","do","does","did"
};

string normalizeWord(const string &input)
{
    string out;
    out.reserve(input.size());

    // lowercase + strip punctuation
    for (char c : input) {
        if (isalnum(static_cast<unsigned char>(c))) {
            out += tolower(static_cast<unsigned char>(c));
        }
    }

    // remove empty
    if (out.empty())
        return "";

    // remove stopwords
    if (STOPWORDS.count(out))
        return "";

    return out;
}
