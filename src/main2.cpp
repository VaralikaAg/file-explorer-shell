// Including the header file
#include "myheader.h"

queue<string> indexQueue;
InvertedIndex globalIndex;

// Function to traverse directories and populate indexQueue
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


int main() {
    cout << "Process ID: " << getpid() << endl;
    string root = "/home"; // fixed root

    logMessage("Starting offline indexing...");
    logMessage("Root directory: " + root);

    // 1️⃣ Traverse filesystem and fill index queue
    traverse(root);
    logMessage("Traversal complete. Total paths queued: " + to_string(indexQueue.size()));

    // 2️⃣ Build inverted index
    logMessage("Indexing started...");
    auto t1 = chrono::high_resolution_clock::now();
    globalIndex.indexAllOnce(indexQueue);
    auto t2 = chrono::high_resolution_clock::now();
    logMessage("Indexing finished.");
    logMessage("Indexing took: " + to_string(chrono::duration_cast<chrono::seconds>(t2 - t1).count()) + " seconds");


    // 3️⃣ Search loop
    while (true) {
        string query;
        cout << "Enter search query (or 'exit' to quit): ";
        getline(cin, query);

        if (query == "exit")
            break;

        logMessage("Searching for query: " + query);
        globalIndex.search(query);
        logMessage("Search done for query: " + query);
    }
    return 0;
}
