#include "myheader.hpp"

void renderLeftPanel() {

  setCursorPos(1, 1);

  std::string s = "";

  std::cout << truncateStr("File Name: " + app.file_details.file_name,
                           app.file_details.col_size - 4)
            << std::endl;

  if (app.size_state.in_progress) {
    s = "File Size: Calculating";
    std::cout << truncateStr(s, app.file_details.col_size - 4) << std::endl;
  } else
    std::cout << truncateStr("File Size: " +
                                 humanReadableSize(app.size_state.last_size),
                             app.file_details.col_size - 4)
              << std::endl;

  std::cout << truncateStr("Ownership: " + app.file_details.user_name +
                               " (User)",
                           app.file_details.col_size - 4)
            << std::endl;
  std::cout << truncateStr(app.file_details.group_name + " (Group)",
                           app.file_details.col_size - 4)
            << std::endl;
  std::cout << truncateStr("Permissions: " + app.file_details.permissions,
                           app.file_details.col_size - 4)
            << std::endl;
  std::cout << truncateStr("Last Modified: " +
                               std::string(app.file_details.time_buffer),
                           app.file_details.col_size - 4)
            << std::endl;

  if (app.file_details.last_scan_duration >= 0)
    std::cout << truncateStr(
                     "Scan Time: " +
                         std::to_string(app.file_details.last_scan_duration) +
                         " ms",
                     app.file_details.col_size - 4)
              << std::endl;
  else {
    s = "Scan Time: Calculating";
    std::cout << truncateStr(s, app.file_details.col_size - 4) << std::endl;
  }
  setDefaultCursorPos();
}