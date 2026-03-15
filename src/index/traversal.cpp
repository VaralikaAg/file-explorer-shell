#include "myheader.h"

void traverse(const string &root) {
    DIR *dir = opendir(root.c_str());
    if (!dir) return;

    struct dirent *entry;
    while ((entry = readdir(dir)) != nullptr) {
        string name = entry->d_name;

        // Skip . and ..
        if (name == "." || name == "..") continue;

        string fullPath = root + "/" + name;
        app.indexing.indexQueue.push(fullPath);

        struct stat st;
        if (stat(fullPath.c_str(), &st) == 0 && S_ISDIR(st.st_mode)) {
            traverse(fullPath); // recursive traversal
        }
    }

    closedir(dir);
}