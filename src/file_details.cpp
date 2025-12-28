#include "myheader.h"

string truncate(const string &str, size_t maxLength) {
    if (str.length() > maxLength) {
        return str.substr(0, maxLength) + "...";
    }
    return str;
}

// Function to convert file size to a human-readable format
string humanReadableSize(off_t size) {
    const char *units[] = {"B", "KB", "MB", "GB", "TB"};
    int unitIndex = 0;
    double sizeInUnit = size;

    while (sizeInUnit >= 1024 && unitIndex < 4) {
        sizeInUnit /= 1024;
        unitIndex++;
    }

    ostringstream oss;
    oss << fixed << setprecision(2) << sizeInUnit << " " << units[unitIndex];
    return oss.str();
}

// off_t getFolderSize(const string &folderPath) {
//     DIR *dir = opendir(folderPath.c_str());
//     if (!dir) return 0;

//     struct dirent *entry;
//     struct stat entryStat;
//     off_t totalSize = 0;

//     while ((entry = readdir(dir)) != nullptr) {
//         string name = entry->d_name;

//         // Skip . and ..
//         if (name == "." || name == "..") continue;

//         string fullPath = folderPath + "/" + name;

//         if (stat(fullPath.c_str(), &entryStat) == 0) {
//             if (S_ISDIR(entryStat.st_mode)) {
//                 totalSize += getFolderSize(fullPath); // Recurse
//             } else {
//                 totalSize += entryStat.st_size;
//             }
//         }
//     }

//     closedir(dir);
//     return totalSize;
// }

off_t getFolderSizeMT(const string &rootPath, int numThreads) {
    queue<string> dirQueue;
    mutex mtx;
    condition_variable cv;

    atomic<off_t> totalSize{0};
    atomic<int> tasks{0};
    bool done = false;

    dirQueue.push(rootPath);
    tasks = 1;

    auto worker = [&]() {
        while (true) {
            string currentDir;

            {
                unique_lock<mutex> lock(mtx);
                cv.wait(lock, [&] {
                    return !dirQueue.empty() || done;
                });

                if (done) return;

                currentDir = dirQueue.front();
                dirQueue.pop();
            }

            DIR *dir = opendir(currentDir.c_str());
            if (dir) {
                struct dirent *entry;
                struct stat st;

                while ((entry = readdir(dir)) != nullptr) {
                    string name = entry->d_name;
                    if (name == "." || name == "..") continue;

                    string fullPath = currentDir + "/" + name;

                    if (lstat(fullPath.c_str(), &st) == 0) {
                        if (S_ISDIR(st.st_mode)) {
                            {
                                lock_guard<mutex> lock(mtx);
                                dirQueue.push(fullPath);
                                tasks++;
                            }
                            cv.notify_one();
                        } else if (S_ISREG(st.st_mode)) {
                            totalSize += st.st_size;
                        }
                    }
                }
                closedir(dir);
            }

            if (--tasks == 0) {
                lock_guard<mutex> lock(mtx);
                done = true;
                cv.notify_all();
            }
        }
    };

    vector<thread> threads;
    for (int i = 0; i < numThreads; i++)
        threads.emplace_back(worker);

    for (auto &t : threads)
        t.join();

    return totalSize.load();
}



// Function to get file details
void getFileDetails(const string &path) {
    struct stat statbuf;
    // auto start,end,duration;
    long long duration=-1;

    // Get file statistics
    if (stat(path.c_str(), &statbuf) != 0) {
        // cerr << "Error: Unable to retrieve information for " << path << endl;
        return;
    }

    // Get file name (last part of the path)
    string fileName = path.substr(path.find_last_of("/") + 1);
    string fileSize;

    // Get file size in human-readable format
    if (S_ISDIR(statbuf.st_mode)) {
        // Get folder size recursively
        // off_t folderSize = getFolderSize(path);
        auto start = chrono::steady_clock::now();
        off_t folderSize = getFolderSizeMT(path,CONFIG_WORKERS);
        auto end = chrono::steady_clock::now();
        duration = chrono::duration_cast<chrono::milliseconds>(end - start).count();
        fileSize = humanReadableSize(folderSize) + " (dir)";
    } else {
        fileSize = humanReadableSize(statbuf.st_size);
    }

    // Get file ownership (user & group)
    struct passwd *user = getpwuid(statbuf.st_uid);
    struct group *grp = getgrgid(statbuf.st_gid);
    string userName = user ? user->pw_name : "Unknown";
    string groupName = grp ? grp->gr_name : "Unknown";

    // Get file permissions (in rwxrwxrwx format)
    string permissions;
    permissions += (statbuf.st_mode & S_IRUSR) ? 'r' : '-';
    permissions += (statbuf.st_mode & S_IWUSR) ? 'w' : '-';
    permissions += (statbuf.st_mode & S_IXUSR) ? 'x' : '-';
    permissions += (statbuf.st_mode & S_IRGRP) ? 'r' : '-';
    permissions += (statbuf.st_mode & S_IWGRP) ? 'w' : '-';
    permissions += (statbuf.st_mode & S_IXGRP) ? 'x' : '-';
    permissions += (statbuf.st_mode & S_IROTH) ? 'r' : '-';
    permissions += (statbuf.st_mode & S_IWOTH) ? 'w' : '-';
    permissions += (statbuf.st_mode & S_IXOTH) ? 'x' : '-';

    // Get last modified time
    time_t lastModified = statbuf.st_mtime;
    char timeBuffer[80];
    struct tm *timeinfo = localtime(&lastModified);
    strftime(timeBuffer, sizeof(timeBuffer), "%Y-%m-%d %H:%M:%S", timeinfo);

    string s="File Name: " + fileName;
    // Print the details
    cout << truncate(s, colSize-4) << endl;
    s="File Size: " + fileSize;
    // cout << "File Size: " << fileSize << endl;
    cout << truncate(s, colSize-4) << endl;
    s="Ownership: " + userName + "(User)";
    // cout << "Ownership: " << userName << "(User)" << endl;
    cout << truncate(s, colSize-4) << endl;
    s= groupName + " (Group)";
    // cout<<groupName << " (Group)" << endl;
    cout << truncate(s, colSize-4) << endl;
    s="Permissions: " + permissions;
    // cout << "Permissions: " << permissions << endl;
    cout << truncate(s, colSize-4) << endl;
    s="Last Modified: " + (string)timeBuffer;
    // cout << "Last Modified: " << timeBuffer << endl;
    cout << truncate(s, colSize-4) << endl;
    if(duration!=-1){
        s= "Scan Time: " + to_string(duration) + " ms";
        logMessage(s);
        cout << truncate(s, colSize-4) << endl;
    }

}