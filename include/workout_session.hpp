// SPDX-License-Identifier: GPL-3.0-only
#pragma once

#include <chrono>

namespace rep_counter {

enum class WorkoutState { Setup, Resting, Ready, Paused, Complete };

class WorkoutSession {
public:
    using Clock = std::chrono::steady_clock;
    using TimePoint = Clock::time_point;

    bool start(int target, int restSeconds, bool delayFirstRep, TimePoint now = Clock::now()) noexcept;
    bool completeRep(TimePoint now = Clock::now()) noexcept;
    bool tick(TimePoint now = Clock::now()) noexcept;
    bool pause(TimePoint now = Clock::now()) noexcept;
    bool resume(TimePoint now = Clock::now()) noexcept;
    bool skipRest() noexcept;
    void reset() noexcept;

    [[nodiscard]] WorkoutState state() const noexcept { return state_; }
    [[nodiscard]] int target() const noexcept { return target_; }
    [[nodiscard]] int completed() const noexcept { return completed_; }
    [[nodiscard]] int restSeconds() const noexcept { return restSeconds_; }
    [[nodiscard]] int remainingSeconds(TimePoint now = Clock::now()) const noexcept;

private:
    void beginRest(TimePoint now) noexcept;
    WorkoutState state_{WorkoutState::Setup};
    int target_{};
    int completed_{};
    int restSeconds_{};
    TimePoint restEnds_{};
    std::chrono::milliseconds pausedRemaining_{};
};

} // namespace rep_counter
