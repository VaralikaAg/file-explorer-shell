#include "myheader.h"

void showHelp()
{
    clearScreen;

    posx(1, 5);
    printf("\033[1;33mCOMMAND MODE HELP (:)\033[0m");

    int r = 3;

    posx(r++, 5);
    printf("\033[1;36mrename <new_name>\033[0m");
    printf("        Rename selected file / directory");

    posx(r++, 5);
    printf("\033[1;36mcreate_file <name>\033[0m");
    printf("       Create a new file in current directory");

    posx(r++, 5);
    printf("\033[1;36mcreate_dir <name>\033[0m");
    printf("        Create a new directory");

    posx(r++, 5);
    printf("\033[1;36mcd <absolute_path>\033[0m");
    printf("       Change directory using absolute path");

    posx(r++, 5);
    printf("\033[1;36msearch <name>\033[0m");
    printf("            Search files & directories");

    posx(r++, 5);
    printf("\033[1;36msearch --file <name>\033[0m");
    printf("     Search only files");

    posx(r++, 5);
    printf("\033[1;36msearch --dir <name>\033[0m");
    printf("      Search only directories");

    posx(r++, 5);
    printf("\033[1;36mfind <keywords>\033[0m");
    printf("          Find paths using indexed search");

    posx(r++, 5);
    printf("\033[1;36mfind --dir <keywords>\033[0m");
    printf("    Find directories under current path");

    posx(r++, 5);
    printf("\033[1;36mhelp | --help\033[0m");
    printf("            Show this help screen");

    posx(r++, 5);
    printf("\033[1;36mq\033[0m");
    printf("                        Exit command mode");

    posx(app.ui.rows - 2, 5);
    printf("\033[1;32mPress any key to return...\033[0m");

    fflush(stdout);
    getchar();

    renderUI();
    pos();
}
