#define UNICODE
#define NOMINMAX

#include <windows.h>
#include <dwmapi.h>
#include <mmsystem.h>
#include <uxtheme.h>

#include <chrono>
#include <filesystem>
#include <string>

#pragma comment(lib, "dwmapi.lib")
#pragma comment(lib, "uxtheme.lib")
#pragma comment(lib, "winmm.lib")

namespace {
constexpr const wchar_t* kVersion = L"0.4-alpha";
constexpr int kTargetInput = 101;
constexpr int kMinutesInput = 102;
constexpr int kStartButton = 103;
constexpr int kCompleteButton = 104;
constexpr UINT_PTR kTimerId = 1;

constexpr COLORREF kBackground = RGB(18, 19, 24);
constexpr COLORREF kSurface = RGB(31, 33, 42);
constexpr COLORREF kText = RGB(238, 240, 246);
constexpr COLORREF kMutedText = RGB(160, 166, 182);
constexpr COLORREF kAccent = RGB(112, 92, 255);
constexpr COLORREF kAccentPressed = RGB(86, 70, 207);

enum class WorkoutState { Setup, Resting, Ready, Complete };

struct AppState {
    HWND window{};
    HWND targetInput{};
    HWND minutesInput{};
    HWND startButton{};
    HWND completeButton{};
    HWND targetLabel{};
    HWND minutesLabel{};
    HWND status{};
    HWND timer{};
    HWND progress{};
    HFONT titleFont{};
    HFONT bodyFont{};
    HFONT timerFont{};
    HBRUSH backgroundBrush{};
    HBRUSH surfaceBrush{};
    WorkoutState state{WorkoutState::Setup};
    int target{};
    int completed{};
    int restSeconds{};
    std::chrono::steady_clock::time_point restEnds{};
};

AppState g;
void layoutControls(int width, int height) {
    constexpr int margin = 34;
    constexpr int gap = 32;
    const int contentWidth = width - margin * 2;
    const int firstInputWidth = (contentWidth - gap) / 3;
    const int secondInputX = margin + firstInputWidth + gap;
    const int secondInputWidth = contentWidth - firstInputWidth - gap;
    const int actionY = height - 52;
    const int statusY = actionY - 32;
    const int timerY = statusY - 76;

    MoveWindow(g.targetLabel, margin, 116, firstInputWidth, 24, TRUE);
    MoveWindow(g.minutesLabel, secondInputX, 116, secondInputWidth, 24, TRUE);
    MoveWindow(g.targetInput, margin, 144, firstInputWidth, 36, TRUE);
    MoveWindow(g.minutesInput, secondInputX, 144, secondInputWidth, 36, TRUE);
    MoveWindow(g.startButton, margin, 202, contentWidth, 42, TRUE);
    MoveWindow(g.progress, margin, timerY - 31, contentWidth, 26, TRUE);
    MoveWindow(g.timer, margin, timerY, contentWidth, 70, TRUE);
    MoveWindow(g.status, margin, statusY, contentWidth, 25, TRUE);
    MoveWindow(g.completeButton, width / 2 - 119, actionY, 238, 38, TRUE);
}

std::wstring formatTime(int seconds) {
    wchar_t buffer[16];
    swprintf_s(buffer, L"%02d:%02d", seconds / 60, seconds % 60);
    return buffer;
}

void setText(HWND control, const std::wstring& text) {
    SetWindowTextW(control, text.c_str());
}

int getInteger(HWND control) {
    wchar_t buffer[32]{};
    GetWindowTextW(control, buffer, static_cast<int>(std::size(buffer)));
    wchar_t* end{};
    const long value = wcstol(buffer, &end, 10);
    return (end == buffer || *end != L'\0' || value < 0 || value > 1000000) ? -1 : static_cast<int>(value);
}

std::filesystem::path soundPath(const wchar_t* filename) {
    const auto fromWorkingDirectory = std::filesystem::current_path() / L"sounds" / filename;
    if (std::filesystem::exists(fromWorkingDirectory)) return fromWorkingDirectory;

    wchar_t modulePath[MAX_PATH]{};
    GetModuleFileNameW(nullptr, modulePath, MAX_PATH);
    const auto fromBuildDirectory = std::filesystem::path(modulePath).parent_path().parent_path().parent_path() / L"sounds" / filename;
    return fromBuildDirectory;
}

void playSound(const wchar_t* filename) {
    const std::wstring path = std::filesystem::absolute(soundPath(filename)).wstring();
    const std::wstring openCommand = L"open \"" + path + L"\" type mpegvideo alias repCounterSound";
    mciSendStringW(L"close repCounterSound", nullptr, 0, nullptr);
    if (mciSendStringW(openCommand.c_str(), nullptr, 0, nullptr) == 0) {
        mciSendStringW(L"play repCounterSound", nullptr, 0, nullptr);
    }
}

void updateProgress() {
    setText(g.progress, L"REP " + std::to_wstring(g.completed) + L" / " + std::to_wstring(g.target));
}

void becomeReady() {
    g.state = WorkoutState::Ready;
    KillTimer(g.window, kTimerId);
    setText(g.status, L"Time for rep " + std::to_wstring(g.completed + 1));
    setText(g.timer, L"GO");
    setText(g.completeButton, L"Complete rep");
    EnableWindow(g.completeButton, TRUE);
    playSound(L"loud-noises!.mp3");
}

void startRest() {
    g.state = WorkoutState::Resting;
    g.restEnds = std::chrono::steady_clock::now() + std::chrono::seconds(g.restSeconds);
    setText(g.status, L"Resting before rep " + std::to_wstring(g.completed + 1));
    setText(g.completeButton, L"Resting");
    EnableWindow(g.completeButton, FALSE);
    SetTimer(g.window, kTimerId, 200, nullptr);
}

void updateTimer() {
    const auto now = std::chrono::steady_clock::now();
    const auto remaining = static_cast<int>(std::chrono::duration_cast<std::chrono::seconds>(g.restEnds - now).count());
    if (remaining <= 0) {
        becomeReady();
    } else {
        setText(g.timer, formatTime(remaining));
    }
}

void startWorkout() {
    const int target = getInteger(g.targetInput);
    const int minutes = getInteger(g.minutesInput);
    if (target <= 0 || minutes < 0) {
        setText(g.status, L"Enter a target above zero and a valid rest time.");
        return;
    }

    g.target = target;
    g.completed = 0;
    g.restSeconds = minutes * 60;
    EnableWindow(g.targetInput, FALSE);
    EnableWindow(g.minutesInput, FALSE);
    EnableWindow(g.startButton, FALSE);
    updateProgress();

    if (g.restSeconds == 0) becomeReady();
    else startRest();
}

void completeRep() {
    if (g.state != WorkoutState::Ready) return;
    ++g.completed;
    updateProgress();

    if (g.completed == g.target) {
        g.state = WorkoutState::Complete;
        setText(g.status, L"Workout complete. Great work!");
        setText(g.timer, L"DONE");
        setText(g.completeButton, L"Complete");
        EnableWindow(g.completeButton, FALSE);
    } else if (g.restSeconds == 0) {
        becomeReady();
    } else {
        startRest();
    }
}

void addLabel(HWND parent, const wchar_t* text, int x, int y, int width) {
    HWND label = CreateWindowW(L"STATIC", text, WS_CHILD | WS_VISIBLE, x, y, width, 24, parent, nullptr, nullptr, nullptr);
    SendMessageW(label, WM_SETFONT, reinterpret_cast<WPARAM>(g.bodyFont), TRUE);
}

LRESULT CALLBACK windowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
    case WM_COMMAND:
        if (LOWORD(wParam) == kStartButton) startWorkout();
        if (LOWORD(wParam) == kCompleteButton) completeRep();
        return 0;
    case WM_TIMER:
        if (wParam == kTimerId && g.state == WorkoutState::Resting) updateTimer();
        return 0;
    case WM_GETMINMAXINFO: {
        auto* info = reinterpret_cast<MINMAXINFO*>(lParam);
        info->ptMinTrackSize = {540, 460};
        return 0;
    }
    case WM_SIZE:
        if (g.targetInput) layoutControls(LOWORD(lParam), HIWORD(lParam));
        return 0;    case WM_CTLCOLOREDIT: {
        HDC dc = reinterpret_cast<HDC>(wParam);
        SetTextColor(dc, kText);
        SetBkColor(dc, kSurface);
        return reinterpret_cast<LRESULT>(g.surfaceBrush);
    }
    case WM_CTLCOLORSTATIC: {
        HDC dc = reinterpret_cast<HDC>(wParam);
        SetTextColor(dc, kMutedText);
        SetBkColor(dc, kBackground);
        return reinterpret_cast<LRESULT>(g.backgroundBrush);
    }
    case WM_DRAWITEM: {
        const auto* draw = reinterpret_cast<DRAWITEMSTRUCT*>(lParam);
        if (draw->CtlType != ODT_BUTTON) break;
        const bool enabled = IsWindowEnabled(draw->hwndItem) != FALSE;
        const bool pressed = (draw->itemState & ODS_SELECTED) != 0;
        HBRUSH brush = CreateSolidBrush(enabled ? (pressed ? kAccentPressed : kAccent) : RGB(65, 67, 78));
        FillRect(draw->hDC, &draw->rcItem, brush);
        DeleteObject(brush);
        SetBkMode(draw->hDC, TRANSPARENT);
        SetTextColor(draw->hDC, enabled ? RGB(255, 255, 255) : kMutedText);
        SelectObject(draw->hDC, g.bodyFont);
        wchar_t text[64]{};
        GetWindowTextW(draw->hwndItem, text, static_cast<int>(std::size(text)));
        DrawTextW(draw->hDC, text, -1, const_cast<RECT*>(&draw->rcItem), DT_CENTER | DT_VCENTER | DT_SINGLELINE);
        return TRUE;
    }
    case WM_DESTROY:
        KillTimer(window, kTimerId);
        mciSendStringW(L"close repCounterSound", nullptr, 0, nullptr);
        DeleteObject(g.titleFont);
        DeleteObject(g.bodyFont);
        DeleteObject(g.timerFont);
        DeleteObject(g.backgroundBrush);
        DeleteObject(g.surfaceBrush);
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(window, message, wParam, lParam);
}
} // namespace

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, PWSTR, int showCommand) {
    g.titleFont = CreateFontW(30, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Segoe UI");
    g.bodyFont = CreateFontW(17, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Segoe UI");
    g.timerFont = CreateFontW(54, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Segoe UI");
    g.backgroundBrush = CreateSolidBrush(kBackground);
    g.surfaceBrush = CreateSolidBrush(kSurface);

    WNDCLASSW windowClass{};
    windowClass.hInstance = instance;
    windowClass.lpszClassName = L"RepCounterWindow";
    windowClass.lpfnWndProc = windowProc;
    windowClass.hCursor = LoadCursor(nullptr, IDC_ARROW);
    windowClass.hbrBackground = g.backgroundBrush;
    RegisterClassW(&windowClass);

    const std::wstring windowTitle = std::wstring(L"Rep Counter v") + kVersion;
    g.window = CreateWindowExW(0, windowClass.lpszClassName, windowTitle.c_str(),
        WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 640, 480,
        nullptr, nullptr, instance, nullptr);
    const BOOL darkMode = TRUE;
    DwmSetWindowAttribute(g.window, 20, &darkMode, sizeof(darkMode));

    HWND title = CreateWindowW(L"STATIC", L"REP COUNTER", WS_CHILD | WS_VISIBLE, 32, 28, 340, 40, g.window, nullptr, instance, nullptr);
    SendMessageW(title, WM_SETFONT, reinterpret_cast<WPARAM>(g.titleFont), TRUE);
    addLabel(g.window, L"v" REP_COUNTER_VERSION L"  •  A focused workout timer", 34, 70, 400);
    addLabel(g.window, L"Rep target", 34, 116, 190);
    addLabel(g.window, L"Rest between reps (minutes)", 242, 116, 300);

    g.targetInput = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"10", WS_CHILD | WS_VISIBLE | ES_CENTER | ES_NUMBER, 34, 144, 176, 36, g.window, reinterpret_cast<HMENU>(static_cast<INT_PTR>(kTargetInput)), instance, nullptr);
    g.minutesInput = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"1", WS_CHILD | WS_VISIBLE | ES_CENTER | ES_NUMBER, 242, 144, 330, 36, g.window, reinterpret_cast<HMENU>(static_cast<INT_PTR>(kMinutesInput)), instance, nullptr);
    SendMessageW(g.targetInput, WM_SETFONT, reinterpret_cast<WPARAM>(g.bodyFont), TRUE);
    SendMessageW(g.minutesInput, WM_SETFONT, reinterpret_cast<WPARAM>(g.bodyFont), TRUE);
    SetWindowTheme(g.targetInput, L"DarkMode_Explorer", nullptr);
    SetWindowTheme(g.minutesInput, L"DarkMode_Explorer", nullptr);

    g.startButton = CreateWindowW(L"BUTTON", L"Start workout", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW, 34, 202, 538, 42, g.window, reinterpret_cast<HMENU>(static_cast<INT_PTR>(kStartButton)), instance, nullptr);
    g.progress = CreateWindowW(L"STATIC", L"SET UP YOUR WORKOUT", WS_CHILD | WS_VISIBLE | SS_CENTER, 34, 276, 538, 26, g.window, nullptr, instance, nullptr);
    g.timer = CreateWindowW(L"STATIC", L"--:--", WS_CHILD | WS_VISIBLE | SS_CENTER, 34, 306, 538, 70, g.window, nullptr, instance, nullptr);
    g.status = CreateWindowW(L"STATIC", L"Choose your target and rest time to begin.", WS_CHILD | WS_VISIBLE | SS_CENTER, 34, 382, 538, 25, g.window, nullptr, instance, nullptr);
    g.completeButton = CreateWindowW(L"BUTTON", L"Complete rep", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW, 184, 416, 238, 38, g.window, reinterpret_cast<HMENU>(static_cast<INT_PTR>(kCompleteButton)), instance, nullptr);
    EnableWindow(g.completeButton, FALSE);
    for (HWND control : {g.startButton, g.completeButton, g.progress, g.status}) SendMessageW(control, WM_SETFONT, reinterpret_cast<WPARAM>(g.bodyFont), TRUE);
    SendMessageW(g.timer, WM_SETFONT, reinterpret_cast<WPARAM>(g.timerFont), TRUE);
    RECT clientRect{};
    GetClientRect(g.window, &clientRect);
    layoutControls(clientRect.right, clientRect.bottom);
    ShowWindow(g.window, showCommand);
    UpdateWindow(g.window);

    MSG message;
    while (GetMessageW(&message, nullptr, 0, 0)) {
        TranslateMessage(&message);
        DispatchMessageW(&message);
    }
    return static_cast<int>(message.wParam);
}
