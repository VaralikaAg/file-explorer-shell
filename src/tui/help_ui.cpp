#include "myheader.hpp"

void showHelp()
{
    clearScreen();

    setCursorPos(1, 5);
    std::cout << ANSI::BOLD_YELLOW << "COMMAND MODE HELP (:)" << ANSI::RESET;

    int r = 3;

    setCursorPos(r++, 5);
    std::cout << ANSI::BOLD_CYAN << "rename <new_name>" << ANSI::RESET;
    std::cout << "        Rename selected file / directory";

    setCursorPos(r++, 5);
    std::cout << ANSI::BOLD_CYAN << "create_file <name>" << ANSI::RESET;
    std::cout << "       Create a new file in current directory";

    setCursorPos(r++, 5);
    std::cout << ANSI::BOLD_CYAN << "create_dir <name>" << ANSI::RESET;
    std::cout << "        Create a new directory";

    setCursorPos(r++, 5);
    std::cout << ANSI::BOLD_CYAN << "cd <absolute_path>" << ANSI::RESET;
    std::cout << "       Change directory using absolute path";

    setCursorPos(r++, 5);
    std::cout << ANSI::BOLD_CYAN << "search <name>" << ANSI::RESET;
    std::cout << "            Search files & directories";

    setCursorPos(r++, 5);
    std::cout << ANSI::BOLD_CYAN << "search --file <name>" << ANSI::RESET;
    std::cout << "     Search only files";

    setCursorPos(r++, 5);
    std::cout << ANSI::BOLD_CYAN << "search --dir <name>" << ANSI::RESET;
    std::cout << "      Search only directories";

    setCursorPos(r++, 5);
    std::cout << ANSI::BOLD_CYAN << "find <keywords>" << ANSI::RESET;
    std::cout << "          Find paths using indexed search";

    setCursorPos(r++, 5);
    std::cout << ANSI::BOLD_CYAN << "find --dir <keywords>" << ANSI::RESET;
    std::cout << "    Find directories under current path";

    setCursorPos(r++, 5);
    std::cout << ANSI::BOLD_CYAN << "help | --help" << ANSI::RESET;
    std::cout << "            Show this help screen";

    setCursorPos(r++, 5);
    std::cout << ANSI::BOLD_CYAN << "q" << ANSI::RESET;
    std::cout << "                        Exit command mode";

    setCursorPos(app.ui.rows - 2, 5);
    std::cout << ANSI::BOLD_GREEN << "Press any key to return..." << ANSI::RESET;

    std::cout << std::flush;
    std::cin.get();

    renderUI();
    setDefaultCursorPos();
}
