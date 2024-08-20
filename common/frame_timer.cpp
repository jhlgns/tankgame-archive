#include "common/frame_timer.hpp"
#include "common/log.hpp"

#include <numeric>
#include <thread>

void FrameTimer::Start() {
    auto now = Clock::now();
    this->started = now;
    this->current_frame = now;
    this->current_tick = now;
}

void FrameTimer::BeginFrame() {
    auto now = Clock::now();
    auto last_frame_duration = now - this->current_frame;
    this->current_frame = now;
    this->accu += last_frame_duration; // This is how much worth of a tick we have done in the last frame

    if (now >= this->tick_length_delta_end && this->tick_length_delta != chrono::milliseconds{0}) {
        this->tick_length_delta = chrono::milliseconds{0};
        LogInfo("frame_timer", "Tick length delta done");
        //DUMP(this->tick_length_delta);
    }
}

void FrameTimer::BeginTick() {
    auto now = Clock::now();
    auto last_tick_duration = now - this->current_tick;
    this->current_tick = now;

    auto last_tick_microseconds = chrono::duration_cast<chrono::microseconds>(last_tick_duration);
    this->fps_ringbuf[this->fps_ringbuf_pos++] = 1.0e6f / last_tick_microseconds.count();
    this->fps_ringbuf_pos %= this->fps_ringbuf.size();

    auto fps_accu = std::accumulate(this->fps_ringbuf.begin(), this->fps_ringbuf.end(), 0.0f);
    this->fps_avg = fps_accu / static_cast<f32>(this->fps_ringbuf.size());
    //DUMP(last_tick_ms);
    //DUMP(this->fps_avg);
}

bool FrameTimer::FrameDone() {
    // Have we done enough ticking in this frame?
    // Probably yes most of the time since our per tick code needs much less than 16ms
    return this->accu < this->GetTickLength();
}

void FrameTimer::AdvanceTick() {
    // Called after begin_tick when frame_done(). Here we know we have done one tick (16ms) worth of work.
    this->accu -= this->GetTickLength(); // HMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMM
    this->total_time += 1.0f;
}

FrameTimer &GetFrameTimer() {
    static FrameTimer res;
    return res;
}

FrameTimer::Duration FrameTimer::GetTickLength() const {
    return this->tick_length + this->tick_length_delta;
}
