#include "myheader.h"

#define posx(x, y) fprintf(stdout, "\033[%d;%dH", x, y)  // Move to (x, y)

// Function to check if the file has read permissions
bool isReadableFile(const string &filepath) {
    return (access(filepath.c_str(), R_OK) == 0);
}

// Function to determine if a file is binary
bool isBinaryFile(const string &filepath) {
    ifstream file(filepath, ios::binary);
    if (!file) return false;  // If file can't be opened, assume not binary

    char buffer[512];
    file.read(buffer, sizeof(buffer));
    int bytesRead = file.gcount();

    for (int i = 0; i < bytesRead; i++) {
        // If file contains non-printable characters (excluding whitespace), it's likely binary
        if (buffer[i] == 0 || (!isprint(buffer[i]) && !isspace(buffer[i]))) {
            return true;
        }
    }
    return false;  // If all characters are printable, assume it's a text file
}


// Function to display the content of a text file
void displayTextFile(const string &filepath) {
    ifstream file(filepath);
    if (!file) {
        posx(1, 2*colSize);
        printf("\033[1;31mError Opening File\033[0m\n");
        return;
    }

    string line;
    int yPos = 1;  // Start from the first line

    while (getline(file, line)) {
        if (line.length() > colSize-3) {
            line = line.substr(0, colSize-7) + "...";  // Truncate after 30 chars
        }

        posx(yPos++, 2*colSize);  // Print each line at a new y-coordinate
        printf("%s\n", line.c_str());

        if (static_cast<unsigned int>(yPos) >= rows - 5) {  // Avoid screen overflow (adjust as needed)
            posx(rows-5, 2*colSize);
            printf("\033[1;33m... Output Truncated\033[0m\n");
            break;
        }
    }

    file.close();
}
