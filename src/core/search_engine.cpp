#include "myheader.h"

void searchanything(const char *path, string filename, bool check_file, bool check_dir) {
    DIR *dir;
    struct dirent *entry;

    if ((dir = opendir(path)) == NULL) {
        return; // Cannot open directory
    }

    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue; // Ignore "." and ".."
        }

        // Construct full path
        string fullPath = string(path) + "/" + entry->d_name;
        string entry_name = (string)entry->d_name;
        transform(entry_name.begin(), entry_name.end(), entry_name.begin(), ::tolower); 

        // Check if the name matches
        if (isDirectory(fullPath.c_str()) && check_dir){
            if (entry_name.find(filename)!=string::npos) {
                app.search.foundPaths.push_back(fullPath); // Store the found path
            }
        }
        else if(!isDirectory(fullPath.c_str()) && check_file){
            if (entry_name.find(filename)!=string::npos) {
                app.search.foundPaths.push_back(fullPath); // Store the found path
            }
        }

        // If it's a directory, recurse into it
        if (isDirectory(fullPath.c_str())) {
            searchanything((char *)fullPath.c_str(), filename, check_file, check_dir);
        }
    }

    closedir(dir);
}
