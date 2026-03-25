#include "myheader.hpp"

/*
 * startIndexing()
 *
 * 1. Opens the LMDB-backed index (loads existing data from disk if present).
 * 2. Launches the background indexing thread which performs a differential crawl.
 *
 * The LMDB environment persists across restarts — only new/modified files
 * are re-indexed on subsequent launches.
 */
void startIndexing()
{
    std::string s = "Process ID: " + std::to_string(getpid());
    logMessage(s);

    std::string main_root = app.config.indexing_root;

    if (!app.config.indexing_enabled) {
        logMessage("Indexing skipped (disabled in config)");
        return;
    }

    /* ── Open LMDB (creates or loads existing index on disk) ── */
    // Store the database in the user's home cache directory
    std::string db_dir = getenv("HOME") ? std::string(getenv("HOME")) + "/.cache/refined-explorer/lmdb"
                                       : "/tmp/refined-explorer/lmdb";

    app.indexing.index.open(db_dir);

    /* ── Initialize Real-time Watcher (inotify/FSEvents) ── */
    app.indexing.watcher = createWatcher();
    if (app.indexing.watcher) {
        if (app.indexing.watcher->start(main_root)) {
            logMessage("Real-time filesystem watcher started on: " + main_root);
        } else {
            logMessage("WARNING: Could not start real-time watcher.");
        }
    }

    /* ── Launch background differential crawl + indexing thread ── */
    app.indexing.worker = std::thread(runIndexingInBackground, main_root);
    app.indexing.worker.detach();
}