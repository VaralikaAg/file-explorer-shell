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
    pos();
}

void runIndexingInBackground(const string root) {
    app.indexing.indexingInProgress = true;

    if (!isValidDirectory(root)) {
        app.indexing.indexingInProgress = false;
        fprintf(stderr,
            "\033[1;31mError:\033[0m indexing_root '%s' is not a valid directory\n",
            root.c_str()
        );
        exit(EXIT_FAILURE);   // exit code != 0
    }

    logMessage("Starting offline indexing...");

    traverse(root);

    logMessage("Traversal complete. Total paths queued: " +
               to_string(app.indexing.indexQueue.size()));

    auto t1 = chrono::high_resolution_clock::now();

    app.indexing.index.indexAllOnce(app.indexing.indexQueue);

    auto t2 = chrono::high_resolution_clock::now();

    logMessage("Indexing finished.");
    logMessage("Indexing took: " +
        to_string(
            chrono::duration_cast<chrono::seconds>(t2 - t1).count()
        ) + " seconds");

    app.indexing.indexingInProgress = false;
}