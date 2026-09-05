// SPDX-License-Identifier: GPL-3.0-only
#include "workout_session.hpp"

#include <algorithm>

namespace rep_counter {

bool WorkoutSession::start(int target, int restSeconds, bool delayFirstRep, TimePoint now) noexcept {
    if (state_ != WorkoutState::Setup || target <= 0 || restSeconds < 0) return false;
    target_ = target;
    completed_ = 0;
    restSeconds_ = restSeconds;
    if (delayFirstRep && restSeconds_ > 0) beginRest(now);
    else state_ = WorkoutState::Ready;
    return true;
}

bool WorkoutSession::completeRep(TimePoint now) noexcept {
    if (state_ != WorkoutState::Ready) return false;
    ++completed_;
    if (completed_ >= target_) state_ = WorkoutState::Complete;
    else if (restSeconds_ > 0) beginRest(now);
    else state_ = WorkoutState::Ready;
    return true;
}

bool WorkoutSession::tick(TimePoint now) noexcept {
    if (state_ != WorkoutState::Resting || now < restEnds_) return false;
    state_ = WorkoutState::Ready;
    pausedRemaining_ = std::chrono::milliseconds::zero();
    return true;
}

bool WorkoutSession::pause(TimePoint now) noexcept {
    if (state_ != WorkoutState::Resting) return false;
    pausedRemaining_ = std::max(std::chrono::milliseconds::zero(), std::chrono::duration_cast<std::chrono::milliseconds>(restEnds_ - now));
    state_ = pausedRemaining_ == std::chrono::milliseconds::zero() ? WorkoutState::Ready : WorkoutState::Paused;
    return true;
}

bool WorkoutSession::resume(TimePoint now) noexcept {
    if (state_ != WorkoutState::Paused) return false;
    restEnds_ = now + pausedRemaining_;
    state_ = WorkoutState::Resting;
    return true;
}

bool WorkoutSession::skipRest() noexcept {
    if (state_ != WorkoutState::Resting && state_ != WorkoutState::Paused) return false;
    pausedRemaining_ = std::chrono::milliseconds::zero();
    state_ = WorkoutState::Ready;
    return true;
}

void WorkoutSession::reset() noexcept { *this = WorkoutSession{}; }

int WorkoutSession::remainingSeconds(TimePoint now) const noexcept {
    std::chrono::milliseconds remaining{};
    if (state_ == WorkoutState::Resting) {
        remaining = std::max(std::chrono::milliseconds::zero(), std::chrono::duration_cast<std::chrono::milliseconds>(restEnds_ - now));
    } else if (state_ == WorkoutState::Paused) {
        remaining = pausedRemaining_;
    } else {
        return 0;
    }
    return static_cast<int>((remaining.count() + 999) / 1000);
}

void WorkoutSession::beginRest(TimePoint now) noexcept {
    pausedRemaining_ = std::chrono::seconds(restSeconds_);
    restEnds_ = now + pausedRemaining_;
    state_ = WorkoutState::Resting;
}

} // namespace rep_counter
