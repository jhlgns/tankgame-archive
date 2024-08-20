#include "common/file_watcher.hpp"

#include <filesystem>
#include <chrono>

i32 FileWatcher::Watch(StringView path, FileWatcherCallback callback) {
    auto res = static_cast<i32>(this->entries.size());
    this->entries.emplace_back(
        FileWatcherEntry{
            .path = String{path},
            .callback = ToRvalue(callback),
            .last_write_time = std::filesystem::last_write_time(path).time_since_epoch().count(),
        });

    return res;
}

void FileWatcher::StopWatching(i32 handle) {
    auto index = static_cast<size_t>(handle);
    assert(index < this->entries.size());
    std::swap(this->entries[index], this->entries.back());
    this->entries.pop_back();
}

void FileWatcher::Update() {
    for (auto &entry : this->entries) {
        std::error_code error;
        auto last_write_time = std::filesystem::last_write_time(entry.path, error).time_since_epoch().count();

        if (error) {
            // TODO(janh): Should we remove the entry if there was an error or do we keep checking?
            // The file might be deleted to be replaced afterwards...
            continue;
        }

        if (last_write_time != entry.last_write_time) {
            entry.callback();
            entry.last_write_time = last_write_time;
        }
    }
}

FileWatcher &GetFileWatcher() {
    static FileWatcher res;
    return res;
}
