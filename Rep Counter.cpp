#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <limits>
#include <string>
#include <thread>

#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#include <mmsystem.h>
#pragma comment(lib, "winmm.lib")
#endif

namespace {
void clearConsole() {
#ifdef _WIN32
    std::system("cls");
#else
    std::system("clear");
#endif
}

void playSound(const std::filesystem::path& file) {
#ifdef _WIN32
    const std::string path = std::filesystem::absolute(file).string();
    const std::string open = "open \"" + path + "\" type mpegvideo alias rep_counter_sound";
    mciSendStringA("close rep_counter_sound", nullptr, 0, nullptr);
    if (mciSendStringA(open.c_str(), nullptr, 0, nullptr) == 0) mciSendStringA("play rep_counter_sound", nullptr, 0, nullptr);
#else
    (void)file;
    std::cout << "\a" << std::flush;
#endif
}

bool readNonNegativeInt(const std::string& prompt, int& value) {
    while (true) {
        std::cout << prompt;
        if (std::cin >> value && value >= 0) return true;
        if (std::cin.eof()) return false;
        std::cout << "Please enter a whole number that is zero or greater.\n\n";
        std::cin.clear();
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    }
}

void countdown(int seconds) {
    while (seconds-- > 0) {
        std::cout << std::setfill('0') << std::setw(2) << (seconds + 1) / 60 << " : "
                  << std::setw(2) << (seconds + 1) % 60 << std::setfill(' ') << '\r' << std::flush;
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    std::cout << "          \r" << std::flush;
}
} // namespace

int main() {
    clearConsole();
    std::cout << "Rep Counter v" << REP_COUNTER_VERSION << "\n\n";
    int totalReps = 0;
    if (!readNonNegativeInt("How many reps are you doing today?\n", totalReps)) return 0;
    if (totalReps == 0) {
        std::cout << "No workout scheduled today.\n";
        playSound("sounds/mother-fucker.mp3");
    } else {
        for (int completedReps = 0; completedReps < totalReps; ++completedReps) {
            std::cout << "\nYou have completed " << completedReps << " reps\n\n";
            int minutes = 0;
            if (!readNonNegativeInt("How long until the next rep in minutes?\n\n", minutes)) return 0;
            countdown(minutes * 60);
            std::cout << "\nStart your next rep.\n";
            playSound("sounds/loud-noises!.mp3");
            std::this_thread::sleep_for(std::chrono::seconds(10));
            clearConsole();
    std::cout << "Rep Counter v" << REP_COUNTER_VERSION << "\n\n";
        }
        std::cout << "Congrats, you are done working out!\n\n";
    }
    std::cout << "Closing window in:\n";
    countdown(60);
#ifdef _WIN32
    mciSendStringA("close rep_counter_sound", nullptr, 0, nullptr);
#endif
}
