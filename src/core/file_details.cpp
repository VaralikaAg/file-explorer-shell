#include "myheader.hpp"

void startFolderSizeWorker(const std::string &path) {
  if (app.size_state.worker.joinable()) {
    app.size_state.cancel_flag = true;
    app.size_state.worker.join();
  }

  app.size_state.cancel_flag = false;
  app.size_state.in_progress = true;
  app.ui.refresh = false;
  app.file_details.last_scan_duration = -1;

  app.size_state.worker = std::thread([path]() {
    auto start = std::chrono::steady_clock::now();
    off_t size = getFolderSizeMT(path, app.config.workers);
    auto end = std::chrono::steady_clock::now();

    {
      app.size_state.last_size = size;
      app.file_details.last_scan_duration =
          std::chrono::duration_cast<std::chrono::milliseconds>(end - start)
              .count();
      app.size_state.in_progress = false;
      app.ui.refresh = true;
    }
  });
  app.size_state.worker.detach();
}

std::uintmax_t getFolderSizeMT(const std::string &root_path, int num_threads) {
  std::queue<fs::path> dir_queue;
  std::mutex mtx;
  std::condition_variable cv;

  std::atomic<std::uintmax_t> total_size{0};
  std::atomic<int> active_tasks{0};
  bool done = false;

  // ---- Init ----
  {
    std::lock_guard<std::mutex> lock(mtx);
    dir_queue.push(root_path);
    active_tasks = 1;
  }

  auto worker = [&]() {
    while (true) {
      fs::path current_dir;

      // ---- Get work ----
      {
        std::unique_lock<std::mutex> lock(mtx);
        cv.wait(lock, [&]() { return !dir_queue.empty() || done; });

        if (done && dir_queue.empty())
          return;

        current_dir = dir_queue.front();
        dir_queue.pop();
      }

      try {
        // ---- Traverse directory ----
        for (const auto &entry : fs::directory_iterator(current_dir)) {
          if (app.size_state.cancel_flag.load()) {
            std::lock_guard<std::mutex> lock(mtx);
            done = true;
            cv.notify_all();
            return;
          }

          if (entry.is_directory()) {
            std::lock_guard<std::mutex> lock(mtx);
            dir_queue.push(entry.path());
            active_tasks++;
            cv.notify_one();
          } else if (entry.is_regular_file()) {
            total_size += entry.file_size();
          }
        }
      } catch (const std::exception &e) {
        logMessage("Error scanning [" + current_dir.string() +
                   "]: " + e.what());
      } catch (...) {
        logMessage("Unknown error scanning [" + current_dir.string() + "]");
      }

      // ---- Task done ----
      if (--active_tasks == 0) {
        std::lock_guard<std::mutex> lock(mtx);
        done = true;
        cv.notify_all();
      }
    }
  };

  // ---- Launch threads ----
  std::vector<std::thread> threads;
  for (int i = 0; i < num_threads; i++)
    threads.emplace_back(worker);

  for (auto &t : threads)
    t.join();

  return total_size.load();
}

void getFileDetails(const std::string &path) {
  std::error_code ec;
  auto s = fs::symlink_status(path, ec);
  if (ec)
    return;

  app.file_details.col_size = app.layout.col_size;
  app.file_details.file_name = fs::path(path).filename().string();

  // Still using POSIX for user/group as std::filesystem doesn't provide these
  struct stat st;
  if (stat(path.c_str(), &st) == 0) {
    passwd *user = getpwuid(st.st_uid);
    group *grp = getgrgid(st.st_gid);
    app.file_details.user_name = user ? user->pw_name : "Unknown";
    app.file_details.group_name = grp ? grp->gr_name : "Unknown";

    auto p = s.permissions();
    app.file_details.permissions.clear();
    auto check = [&](fs::perms bit, char c) {
      app.file_details.permissions += (p & bit) != fs::perms::none ? c : '-';
    };
    check(fs::perms::owner_read, 'r');
    check(fs::perms::owner_write, 'w');
    check(fs::perms::owner_exec, 'x');
    check(fs::perms::group_read, 'r');
    check(fs::perms::group_write, 'w');
    check(fs::perms::group_exec, 'x');
    check(fs::perms::others_read, 'r');
    check(fs::perms::others_write, 'w');
    check(fs::perms::others_exec, 'x');

    std::time_t t = st.st_mtime;
    tm tm_result;
    localtime_r(&t, &tm_result);
    char buf[80];
    strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &tm_result);
    app.file_details.time_buffer = buf;

    if (fs::is_directory(s)) {
      if (app.file_details.current_path == path &&
          (app.size_state.in_progress || app.size_state.last_size > 0)) {
        return;
      }
      app.file_details.current_path = path;

      startFolderSizeWorker(path);
      app.file_details.file_size = "Calculating...";
    } else {
      app.file_details.current_path = path;
      auto sz = fs::file_size(path, ec);
      app.size_state.last_size = ec ? 0 : sz;
      app.file_details.file_size = humanReadableSize(app.size_state.last_size);
    }
  }
}
