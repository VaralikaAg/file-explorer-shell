#ifdef __APPLE__
#include "myheader.hpp"
#include <CoreServices/CoreServices.h>
#include <vector>
#include <string>
#include <mutex>

/* 
 * Callback for Apple FSEvents
 * This receives notifications for all filesystem activity within the watched root.
 */
static void fsevent_callback(
    ConstFSEventStreamRef streamRef,
    void *client_callback_info,
    size_t num_events,
    void *event_paths,
    const FSEventStreamEventFlags event_flags[],
    const FSEventStreamEventId event_ids[])
{
    char **paths = (char **)event_paths;

    for (size_t i = 0; i < num_events; i++) {
        std::string path = paths[i];
        FSEventStreamEventFlags flags = event_flags[i];

        WatcherEvent ev;
        ev.path = path;

        // FSEvents flags can be combined. We prioritize the most destructive/constructive ones.
        if (flags & kFSEventStreamEventFlagItemRemoved) {
            ev.type = WatcherEventType::DELETE;
        } else if (flags & kFSEventStreamEventFlagItemCreated) {
            ev.type = WatcherEventType::CREATE;
        } else if (flags & kFSEventStreamEventFlagItemRenamed) {
            ev.type = WatcherEventType::RENAME;
        } else if (flags & (kFSEventStreamEventFlagItemModified | kFSEventStreamEventFlagItemInodeMetaMod)) {
            ev.type = WatcherEventType::MODIFY;
        } else {
            ev.type = WatcherEventType::MODIFY;
        }

        {
            std::lock_guard<std::mutex> lock(app.indexing.mtx);
            app.indexing.event_queue.push(ev);
        }
        app.indexing.cv.notify_one();
    }
}

class MacFSEventsWatcher : public FileSystemWatcher {
    FSEventStreamRef stream = nullptr;
    std::thread loop_thread;
    CFRunLoopRef run_loop = nullptr;

public:
    MacFSEventsWatcher() : stream(nullptr), run_loop(nullptr) {}

    ~MacFSEventsWatcher() {
        stop();
    }

    bool start(const std::string& path) override {
        CFStringRef path_ref = CFStringCreateWithCString(nullptr, path.c_str(), kCFStringEncodingUTF8);
        CFArrayRef paths_to_watch = CFArrayCreate(nullptr, (const void **)&path_ref, 1, nullptr);

        FSEventStreamContext context = {0, nullptr, nullptr, nullptr, nullptr};
        
        /* 
         * We use kFSEventStreamCreateFlagFileEvents to get individual file notifications.
         * Without this flag, FSEvents only reports directory-level changes.
         */
        stream = FSEventStreamCreate(nullptr,
                                     &fsevent_callback,
                                     &context,
                                     paths_to_watch,
                                     kFSEventStreamEventIdSinceNow,
                                     5, // Latency in seconds
                                     kFSEventStreamCreateFlagFileEvents | kFSEventStreamCreateFlagNoDefer);

        CFRelease(path_ref);
        CFRelease(paths_to_watch);

        if (!stream) return false;

        loop_thread = std::thread([this]() {
            run_loop = CFRunLoopGetCurrent();
            FSEventStreamScheduleWithRunLoop(stream, run_loop, kCFRunLoopDefaultMode);
            FSEventStreamStart(stream);
            CFRunLoopRun();
        });

        return true;
    }

    void stop() override {
        if (stream) {
            if (run_loop) {
                CFRunLoopStop(run_loop);
            }
            if (loop_thread.joinable()) {
                loop_thread.join();
            }
            FSEventStreamStop(stream);
            FSEventStreamInvalidate(stream);
            FSEventStreamRelease(stream);
            stream = nullptr;
            run_loop = nullptr;
        }
    }
};

/* Factory function for the macOS implementation */
FileSystemWatcher* createWatcher() {
    return new MacFSEventsWatcher();
}

#endif
