#ifdef __APPLE__
#include "myheader.h"
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
    void *clientCallBackInfo,
    size_t numEvents,
    void *eventPaths,
    const FSEventStreamEventFlags eventFlags[],
    const FSEventStreamEventId eventIds[])
{
    char **paths = (char **)eventPaths;

    for (size_t i = 0; i < numEvents; i++) {
        std::string path = paths[i];
        FSEventStreamEventFlags flags = eventFlags[i];

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
            app.indexing.eventQueue.push(ev);
        }
        app.indexing.cv.notify_one();
    }
}

class MacFSEventsWatcher : public FileSystemWatcher {
    FSEventStreamRef stream = nullptr;
    std::thread loopThread;
    CFRunLoopRef runLoop = nullptr;

public:
    MacFSEventsWatcher() : stream(nullptr), runLoop(nullptr) {}

    ~MacFSEventsWatcher() {
        stop();
    }

    bool start(const std::string& path) override {
        CFStringRef pathRef = CFStringCreateWithCString(nullptr, path.c_str(), kCFStringEncodingUTF8);
        CFArrayRef pathsToWatch = CFArrayCreate(nullptr, (const void **)&pathRef, 1, nullptr);

        FSEventStreamContext context = {0, nullptr, nullptr, nullptr, nullptr};
        
        /* 
         * We use kFSEventStreamCreateFlagFileEvents to get individual file notifications.
         * Without this flag, FSEvents only reports directory-level changes.
         */
        stream = FSEventStreamCreate(nullptr,
                                     &fsevent_callback,
                                     &context,
                                     pathsToWatch,
                                     kFSEventStreamEventIdSinceNow,
                                     0.5, // Latency in seconds
                                     kFSEventStreamCreateFlagFileEvents | kFSEventStreamCreateFlagNoDefer);

        CFRelease(pathRef);
        CFRelease(pathsToWatch);

        if (!stream) return false;

        loopThread = std::thread([this]() {
            runLoop = CFRunLoopGetCurrent();
            FSEventStreamScheduleWithRunLoop(stream, runLoop, kCFRunLoopDefaultMode);
            FSEventStreamStart(stream);
            CFRunLoopRun();
        });

        return true;
    }

    void stop() override {
        if (stream) {
            if (runLoop) {
                CFRunLoopStop(runLoop);
            }
            if (loopThread.joinable()) {
                loopThread.join();
            }
            FSEventStreamStop(stream);
            FSEventStreamInvalidate(stream);
            FSEventStreamRelease(stream);
            stream = nullptr;
            runLoop = nullptr;
        }
    }
};

/* Factory function for the macOS implementation */
FileSystemWatcher* createWatcher() {
    return new MacFSEventsWatcher();
}

#endif
