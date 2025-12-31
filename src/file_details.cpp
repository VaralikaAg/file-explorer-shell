#include "myheader.h"

#define pos() fprintf(stdout, "\033[%d;%dH", xcurr, ycurr)  // Move cursor
#define posx(x, y) fprintf(stdout, "\033[%d;%dH", x, y)  // Move to (x, y)

std::atomic<bool> sizeCancelFlag{false};
std::atomic<bool> sizeInProgress{false};
std::atomic<off_t> lastComputedSize{0};
std::thread sizeWorker;
std::mutex sizeMutex;

std::atomic<long long> lastScanDuration{-1};

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

                if (done && dirQueue.empty())
                    return;

                currentDir = dirQueue.front();
                dirQueue.pop();
            }

            struct stat st;
            if (lstat(currentDir.c_str(), &st) == 0) {
                // COUNT DIRECTORY ITSELF
                totalSize += st.st_blocks * 512;
            }

            DIR *dir = opendir(currentDir.c_str());
            if (!dir) {
                if (--tasks == 0) {
                    lock_guard<mutex> lock(mtx);
                    done = true;
                    cv.notify_all();
                }
                continue;
            }

            struct dirent *entry;
            while ((entry = readdir(dir)) != nullptr) {
                if (sizeCancelFlag.load()) {
                    closedir(dir);
                    return;
                }
                string name = entry->d_name;
                if (name == "." || name == "..") continue;

                string fullPath = currentDir + "/" + name;

                if (lstat(fullPath.c_str(), &st) == 0) {
                    
                    if (S_ISDIR(st.st_mode)) {
                        lock_guard<mutex> lock(mtx);
                        dirQueue.push(fullPath);
                        tasks++;
                        cv.notify_one();
                    }
                    
                    else{
                        totalSize += st.st_blocks * 512;
                    }
                }
            }

            closedir(dir);

            if (--tasks == 0) {
                lock_guard<mutex> lock(mtx);
                done = true;
                cv.notify_all();
            }
        }
    };

    sizeWorker.detach();

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
        if (!sizeInProgress) {
            sizeCancelFlag = false;
            sizeInProgress = true;

            if (sizeWorker.joinable())
                sizeWorker.join();

            sizeWorker = std::thread([path]() {
                auto start = chrono::steady_clock::now();
                off_t folderSize = getFolderSizeMT(path,CONFIG_WORKERS);
                auto end = chrono::steady_clock::now();
                lastScanDuration.store(chrono::duration_cast<chrono::milliseconds>(end - start).count());

                if (!sizeCancelFlag) {
                    lastComputedSize.store(folderSize);
                }
                sizeInProgress = false;
            });
        }
        if (sizeInProgress) {
            fileSize = "Calculating...";
        } else {
            fileSize = humanReadableSize(lastComputedSize) + " (dir)";
        }   
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
    if(lastScanDuration.load()!=-1){
        s= "Scan Time: " + to_string(lastScanDuration.load()) + " ms";
        logMessage(s);
        cout << truncate(s, colSize-4) << endl;
    }
    while(sizeInProgress){
    }
    posx(1,1);
    fileSize = humanReadableSize(lastComputedSize) + " (dir)";
    s="File Name: " + fileName;
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
    if(lastScanDuration.load()!=-1){
        s= "Scan Time: " + to_string(lastScanDuration.load()) + " ms";
        logMessage(s);
        cout << truncate(s, colSize-4) << endl;
    }
    pos();
}