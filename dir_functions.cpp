#include "myheader.h"

vector<string> fileList;
stack<NavState> backStack;
int up_screen, down_screen, prev_up_screen, prev_down_screen, for_up_screen, for_down_screen;
unsigned int totalFiles;

bool isDirectory(const char *path) {
    struct stat statbuf;
    if (stat(path, &statbuf) != 0)
        return false;
    return S_ISDIR(statbuf.st_mode);
}

int getDirectoryCount(const char *path)
{
	int count=0;
	fileList.clear();
	DIR *d;
	struct dirent *dir;
	d = opendir(path);
	if (d)
	{
		while ((dir = readdir(d)) != NULL)
		{
            string name = string(dir->d_name);
			if (name == "." || name == "..") continue;
			fileList.push_back(name);
            count++;
		}
		closedir(d);
	}
	else
	{
		printf("No such Directory Exist:::");
	}
	sort(fileList.begin(), fileList.end());
	return count;
}

void openDirectory(const char *path, int &up, int &down)
{
	totalFiles = getDirectoryCount(path);
    if(totalFiles == 0) {
        return;
    }
	sort(fileList.begin(), fileList.end());
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
	sort(fileList.begin(), fileList.end());
}

void display(const char *fileName, const char *root)
{
	string path = string(root) + '/' + string(fileName);
	string displayFile = string(fileName);
	if(displayFile.length()>colSize-3)
		displayFile = displayFile.substr(0, colSize-6) + "...";
	if(isDirectory(path.c_str())){
		printf("\033[1;32m%s\033[0m\n", displayFile.c_str());
	}
	else printf("%s\n", displayFile.c_str());
}