#include "myheader.h"

string humanReadableSize(off_t size) {
    const char *units[] = {"B", "KB", "MB", "GB", "TB"};
    int unitIndex = 0;
    double sizeInUnit = size;

    while (sizeInUnit >= 1024 && unitIndex < 4) {
        sizeInUnit /= 1024;
        unitIndex++;
    }

    ostringstream oss;
    oss << fixed << setprecision(2) << sizeInUnit << " " << units[unitIndex];
    return oss.str();
}

string truncateStr(const string &str, size_t maxLength) {
    if (str.length() > maxLength) {
        return str.substr(0, maxLength) + "...";
    }
    return str + string(maxLength - str.length(), ' ');
}