#include "myheader.h"

vector<string> foundPaths;  // Store found file/directory paths
#define esc 27
#define clear fputs("\033[2J", stdout)                  // Clear screen
#define posx(x, y) fprintf(stdout, "\033[%d;%dH", x, y)  // Move to (x, y)


void displayFoundFiles(int file_up, int file_down) {
    clear;

    for (size_t i = file_up, line = 1; i < foundPaths.size() && i < file_up + rowSize; i++, line++) {
        std::string filePath = foundPaths[i];
        
        // Truncate filePath if its length is less than colSize - 3
        if (filePath.length() < cols - 3) {
            filePath = filePath.substr(0, cols - 7) + "..."; // Add ellipsis for truncation
        }

        posx(static_cast<int>(line), 3);
        printf("%s\n", filePath.c_str());
    }
}


void displaySearchResults(){
    if(foundPaths.size()==0) return;

    char ch;

    unsigned int file_up=0, file_down=0, filecurr=1;
    file_down = foundPaths.size() - file_up - rowSize;
    displayFoundFiles(file_up, file_down);
    posx(1,1);
    while (true) {
        ch = cin.get();

        if (ch == 27) {  // Escape sequence
            clear;
            displayFoundFiles(file_up, file_down);
            posx(filecurr, 1);
            ch = cin.get();
            // ch = cin.peek();
            // logMessage(string(1,ch));
            if (ch == '\n' || ch == EOF) {  
                displayFiles();
                break;
            }
            //ch = cin.get();
            // logMessage(string(1,ch));
            ch = cin.get();

            if (ch == 'A') {  // Up arrow key
                // printf("Up arrow key used\n");
                if(filecurr>1){
                    filecurr--;
                    displayFoundFiles(file_up, file_down);
                    posx(filecurr, 1);
                }
                else if (filecurr==1 && file_up > 0) {
                    file_up--;
                    file_down++;
                    displayFoundFiles(file_up, file_down);
                }
                else{
                    displayFoundFiles(file_up, file_down);
                    posx(filecurr, 1);
                }
            }

            else if(ch=='B'){
                // printf("Down arrow key used\n");
                openCurrDirectory(currPath);
                if(filecurr<rowSize && filecurr<foundPaths.size()){
                    filecurr++;
                    displayFoundFiles(file_up, file_down);
                    posx(filecurr, 1);
                }
                else if(filecurr==rowSize && file_down>0){
                    file_up++;
                    file_down--;
                    displayFoundFiles(file_up, file_down);
                    posx(rowSize, 1);
                }
                else{
                    displayFoundFiles(file_up, file_down);
                    posx(min((int)(foundPaths.size()), (int)rowSize), 1);
                }
            }
        }
        else if (ch == '\n' || ch == '\r'){
            string selectedPath  = foundPaths[filecurr + file_up - 1];
            while (!backStack.empty()) {
                backStack.pop();
            }
            // Tokenize the selectedPath using '/'
            stringstream ss(selectedPath);
            string token;
            vector<string> pathSegments;

            while (getline(ss, token, '/')) {
                pathSegments.push_back(token);
            }

            // Push all except the last element (file name) into backStack
            string rebuiltPath = "";
            for (size_t i = 1; i < pathSegments.size() - 1; i++) {
                rebuiltPath += "/";
                rebuiltPath += pathSegments[i];
                NavState rePath;
                rePath.path=rebuiltPath;
                getDirectoryCount(rebuiltPath.c_str());
                auto it = find(fileList.begin(), fileList.end(), pathSegments[i+1]);
                if (it != fileList.end()){
                    int dis = distance(fileList.begin(), it) + 1;
                    if (fileList.size() <= rowSize) { 
                        rePath.up_screen = 0;
                        rePath.xcurr = dis;
                    } 
                    else if(fileList.size()-dis<rowSize){
                        rePath.up_screen = max(0, (int)(fileList.size() - rowSize));
                        rePath.xcurr = rowSize - ((int)fileList.size() - dis);
                    }
                    else{
                        rePath.xcurr = 1;
                        rePath.up_screen = dis-1;
                    }
                    
                }
                // rePath.xcurr=1;
                // rePath.up_screen=0;
                backStack.push(rePath);
                logMessage(rebuiltPath);
            }

            if (!backStack.empty()) {
                delete[] currPath;  
                currPath = new char[backStack.top().path.length() + 1];
                strcpy(currPath, backStack.top().path.c_str());
            }

            string fileName = pathSegments.back();
            openCurrDirectory(currPath);
            backStack.pop();

            auto it = find(fileList.begin(), fileList.end(), fileName);
            if (it != fileList.end()){
                int dis = distance(fileList.begin(), it) + 1;
                if (fileList.size() <= rowSize) { 
                    up_screen = 0;
                    xcurr = dis;
                } 
                else if(fileList.size()-dis<rowSize){
                    up_screen = max(0, (int)(fileList.size() - rowSize));
                    xcurr = rowSize - ((int)fileList.size() - dis);
                }
                else{
                    xcurr = 1;
                    up_screen = dis-1;
                }
                down_screen=fileList.size()-up_screen-rowSize;
            }
            displayFiles();
            return;
        }
        else if(ch=='q'){
            displayFiles();
            return;
        }

    }

}

void searchanything(char *path, string filename, bool check_file, bool check_dir) {
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
                foundPaths.push_back(fullPath); // Store the found path
            }
        }
        else if(!isDirectory(fullPath.c_str()) && check_file){
            if (entry_name.find(filename)!=string::npos) {
                foundPaths.push_back(fullPath); // Store the found path
            }
        }

        // If it's a directory, recurse into it
        if (isDirectory(fullPath.c_str())) {
            searchanything((char *)fullPath.c_str(), filename, check_file, check_dir);
        }
    }

    closedir(dir);
}

void searchCommand(bool check_dir, bool check_file)
{
    posx(rows-2, 0);
    if(check_dir && check_file)
        printf("\033[1;33mEnter file/dir name to search: \033[0m");
    else if (check_dir)
        printf("\033[1;33mEnter dir name to search: \033[0m");
    else if (check_file)
        printf("\033[1;33mEnter file name to search: \033[0m");

    string filename=get_input();
    // getline(cin >> ws, filename);
    if(filename.empty()) return;

    char *path = new char[strlen(currPath) + 1];
    strcpy(path, currPath);

    foundPaths.resize(0);
    transform(filename.begin(), filename.end(), filename.begin(), ::tolower); 
    searchanything(path, filename, check_file, check_dir);
    clear;
    displaySearchResults();
}