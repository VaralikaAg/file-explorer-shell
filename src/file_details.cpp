#include "myheader.h"

#define pos() cout << "\033[" << xcurr << ";" << ycurr << "H" << flush
#define posx(x, y) cout << "\033[" << x << ";" << y << "H" << flush

atomic<bool> sizeCancelFlag{false};
atomic<bool> sizeInProgress{false};
atomic<bool> uiRefresh{false};
atomic<off_t> lastComputedSize{0};
atomic<long long> lastScanDuration{-1};

thread sizeWorker;

string left_fileName;
string left_userName;
string left_groupName;
string left_permissions;
string left_fileSize;
int left_colSize;
char left_timeBuffer[80];

string truncate(const string &str, size_t maxLength) {
    if (str.length() > maxLength) {
        return str.substr(0, maxLength) + "...";
    }
    return str + string(maxLength - str.length(), ' ');
}

void print_details() {

    posx(1,1);

    std::string s = "";

    cout << truncate("File Name: " + left_fileName, left_colSize - 4) << endl;

    if (sizeInProgress){
        s = "File Size: Calculating";
        cout << truncate(s, left_colSize - 4) << endl;
    }
    else
        cout << truncate("File Size: " + humanReadableSize(lastComputedSize), left_colSize - 4) << endl;

    cout << truncate("Ownership: " + left_userName + " (User)", left_colSize - 4) << endl;
    cout << truncate(left_groupName + " (Group)", left_colSize - 4) << endl;
    cout << truncate("Permissions: " + left_permissions, left_colSize - 4) << endl;
    cout << truncate("Last Modified: " + string(left_timeBuffer), left_colSize - 4) << endl;

    if (lastScanDuration >= 0)
        cout << truncate("Scan Time: " + to_string(lastScanDuration) + " ms",left_colSize - 4) << endl;
    else{
        s = "Scan Time: Calculating";
        cout << truncate(s, left_colSize - 4) << endl;
    }
    pos();
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

void startFolderSizeWorker(const string &path) {
    if (sizeWorker.joinable()) {
        sizeCancelFlag = true;
        sizeWorker.join();
    }

    sizeCancelFlag = false;
    sizeInProgress = true;
    uiRefresh = false;
    lastScanDuration = -1;

    sizeWorker = thread([path]() {
        auto start = chrono::steady_clock::now();
        off_t size = getFolderSizeMT(path, CONFIG_WORKERS);
        auto end = chrono::steady_clock::now();

        {
            lastComputedSize = size;
            lastScanDuration =
                chrono::duration_cast<chrono::milliseconds>(end - start).count();
            sizeInProgress = false;
            uiRefresh = true;
        }

        // print_details();
    });
    sizeWorker.detach();
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
                    lock_guard<mutex> lock(mtx);
                    done = true;
                    cv.notify_all();
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

    vector<thread> threads;
    for (int i = 0; i < numThreads; i++)
        threads.emplace_back(worker);

    for (auto &t : threads)
        t.join();

    return totalSize.load();
}



// Function to get file details
void getFileDetails(const string &path) {
    struct stat st;
    if (stat(path.c_str(), &st) != 0) return;

    left_colSize = colSize;
    left_fileName = path.substr(path.find_last_of("/") + 1);

    struct passwd *user = getpwuid(st.st_uid);
    struct group  *grp  = getgrgid(st.st_gid);
    left_userName  = user ? user->pw_name : "Unknown";
    left_groupName = grp  ? grp->gr_name  : "Unknown";

    left_permissions.clear();
    left_permissions += (st.st_mode & S_IRUSR) ? 'r' : '-';
    left_permissions += (st.st_mode & S_IWUSR) ? 'w' : '-';
    left_permissions += (st.st_mode & S_IXUSR) ? 'x' : '-';
    left_permissions += (st.st_mode & S_IRGRP) ? 'r' : '-';
    left_permissions += (st.st_mode & S_IWGRP) ? 'w' : '-';
    left_permissions += (st.st_mode & S_IXGRP) ? 'x' : '-';
    left_permissions += (st.st_mode & S_IROTH) ? 'r' : '-';
    left_permissions += (st.st_mode & S_IWOTH) ? 'w' : '-';
    left_permissions += (st.st_mode & S_IXOTH) ? 'x' : '-';

    strftime(left_timeBuffer, sizeof(left_timeBuffer),
             "%Y-%m-%d %H:%M:%S", localtime(&st.st_mtime));

    if (S_ISDIR(st.st_mode)) {
        startFolderSizeWorker(path);
        left_fileSize = "Calculating...";
    } else {
        lastComputedSize = st.st_size;
        left_fileSize = humanReadableSize(st.st_size);
    }

    print_details();
}
