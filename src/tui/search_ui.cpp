#include "myheader.hpp"

void displayFoundFiles(int file_up) {
    clearScreen();

    for (int i = file_up, line = 1; i < (int)app.search.found_paths.size() && i < file_up + app.layout.row_size; i++, line++)
    {
        std::string file_path = app.search.found_paths[i];

        const size_t available_width = app.ui.cols - 3;

        if (file_path.length() > available_width) {
            file_path = "..." + file_path.substr(
                file_path.length() - (available_width - 3)
            );
        }

        setCursorPos(static_cast<int>(line), 3);
        std::cout << file_path << "\n";
    }
}

void jumpToSearchResult(const std::string &selected_path){
    while (!app.nav.back_stack.empty())
    {
        app.nav.back_stack.pop();
    }

    std::stringstream ss(selected_path);
    std::string token;
    std::vector<std::string> path_segments;

    while (getline(ss, token, '/'))
    {
        path_segments.push_back(token);
    }

    std::string rebuilt_path = "";
    for (size_t i = 1; i < path_segments.size() - 1; i++)
    {
        rebuilt_path += "/";
        rebuilt_path += path_segments[i];

        NavState re_path;
        re_path.path = rebuilt_path;

        app.nav.file_list = getDirectoryFiles(rebuilt_path);
        app.layout.total_files = app.nav.file_list.size();

        auto it = find(app.nav.file_list.begin(), app.nav.file_list.end(), path_segments[i + 1]);

        if (it != app.nav.file_list.end())
        {
            int dis = distance(app.nav.file_list.begin(), it) + 1;

            int dummy_down = 0;
            scrollToIndex(dis - 1, (int)app.nav.file_list.size(), app.layout.row_size, re_path.up_screen, dummy_down, re_path.x_curr);
        }

        app.nav.back_stack.push(re_path);
    }

    if (!app.nav.back_stack.empty())
    {
        app.nav.curr_path = app.nav.back_stack.top().path;
    }

    std::string file_name = path_segments.back();

    openCurrDirectory(app.nav.curr_path.c_str());
    app.nav.back_stack.pop();

    auto it = find(app.nav.file_list.begin(), app.nav.file_list.end(), file_name);

    if (it != app.nav.file_list.end())
    {
        int dis = distance(app.nav.file_list.begin(), it) + 1;

        scrollToIndex(dis - 1, (int)app.nav.file_list.size(), app.layout.row_size, app.nav.up_screen, app.nav.down_screen, app.nav.x_curr);
    }
}

void displaySearchResults(){
    if ((int)app.search.found_paths.size() == 0)
        return;

    char ch;

    int file_up = 0, file_down = 0, filecurr = 1;
    normalizeRange((int)app.search.found_paths.size(), app.layout.row_size, file_up, file_down, filecurr);
    displayFoundFiles(file_up);
    setCursorPos(1,1);
    while (true) {
        ch = std::cin.get();

        if (ch == 27) {  // Escape sequence
            clearScreen();
            displayFoundFiles(file_up);
            setCursorPos(filecurr, 1);
            ch = std::cin.get();
            if (ch == '\n' || ch == EOF) {
                renderUI();
                break;
            }
            ch = std::cin.get();

            if (ch == 'A') {  // Up arrow key
                if (filecurr > 1) {
                    filecurr--;
                } else if (file_up > 0) {
                    file_up--;
                }
            } else if (ch == 'B') { // Down arrow key
                if (filecurr < app.layout.row_size && filecurr < (int)app.search.found_paths.size()) {
                    filecurr++;
                } else if (file_down > 0) {
                    file_up++;
                }
            }
            normalizeRange((int)app.search.found_paths.size(), app.layout.row_size, file_up, file_down, filecurr);
            displayFoundFiles(file_up);
            setCursorPos(filecurr, 1);
        }
        else if (ch == '\n' || ch == '\r'){
            std::string selected_path  = app.search.found_paths[filecurr + file_up - 1];
            jumpToSearchResult(selected_path);
            renderUI();
            return;
        }
        else if(ch=='q'){
            renderUI();
            return;
        }

    }

}
