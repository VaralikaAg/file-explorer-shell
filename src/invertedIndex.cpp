#include "myheader.h"

bool isRegularFile(const string &path) {
    struct stat st;
    return (stat(path.c_str(), &st) == 0 && S_ISREG(st.st_mode));
}

bool isDirectoryPath(const string &path) {
    struct stat st;
    return (stat(path.c_str(), &st) == 0 && S_ISDIR(st.st_mode));
}

void InvertedIndex::indexPath(const string &path)
{
    int file_id;

    // assign ID if new file
    auto it = fileToId.find(path);
    if (it == fileToId.end()) {
        file_id = idToFile.size();
        fileToId[path] = file_id;
        idToFile.push_back(path);
    } else {
        file_id = it->second;
    }

    // index filename or directory name
    size_t pos = path.find_last_of("/\\");
    string name = (pos == string::npos) ? path : path.substr(pos + 1);
    Dictionary[name].insert(file_id);

    // stop if directory
    if (isDirectoryPath(path)) return;
    if (!isRegularFile(path)) return;

    ifstream fp(path);
    if (!fp) return;

    string line, word;
    while (getline(fp, line)) {
        stringstream ss(line);
        while (ss >> word) {
            Dictionary[word].insert(file_id);
        }
    }

    fp.close();
}


void InvertedIndex::indexAllOnce(queue<string> &paths)
{
    while (!paths.empty()) {
        string path = paths.front();
        paths.pop();
        indexPath(path);
    }
}

void InvertedIndex::search(const string &word)
{
    auto t1 = chrono::high_resolution_clock::now();
    auto it = Dictionary.find(word);
    auto t2 = chrono::high_resolution_clock::now();
    logMessage("Search took: " + to_string(chrono::duration_cast<chrono::milliseconds>(t2 - t1).count()) + " ms");
    if (it == Dictionary.end()) {
        logMessage("Word not found: " + word);
        return;
    }

    logMessage("Results for word: " + word);

    for (int file_id : it->second) {
        logMessage(idToFile[file_id]);
    }
}
