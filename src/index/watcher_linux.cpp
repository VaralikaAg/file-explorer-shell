#ifdef __linux__
#include "myheader.h"
#include <sys/inotify.h>
#include <unistd.h>
#include <map>
#include <vector>
#include <string>
#include <filesystem>

namespace fs = std::filesystem;

class LinuxFileSystemWatcher : public FileSystemWatcher {
    int fd = -1;
    std::thread loopThread;
    std::map<int, std::string> watchDescriptors;
    std::mutex watchMtx;

public:
    ~LinuxFileSystemWatcher() {
        stop();
    }

    bool start(const std::string& path) override {
        fd = inotify_init();
        if (fd < 0) return false;

        addRecursiveWatch(path);

        loopThread = std::thread([this]() {
            char buffer[4096];
            while (fd >= 0) {
                ssize_t length = read(fd, buffer, sizeof(buffer));
                if (length < 0) break;

                ssize_t i = 0;
                while (i < length) {
                    struct inotify_event *event = (struct inotify_event *)&buffer[i];
                    if (event->len) {
                        std::string name = event->name;
                        std::string parentPath;
                        {
                            std::lock_guard<std::mutex> lock(watchMtx);
                            parentPath = watchDescriptors[event->wd];
                        }
                        
                        std::string fullPath = parentPath + "/" + name;
                        WatcherEvent ev;
                        ev.path = fullPath;

                        if (event->mask & IN_CREATE) {
                            ev.type = WatcherEventType::CREATE;
                            if (event->mask & IN_ISDIR) addRecursiveWatch(fullPath);
                        } else if (event->mask & IN_DELETE) {
                            ev.type = WatcherEventType::DELETE;
                        } else if (event->mask & (IN_MODIFY | IN_CLOSE_WRITE)) {
                            ev.type = WatcherEventType::MODIFY;
                        } else if (event->mask & IN_MOVED_FROM) {
                            ev.type = WatcherEventType::DELETE; // Simplified move
                        } else if (event->mask & IN_MOVED_TO) {
                            ev.type = WatcherEventType::CREATE;
                            if (event->mask & IN_ISDIR) addRecursiveWatch(fullPath);
                        }

                        {
                            std::lock_guard<std::mutex> lock(app.indexing.mtx);
                            app.indexing.eventQueue.push(ev);
                        }
                        app.indexing.cv.notify_one();
                    }
                    i += sizeof(struct inotify_event) + event->len;
                }
            }
        });

        return true;
    }

    void addRecursiveWatch(const std::string& path) {
        std::error_code ec;
        if (!fs::exists(path, ec) || !fs::is_directory(path, ec)) return;

        int wd = inotify_add_watch(fd, path.c_str(), IN_CREATE | IN_DELETE | IN_MODIFY | IN_CLOSE_WRITE | IN_MOVED_FROM | IN_MOVED_TO);
        if (wd >= 0) {
            std::lock_guard<std::mutex> lock(watchMtx);
            watchDescriptors[wd] = path;
        }

        for (const auto& entry : fs::recursive_directory_iterator(path, fs::directory_options::skip_permission_denied, ec)) {
            if (fs::is_directory(entry, ec)) {
                int swd = inotify_add_watch(fd, entry.path().c_str(), IN_CREATE | IN_DELETE | IN_MODIFY | IN_CLOSE_WRITE | IN_MOVED_FROM | IN_MOVED_TO);
                if (swd >= 0) {
                    std::lock_guard<std::mutex> lock(watchMtx);
                    watchDescriptors[swd] = entry.path().string();
                }
            }
        }
    }

    void stop() override {
        int old_fd = fd;
        fd = -1;
        if (old_fd >= 0) {
            close(old_fd);
        }
        if (loopThread.joinable()) {
            loopThread.join();
        }
    }
};

FileSystemWatcher* createWatcher() {
    return new LinuxFileSystemWatcher();
}

#endif
