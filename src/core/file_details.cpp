#include "myheader.h"

void startFolderSizeWorker(const string &path) {
    if (app.sizeState.worker.joinable()) {
        app.sizeState.cancelFlag = true;
        app.sizeState.worker.join();
    }

    app.sizeState.cancelFlag = false;
    app.sizeState.inProgress = true;
    app.ui.refresh = false;
    app.fileDetails.lastScanDuration = -1;

    app.sizeState.worker = thread([path]() {
        auto start = chrono::steady_clock::now();
        off_t size = getFolderSizeMT(path, app.config.workers);
        auto end = chrono::steady_clock::now();

        {
            app.sizeState.lastSize = size;
            app.fileDetails.lastScanDuration =
                chrono::duration_cast<chrono::milliseconds>(end - start).count();
            app.sizeState.inProgress = false;
            app.ui.refresh = true;
        }

        // print_details();
    });
    app.sizeState.worker.detach();
}

std::uintmax_t getFolderSizeMT(const string &rootPath, int numThreads)
{
    std::queue<fs::path> dirQueue;
    std::mutex mtx;
    std::condition_variable cv;

    std::atomic<std::uintmax_t> totalSize{0};
    std::atomic<int> activeTasks{0};
    bool done = false;

    // ---- Init ----
    {
        std::lock_guard<std::mutex> lock(mtx);
        dirQueue.push(rootPath);
        activeTasks = 1;
    }

    auto worker = [&]()
    {
        while (true)
        {
            fs::path currentDir;

            // ---- Get work ----
            {
                std::unique_lock<std::mutex> lock(mtx);
                cv.wait(lock, [&]()
                        { return !dirQueue.empty() || done; });

                if (done && dirQueue.empty())
                    return;

                currentDir = dirQueue.front();
                dirQueue.pop();
            }

            try
            {
                // ---- Traverse directory ----
                for (const auto &entry : fs::directory_iterator(currentDir))
                {
                    if (app.sizeState.cancelFlag.load())
                    {
                        std::lock_guard<std::mutex> lock(mtx);
                        done = true;
                        cv.notify_all();
                        return;
                    }

                    if (entry.is_directory())
                    {
                        std::lock_guard<std::mutex> lock(mtx);
                        dirQueue.push(entry.path());
                        activeTasks++;
                        cv.notify_one();
                    }
                    else if (entry.is_regular_file())
                    {
                        totalSize += entry.file_size();
                    }
                }
            }
            catch (...)
            {
                // Ignore permission errors etc.
            }

            // ---- Task done ----
            if (--activeTasks == 0)
            {
                std::lock_guard<std::mutex> lock(mtx);
                done = true;
                cv.notify_all();
            }
        }
    };

    // ---- Launch threads ----
    std::vector<std::thread> threads;
    for (int i = 0; i < numThreads; i++)
        threads.emplace_back(worker);

    for (auto &t : threads)
        t.join();

    return totalSize.load();
}

void getFileDetails(const string &path) {
    struct stat st;
    if (stat(path.c_str(), &st) != 0) return;

    app.fileDetails.colSize = app.layout.colSize;
    app.fileDetails.fileName = path.substr(path.find_last_of("/") + 1);

    struct passwd *user = getpwuid(st.st_uid);
    struct group  *grp  = getgrgid(st.st_gid);
    app.fileDetails.userName  = user ? user->pw_name : "Unknown";
    app.fileDetails.groupName = grp  ? grp->gr_name  : "Unknown";

    app.fileDetails.permissions.clear();
    app.fileDetails.permissions += (st.st_mode & S_IRUSR) ? 'r' : '-';
    app.fileDetails.permissions += (st.st_mode & S_IWUSR) ? 'w' : '-';
    app.fileDetails.permissions += (st.st_mode & S_IXUSR) ? 'x' : '-';
    app.fileDetails.permissions += (st.st_mode & S_IRGRP) ? 'r' : '-';
    app.fileDetails.permissions += (st.st_mode & S_IWGRP) ? 'w' : '-';
    app.fileDetails.permissions += (st.st_mode & S_IXGRP) ? 'x' : '-';
    app.fileDetails.permissions += (st.st_mode & S_IROTH) ? 'r' : '-';
    app.fileDetails.permissions += (st.st_mode & S_IWOTH) ? 'w' : '-';
    app.fileDetails.permissions += (st.st_mode & S_IXOTH) ? 'x' : '-';

    struct tm tm_result;

    localtime_r(&st.st_mtime, &tm_result);

    strftime(app.fileDetails.timeBuffer,
             sizeof(app.fileDetails.timeBuffer),
             "%Y-%m-%d %H:%M:%S",
             &tm_result);

    if (S_ISDIR(st.st_mode)) {
        startFolderSizeWorker(path);
        app.fileDetails.fileSize = "Calculating...";
    } else {
        app.sizeState.lastSize = st.st_size;
        app.fileDetails.fileSize = humanReadableSize(st.st_size);
    }
}

