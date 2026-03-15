#include "myheader.h"

void commandMode()
{
    posx(app.ui.rows - 2, 0);
    printf("\033[K");
    printf("\033[1;36m:\033[0m ");

    string commandLine = get_input();

    CommandResult res = processCommand(commandLine);

    // HANDLE SPECIAL CASES
    if (res.message == "SHOW_HELP") {
        showHelp();
        return;
    }

    if (res.message == "EXIT_COMMAND_MODE") {
        refreshCurrentDirectory();
        return;
    }

    // STATUS MESSAGE
    if (!res.message.empty()) {
        string color = res.success ? "\033[1;32m" : "\033[1;31m";
        showStatusMessage(res.message, color);
    }

    // REFRESH
    if (res.refresh) {
        refreshCurrentDirectory();

        if (!res.targetFile.empty()) {
            update_position(res.targetFile);
        }

        renderUI();
    }
}