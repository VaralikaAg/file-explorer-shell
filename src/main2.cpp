// Including the header file
#include "myheader.h"

queue<string> indexQueue;
InvertedIndex globalIndex;

/* ---------- safe recursive traversal ---------- */
// void traverse(const string &path)
// {
//     struct stat st;
//     if (stat(path.c_str(), &st) != 0)
//         return;

//     // push every path (file OR dir)
//     indexQueue.push(path);

//     // stop if not directory
//     if (!S_ISDIR(st.st_mode))
//         return;

//     DIR *dir = opendir(path.c_str());
//     if (!dir)
//         return;

//     struct dirent *entry;
//     while ((entry = readdir(dir)) != nullptr)
//     {
//         string name = entry->d_name;

//         if (name == "." || name == "..")
//             continue;

//         string fullPath = path + "/" + name;
//         traverse(fullPath);
//     }

//     closedir(dir);
// }

/* ---------- main ---------- */
// int main(int argc, char *argv[])
// {
//     string root = ".";

//     if (argc > 1)
//         root = argv[1];

//     logMessage("Starting offline indexing...");
//     logMessage("Root: " + root);

//     /* 1️⃣ traverse filesystem */
//     traverse(root);

//     logMessage("Traversal complete.");
//     logMessage("Indexing started...");

//     /* 2️⃣ build index */
//     globalIndex.indexAllOnce(indexQueue);

//     logMessage("Indexing finished.");

//     /* 3️⃣ test search */
//     string query;
//     cout << "Enter word to search: ";
//     cin >> query;

//     globalIndex.search(query);

//     logMessage("Search done.");

//     return 0;
// }

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
        cout << "Enter word to search (or 'exit' to quit): ";
        cin >> query;
        if (query == "exit") break;

        logMessage("Searching for word: " + query);
        globalIndex.search(query);
        logMessage("Search done for word: " + query);
    }

    return 0;
}
