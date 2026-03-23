#include "myheader.h"

void handleResizeIfNeeded() {
    if (app.layout.resized) {
        app.layout.resized = 0;

        get_terminal_size();
        app.layout.rowSize = (app.ui.rows * 75) / 100;
        app.layout.colSize = app.ui.cols / 3;

        normalizeCursor();
    }
}

void cleanup() {
    app.sizeState.cancelFlag = true;

    if (app.sizeState.worker.joinable())
        app.sizeState.worker.join();
}

void stopFolderScan() {
    app.sizeState.cancelFlag = true;

    if (app.sizeState.worker.joinable())
        app.sizeState.worker.join();
    app.sizeState.inProgress = false;
    setDefaultCursorPos();
}

void runIndexingInBackground(const std::string root) {
    app.indexing.indexingInProgress = true;

    if (!isDirectory(root)) {
        app.indexing.indexingInProgress = false;
        fprintf(stderr,
            "\033[1;31mError:\033[0m indexing_root '%s' is not a valid directory\n",
            root.c_str());
        exit(EXIT_FAILURE);
    }

    /* ─── Read last sync timestamp from LMDB ─── */
    uint64_t lastSync = app.indexing.index.getLastSyncTime();
    logMessage(lastSync == 0 ? "First-time indexing (full crawl)..."
                             : "Differential crawl since ts=" + std::to_string(lastSync));

    std::unordered_set<std::string> visited;
    static const std::unordered_set<std::string> ignoreSet = {
        ".git", ".svn", ".hg", "node_modules", "build", "dist", ".env", "env", "venv", ".venv", "bin", "obj", ".idea", ".vscode"
    };

    auto t1 = std::chrono::high_resolution_clock::now();
    int  newCount = 0, updCount = 0, skipCount = 0;

    std::error_code ec;
    for (auto it = fs::recursive_directory_iterator(root,
                            fs::directory_options::skip_permission_denied, ec);
         it != fs::recursive_directory_iterator(); it.increment(ec))
    {
        if (ec) {
            logMessage("Error during traversal: " + ec.message());
            break;
        }

        const auto &entry = *it;
        const std::string filename = entry.path().filename().string();

        if (ignoreSet.count(filename)) {
            if (isDirectory(entry.path())) it.disable_recursion_pending();
            continue;
        }

        const std::string path = entry.path().string();
        visited.insert(path);

        uint64_t mtime = 0;
        struct stat st;
        if (::stat(path.c_str(), &st) == 0)
            mtime = static_cast<uint64_t>(st.st_mtime);

        if (lastSync == 0 || mtime > lastSync) {
            if (lastSync != 0) app.indexing.index.removePath(path);
            app.indexing.index.indexPath(path);
            (lastSync == 0 ? newCount : updCount)++;
        } else {
            skipCount++;
        }
    }

    auto t2 = std::chrono::high_resolution_clock::now();
    auto secs = std::chrono::duration_cast<std::chrono::seconds>(t2 - t1).count();
    logMessage("Crawl complete in " + std::to_string(secs) + "s — new=" + std::to_string(newCount) +
               " updated=" + std::to_string(updCount) + " skipped=" + std::to_string(skipCount));

    // Update last sync time
    app.indexing.index.setLastSyncTime(static_cast<uint64_t>(std::time(nullptr)));

    /* ─── Continuous Monitoring Loop (with Batching) ─── */
    logMessage("Real-time monitoring active.");
    
    MDB_txn *batchTxn = nullptr;
    int batchCount = 0;
    const int MAX_BATCH = 100;

    auto commitBatch = [&]() {
        if (batchTxn) {
            mdb_txn_commit(batchTxn);
            batchTxn = nullptr;
            batchCount = 0;
        }
    };

    while (!app.indexing.stopIndexer) {
        WatcherEvent event;
        bool hasEvent = false;

        {
            std::unique_lock<std::mutex> lock(app.indexing.mtx);
            // Wait for event or stop, but with a timeout for idle commit
            app.indexing.cv.wait_for(lock, std::chrono::milliseconds(500), [&] {
                return !app.indexing.eventQueue.empty() || app.indexing.stopIndexer;
            });

            if (app.indexing.stopIndexer) {
                commitBatch();
                break;
            }

            if (!app.indexing.eventQueue.empty()) {
                event = app.indexing.eventQueue.front();
                app.indexing.eventQueue.pop();
                hasEvent = true;
            } else {
                // Timeout reached - idle commit
                commitBatch();
                continue;
            }
        }

        if (hasEvent) {
            // Start transaction if needed
            if (!batchTxn) {
                if (mdb_txn_begin(app.indexing.index.getEnv(), nullptr, 0, &batchTxn) != MDB_SUCCESS) {
                    continue;
                }
            }

            // Process Event using the batch transaction
            if (event.type == WatcherEventType::CREATE || event.type == WatcherEventType::MODIFY) {
                app.indexing.index.removePath(event.path, batchTxn);
                app.indexing.index.indexPath(event.path, batchTxn);
            } else if (event.type == WatcherEventType::DELETE) {
                app.indexing.index.removePath(event.path, batchTxn);
            } else if (event.type == WatcherEventType::RENAME) {
                if (!event.oldPath.empty()) app.indexing.index.removePath(event.oldPath, batchTxn);
                app.indexing.index.indexPath(event.path, batchTxn);
            }

            batchCount++;
            if (batchCount >= MAX_BATCH) {
                commitBatch();
            }
        }
    }

    commitBatch();
    app.indexing.indexingInProgress = false;
    logMessage("Indexing background thread exit.");
}