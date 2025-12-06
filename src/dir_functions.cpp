#include "myheader.h"

vector<string> fileList;
stack<NavState> backStack;
int up_screen, down_screen, prev_up_screen, prev_down_screen, for_up_screen, for_down_screen;
unsigned int totalFiles;
unordered_map<string, vector<string>> dirCache;

void invalidateDirCache(const string &dirPath)
{
    auto it = dirCache.find(dirPath);
    if (it != dirCache.end())
        dirCache.erase(it);
}


bool isDirectory(const char *path) {
    struct stat statbuf;
    if (stat(path, &statbuf) != 0)
        return false;
    return S_ISDIR(statbuf.st_mode);
}

int getDirectoryCount(const char *path)
{
	string dirPath(path);

	// logMessage("dirCache size before lookup: " + std::to_string(dirCache.size()));

	auto it = dirCache.find(dirPath);
    if (it != dirCache.end()) {
        fileList = it->second;
        return fileList.size();
    }

	vector<string> tempList;
	DIR *d;
	struct dirent *dir;
	d = opendir(path);
	if (d)
	{
		while ((dir = readdir(d)) != NULL)
		{
            string name = string(dir->d_name);
			if (name == "." || name == "..") continue;
			tempList.push_back(name);
		}
		closedir(d);
	}
	else
	{
		printf("No such Directory Exist:::");
	}

	sort(tempList.begin(), tempList.end());
	dirCache[dirPath] = tempList;
	fileList = tempList;

    return fileList.size();
}

void openDirectory(const char *path, int &up, int &down)
{
	totalFiles = getDirectoryCount(path);
    if(totalFiles == 0) {
        return;
    }
	up = 0, down=0;
    up = fileList.size() > rowSize ? fileList.size() - rowSize : 0;
    down = max(0,(int)(fileList.size() - up - rowSize));
}

void openCurrDirectory(const char *path)
{
	totalFiles = getDirectoryCount(path);
    if(totalFiles == 0) {
        return;
    }
}

void display(const char *fileName, const char *root)
{
	string path = string(root) + '/' + string(fileName);
	string displayFile = string(fileName);
	bool selected = selectedFiles.count(path);
	if(displayFile.length()>colSize-3)
		displayFile = displayFile.substr(0, colSize-6) + "...";
	if (selected)
        printf("\033[7m");
	if(isDirectory(path.c_str())){
		printf("\033[1;32m%s\033[0m\n", displayFile.c_str());
	}
	else printf("%s\n", displayFile.c_str());
	if (selected)
        printf("\033[0m");
}