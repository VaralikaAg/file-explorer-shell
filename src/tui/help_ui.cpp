#include "myheader.h"

void showHelp()
{
    clearScreen();

    setCursorPos(1, 5);
    printf("\033[1;33mCOMMAND MODE HELP (:)\033[0m");

    int r = 3;

    setCursorPos(r++, 5);
    printf("\033[1;36mrename <new_name>\033[0m");
    printf("        Rename selected file / directory");

    setCursorPos(r++, 5);
    printf("\033[1;36mcreate_file <name>\033[0m");
    printf("       Create a new file in current directory");

    setCursorPos(r++, 5);
    printf("\033[1;36mcreate_dir <name>\033[0m");
    printf("        Create a new directory");

    setCursorPos(r++, 5);
    printf("\033[1;36mcd <absolute_path>\033[0m");
    printf("       Change directory using absolute path");

    setCursorPos(r++, 5);
    printf("\033[1;36msearch <name>\033[0m");
    printf("            Search files & directories");

    setCursorPos(r++, 5);
    printf("\033[1;36msearch --file <name>\033[0m");
    printf("     Search only files");

    setCursorPos(r++, 5);
    printf("\033[1;36msearch --dir <name>\033[0m");
    printf("      Search only directories");

    setCursorPos(r++, 5);
    printf("\033[1;36mfind <keywords>\033[0m");
    printf("          Find paths using indexed search");

    setCursorPos(r++, 5);
    printf("\033[1;36mfind --dir <keywords>\033[0m");
    printf("    Find directories under current path");

    setCursorPos(r++, 5);
    printf("\033[1;36mhelp | --help\033[0m");
    printf("            Show this help screen");

    setCursorPos(r++, 5);
    printf("\033[1;36mq\033[0m");
    printf("                        Exit command mode");

    setCursorPos(app.ui.rows - 2, 5);
    printf("\033[1;32mPress any key to return...\033[0m");

    fflush(stdout);
    getchar();

    renderUI();
    setDefaultCursorPos();
}
