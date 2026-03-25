#include "myheader.hpp"

void handleResizeIfNeeded() {
    if (app.layout.resized) {
        app.layout.resized = 0;

        getTerminalSize();
        app.layout.row_size = (app.ui.rows * 75) / 100;
        app.layout.col_size = app.ui.cols / 3;

        normalizeCursor();
    }
}

void cleanup() {
    app.size_state.cancel_flag = true;

    if (app.size_state.worker.joinable())
        app.size_state.worker.join();
}

void stopFolderScan() {
    app.size_state.cancel_flag = true;

    if (app.size_state.worker.joinable())
        app.size_state.worker.join();
    app.size_state.in_progress = false;
    setDefaultCursorPos();
}

void runIndexingInBackground(const std::string root) {
    app.indexing.indexing_in_progress = true;

    if (!isDirectory(root)) {
        app.indexing.indexing_in_progress = false;
        std::cerr << ANSI::BOLD_RED << "Error:" << ANSI::RESET << " indexing_root '" << root << "' is not a valid directory\n";
        exit(EXIT_FAILURE);
    }

    /* ─── Read last sync timestamp from LMDB ─── */
    uint64_t last_sync = app.indexing.index.getLastSyncTime();
    logMessage(last_sync == 0 ? "First-time indexing (full crawl)..."
                             : "Differential crawl since ts=" + std::to_string(last_sync));

    std::unordered_set<std::string> visited;
    static const std::unordered_set<std::string> ignore_set = {
        ".git", ".svn", ".hg", "node_modules", "build", "dist", ".env", "env", "venv", ".venv", "bin", "obj", ".idea", ".vscode"
    };

    auto t1 = std::chrono::high_resolution_clock::now();
    int  new_count = 0, upd_count = 0, skip_count = 0;

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
        const std::string file_name = entry.path().filename().string();

        if (ignore_set.count(file_name)) {
            if (isDirectory(entry.path())) it.disable_recursion_pending();
            continue;
        }

        const std::string path = entry.path().string();
        visited.insert(path);

        uint64_t mtime = 0;
        std::error_code stat_ec;
        auto ftime = fs::last_write_time(entry.path(), stat_ec);
        if (!stat_ec) {
            auto sctp = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
                ftime - fs::file_time_type::clock::now() + std::chrono::system_clock::now());
            mtime = std::chrono::system_clock::to_time_t(sctp);
        }

        if (last_sync == 0 || mtime > last_sync) {
            if (last_sync != 0) app.indexing.index.removePath(path);
            app.indexing.index.indexPath(path);
            (last_sync == 0 ? new_count : upd_count)++;
        } else {
            skip_count++;
        }
    }

    auto t2 = std::chrono::high_resolution_clock::now();
    auto secs = std::chrono::duration_cast<std::chrono::seconds>(t2 - t1).count();
    logMessage("Crawl complete in " + std::to_string(secs) + "s — new=" + std::to_string(new_count) +
               " updated=" + std::to_string(upd_count) + " skipped=" + std::to_string(skip_count));

    // Update last sync time
    app.indexing.index.setLastSyncTime(static_cast<uint64_t>(std::time(nullptr)));

    /* ─── Continuous Monitoring Loop (with Batching) ─── */
    logMessage("Real-time monitoring active.");
    
    MDB_txn *batch_txn = nullptr;
    int batch_count = 0;
    const int MAX_BATCH = 100;

    auto commit_batch = [&]() {
        if (batch_txn) {
            mdb_txn_commit(batch_txn);
            batch_txn = nullptr;
            batch_count = 0;
        }
    };

    while (!app.indexing.stop_indexer) {
        WatcherEvent event;
        bool has_event = false;

        {
            std::unique_lock<std::mutex> lock(app.indexing.mtx);
            // Wait for event or stop, but with a timeout for idle commit
            app.indexing.cv.wait_for(lock, std::chrono::milliseconds(500), [&] {
                return !app.indexing.event_queue.empty() || app.indexing.stop_indexer;
            });

            if (app.indexing.stop_indexer) {
                commit_batch();
                break;
            }

            if (!app.indexing.event_queue.empty()) {
                event = app.indexing.event_queue.front();
                app.indexing.event_queue.pop();
                has_event = true;
            } else {
                // Timeout reached - idle commit
                commit_batch();
                continue;
            }
        }

        if (has_event) {
            // Start transaction if needed
            if (!batch_txn) {
                if (mdb_txn_begin(app.indexing.index.getEnv(), nullptr, 0, &batch_txn) != MDB_SUCCESS) {
                    continue;
                }
            }

            // Process Event using the batch transaction
            if (event.type == WatcherEventType::CREATE || event.type == WatcherEventType::MODIFY) {
                app.indexing.index.updatePath(event.path, batch_txn);
            } else if (event.type == WatcherEventType::DELETE) {
                app.indexing.index.removePath(event.path, batch_txn);
            } else if (event.type == WatcherEventType::RENAME) {
                if (!event.old_path.empty()) app.indexing.index.removePath(event.old_path, batch_txn);
                app.indexing.index.updatePath(event.path, batch_txn);
            }

            batch_count++;
            if (batch_count >= MAX_BATCH) {
                commit_batch();
            }
        }
    }

    commit_batch();
    app.indexing.indexing_in_progress = false;
    logMessage("Indexing background thread exit.");
}