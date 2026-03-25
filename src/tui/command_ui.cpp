#include "myheader.hpp"

void commandMode()
{
    setCursorPos(app.ui.rows - 2, 0);
    std::cout << ANSI::CLEAR_LINE;
    std::cout << ANSI::BOLD_CYAN << ":" << ANSI::RESET << " " << std::flush;

    std::string command_line = getInput();

    CommandResult res = processCommand(command_line);

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
        std::string color = res.success ? ANSI::BOLD_GREEN : ANSI::BOLD_RED;
        showStatusMessage(res.message, color, res.success ? 0 : 2000);
    }

    // REFRESH
    if (res.refresh) {
        refreshCurrentDirectory();

        if (!res.target_file.empty()) {
            updatePosition(res.target_file);
        }

        renderUI();
    }
}