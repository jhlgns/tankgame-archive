#include <chrono>
#include <deque>

#include "common.hpp"

struct FrameTimer {
    using Clock = chrono::high_resolution_clock;
    using TimePoint = Clock::time_point;
    using Duration = Clock::duration;

    void Start();
    void BeginFrame();
    void BeginTick();
    bool FrameDone();
    void AdvanceTick();
    Duration GetTickLength() const;

    constexpr static Duration tick_length = chrono::microseconds{16'667};

    Duration tick_length_delta{}; // Changed by the server if the server wants the client to catch up/slow down
    TimePoint tick_length_delta_end{}; // This specifies for how long the tick_length_delta should be applied.
    TimePoint started;
    TimePoint current_frame{};
    TimePoint current_tick{};
    std::array<f32, 64> fps_ringbuf{};
    size_t fps_ringbuf_pos = 0;
    f32 fps_avg = 0.0f;
    i32 time_rate = 0; // TODO(janh): unused right now
    bool paused = false; // TODO(janh): unused right now
    f32 dt = 1.0f; // TODO(janh): unused right now
    Duration accu{};
    f32 total_time = 0.0f;
};

FrameTimer &GetFrameTimer();
