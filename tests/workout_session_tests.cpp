// SPDX-License-Identifier: GPL-3.0-only
#include "workout_session.hpp"

#include <chrono>
#include <iostream>

namespace {
using namespace std::chrono_literals;
using rep_counter::WorkoutSession;
using rep_counter::WorkoutState;
int failures = 0;

void expect(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        ++failures;
    }
}

void testValidationAndImmediateStart() {
    WorkoutSession session;
    const auto now = WorkoutSession::TimePoint{};
    expect(!session.start(0, 30, false, now), "zero target is rejected");
    expect(!session.start(10, -1, false, now), "negative rest is rejected");
    expect(session.start(3, 30, false, now), "valid workout starts");
    expect(session.state() == WorkoutState::Ready, "first rep starts immediately by default");
}

void testCountdownRoundingAndTransitions() {
    WorkoutSession session;
    const auto now = WorkoutSession::TimePoint{};
    expect(session.start(2, 60, true, now), "delayed workout starts");
    expect(session.remainingSeconds(now) == 60, "full duration is initially visible");
    expect(session.remainingSeconds(now + 200ms) == 60, "fractional seconds round upward");
    expect(session.remainingSeconds(now + 1001ms) == 59, "countdown advances correctly");
    expect(!session.tick(now + 59999ms), "timer does not finish early");
    expect(session.tick(now + 60s), "timer becomes ready at its deadline");
    expect(session.completeRep(now + 60s), "ready rep completes");
    expect(session.state() == WorkoutState::Resting, "rest follows a completed rep");
}

void testPauseSkipCompleteAndReset() {
    WorkoutSession session;
    const auto now = WorkoutSession::TimePoint{};
    expect(session.start(2, 30, false, now), "workout starts");
    expect(session.completeRep(now), "first rep completes");
    expect(session.pause(now + 5s), "rest pauses");
    expect(session.state() == WorkoutState::Paused, "paused state is exposed");
    expect(session.remainingSeconds(now + 20s) == 25, "paused time does not elapse");
    expect(session.resume(now + 20s), "rest resumes");
    expect(session.remainingSeconds(now + 20s) == 25, "resume preserves remaining time");
    expect(session.skipRest(), "rest can be skipped");
    expect(session.completeRep(now + 20s), "last rep completes");
    expect(session.state() == WorkoutState::Complete, "target completes workout");
    session.reset();
    expect(session.state() == WorkoutState::Setup, "reset returns to setup");
    expect(session.completed() == 0 && session.target() == 0, "reset clears progress");
}

void testZeroRestAndInvalidActions() {
    WorkoutSession session;
    const auto now = WorkoutSession::TimePoint{};
    expect(!session.completeRep(now), "a rep cannot complete during setup");
    expect(session.start(2, 0, true, now), "zero-rest workout starts");
    expect(session.state() == WorkoutState::Ready, "zero rest ignores initial delay");
    expect(session.completeRep(now), "first zero-rest rep completes");
    expect(session.state() == WorkoutState::Ready, "zero rest remains ready");
    expect(!session.pause(now), "ready workout cannot pause");
    expect(session.completeRep(now), "second zero-rest rep completes");
    expect(!session.completeRep(now), "completed workout rejects extra reps");
}
} // namespace

int main() {
    testValidationAndImmediateStart();
    testCountdownRoundingAndTransitions();
    testPauseSkipCompleteAndReset();
    testZeroRestAndInvalidActions();
    if (failures == 0) std::cout << "All workout session tests passed.\n";
    return failures == 0 ? 0 : 1;
}
