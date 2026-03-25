#include "myheader.hpp"

void initializeNavigation(int argc, char *argv[]) {

    // ROOT SET
    if (argc == 1) {
        app.nav.root = fs::current_path().string();
    }
    else{
        app.nav.root = argv[1];
    }

    std::string abs_path = app.nav.root;

    std::vector<std::string> path_parts;
    std::stringstream ss(abs_path);
    std::string segment;

    while (getline(ss, segment, '/')) {
        if (!segment.empty()) {
            path_parts.push_back(segment);
        }
    }

    while (!app.nav.back_stack.empty())
        app.nav.back_stack.pop();


    std::string new_path = "";

    for (size_t ind=0; ind<path_parts.size(); ind++) {
        const auto dir = path_parts[ind];
        new_path = new_path + "/" + dir;

        std::vector<std::string> curr_file_list;
        std::error_code ec;
        if (fs::exists(new_path, ec) && fs::is_directory(new_path, ec)) {
            for (const auto& entry : fs::directory_iterator(new_path, ec)) {
                curr_file_list.push_back(entry.path().filename().string());
            }
            std::sort(curr_file_list.begin(), curr_file_list.end());
        }

        int dir_index = 0;
        if (ind < path_parts.size() - 1) {
            auto it = std::find(curr_file_list.begin(), curr_file_list.end(), path_parts[ind + 1]);
            if (it != curr_file_list.end()) {
                dir_index = std::distance(curr_file_list.begin(), it);
            }
        }

        scrollToIndex(dir_index, (int)curr_file_list.size(), app.layout.row_size, app.nav.up_screen, app.nav.down_screen, app.nav.x_curr);

        NavState curr_state;
        curr_state.path = new_path;
        curr_state.x_curr = app.nav.x_curr;
        curr_state.up_screen = app.nav.up_screen;

        app.nav.back_stack.push(curr_state);
    }

    app.nav.back_stack.pop();
    app.nav.curr_path = app.nav.root;
}