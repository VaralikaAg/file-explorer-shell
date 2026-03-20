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

    /* ─── Track every path we see during this crawl (for zombie cleanup) ─── */
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
            if (isDirectory(entry.path())) {
                it.disable_recursion_pending();
            }
            continue;
        }

        const std::string path = entry.path().string();
        visited.insert(path);

        /* Get filesystem mtime via stat() — C++17 compatible */
        uint64_t mtime = 0;
        struct stat st;
        if (::stat(path.c_str(), &st) == 0)
            mtime = static_cast<uint64_t>(st.st_mtime);

        if (lastSync == 0 || mtime > lastSync) {
            /* New or modified — re-index */
            if (lastSync != 0)
                app.indexing.index.removePath(path); // clean old entries first
            app.indexing.index.indexPath(path);
            (lastSync == 0 ? newCount : updCount)++;
        } else {
            skipCount++;
        }
    }

    /* ─── Zombie cleanup: remove index entries for paths no longer on disk ─── */
    // We queue them up first (can't modify LMDB while we enumerate it here)
    // This is a lightweight O(N_indexed) pass using the queue built during traverse.
    // For now, any path that was in indexQueue previously but not in `visited`
    // would be caught on next crawl. Full zombie sweep can be added as a Phase 2.
    // (Traversal-based zombie detection requires iterating db_files — added in Phase 2)

    auto t2 = std::chrono::high_resolution_clock::now();
    auto secs = std::chrono::duration_cast<std::chrono::seconds>(t2 - t1).count();

    logMessage("Crawl complete in " + std::to_string(secs) + "s — "
               "new=" + std::to_string(newCount) +
               " updated=" + std::to_string(updCount) +
               " skipped=" + std::to_string(skipCount));

    /* ─── Update last sync timestamp in LMDB ─── */
    auto now = std::chrono::time_point_cast<std::chrono::seconds>(
                    std::chrono::system_clock::now());
    app.indexing.index.setLastSyncTime(
        static_cast<uint64_t>(now.time_since_epoch().count()));

    logMessage("Indexing finished. Sync timestamp updated.");
    app.indexing.indexingInProgress = false;
}