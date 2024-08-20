#pragma once

#include "common.hpp"

using FileWatcherCallback = std::function<void()>;

struct FileWatcherEntry {
    String path;
    FileWatcherCallback callback;
    i64 last_write_time;
};

struct FileWatcher {
    i32 Watch(StringView path, FileWatcherCallback callback);
    void StopWatching(i32 handle);
    void Update();

    Array<FileWatcherEntry> entries;
};

FileWatcher &GetFileWatcher();
