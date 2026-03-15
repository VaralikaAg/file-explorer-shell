#include "myheader.h"

void startIndexing()
{
    string s = "Process ID: " + to_string(getpid());
    logMessage(s);

    string main_root = app.config.indexingRoot;

    if (app.config.indexingEnabled)
    {
        app.indexing.worker = std::thread(runIndexingInBackground, main_root);
        app.indexing.worker.detach();
    }
    else
    {
        logMessage("Indexing skipped (disabled in config)");
    }
}