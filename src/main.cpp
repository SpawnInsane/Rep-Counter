// SPDX-License-Identifier: GPL-3.0-only
#define UNICODE
#define _UNICODE
#define NOMINMAX

#include <windows.h>
#include <commctrl.h>
#include <dwmapi.h>
#include <mmsystem.h>
#include <uxtheme.h>

#include "version.hpp"
#include "workout_session.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <numbers>
#include <string>
#include <vector>

namespace {
constexpr UINT_PTR kTimerId = 1;
constexpr int kTargetInput = 101;
constexpr int kRestInput = 102;
constexpr int kStartButton = 103;
constexpr int kCompleteButton = 104;
constexpr int kPauseButton = 105;
constexpr int kSkipButton = 106;
constexpr int kResetButton = 107;
constexpr int kCancelButton = 108;
constexpr int kDelayFirstCheck = 109;
constexpr int kMuteCheck = 110;
constexpr int kSoundCombo = 111;
constexpr int kVolumeSlider = 112;
constexpr int kPreset30 = 113;
constexpr int kPreset60 = 114;
constexpr int kPreset90 = 115;
constexpr int kPrimaryAction = 120;

constexpr COLORREF kBackground = RGB(18, 19, 24);
constexpr COLORREF kSurface = RGB(31, 33, 42);
constexpr COLORREF kText = RGB(238, 240, 246);
constexpr COLORREF kMutedText = RGB(160, 166, 182);
constexpr COLORREF kAccent = RGB(112, 92, 255);
constexpr COLORREF kAccentPressed = RGB(86, 70, 207);

struct AppState {
    HINSTANCE instance{};
    HWND window{};
    HWND title{};
    HWND subtitle{};
    HWND targetLabel{};
    HWND restLabel{};
    HWND targetInput{};
    HWND restInput{};
    HWND preset30{};
    HWND preset60{};
    HWND preset90{};
    HWND delayFirst{};
    HWND mute{};
    HWND soundLabel{};
    HWND soundCombo{};
    HWND volumeLabel{};
    HWND volumeSlider{};
    HWND startButton{};
    HWND progress{};
    HWND timer{};
    HWND status{};
    HWND completeButton{};
    HWND pauseButton{};
    HWND skipButton{};
    HWND resetButton{};
    HWND cancelButton{};
    HFONT titleFont{};
    HFONT bodyFont{};
    HFONT timerFont{};
    HBRUSH backgroundBrush{};
    HBRUSH surfaceBrush{};
    HACCEL accelerators{};
    UINT dpi{96};
    rep_counter::WorkoutSession session;
    std::vector<std::uint8_t> soundData;
};

AppState g;

int scaled(int value) { return MulDiv(value, static_cast<int>(g.dpi), 96); }

void setText(HWND control, const std::wstring& text) {
    if (control) SetWindowTextW(control, text.c_str());
}

int getInteger(HWND control, int maximum) {
    wchar_t buffer[16]{};
    if (GetWindowTextW(control, buffer, static_cast<int>(std::size(buffer))) <= 0) return -1;
    wchar_t* end{};
    const long value = wcstol(buffer, &end, 10);
    return (end == buffer || *end != L'\0' || value < 0 || value > maximum) ? -1 : static_cast<int>(value);
}

std::wstring formatTime(int seconds) {
    wchar_t buffer[24]{};
    swprintf_s(buffer, L"%02d:%02d", seconds / 60, seconds % 60);
    return buffer;
}

void appendU16(std::vector<std::uint8_t>& bytes, std::uint16_t value) {
    bytes.push_back(static_cast<std::uint8_t>(value));
    bytes.push_back(static_cast<std::uint8_t>(value >> 8));
}

void appendU32(std::vector<std::uint8_t>& bytes, std::uint32_t value) {
    for (int shift = 0; shift < 32; shift += 8) bytes.push_back(static_cast<std::uint8_t>(value >> shift));
}

void appendTag(std::vector<std::uint8_t>& bytes, const char* tag) {
    for (int i = 0; i < 4; ++i) bytes.push_back(static_cast<std::uint8_t>(tag[i]));
}

std::vector<std::uint8_t> makeTone(double firstHz, double secondHz, int volume, int milliseconds) {
    constexpr std::uint32_t sampleRate = 44100;
    const auto sampleCount = static_cast<std::uint32_t>(sampleRate * milliseconds / 1000);
    const std::uint32_t dataSize = sampleCount * sizeof(std::int16_t);
    std::vector<std::uint8_t> bytes;
    bytes.reserve(44 + dataSize);
    appendTag(bytes, "RIFF"); appendU32(bytes, 36 + dataSize); appendTag(bytes, "WAVE");
    appendTag(bytes, "fmt "); appendU32(bytes, 16); appendU16(bytes, 1); appendU16(bytes, 1);
    appendU32(bytes, sampleRate); appendU32(bytes, sampleRate * 2); appendU16(bytes, 2); appendU16(bytes, 16);
    appendTag(bytes, "data"); appendU32(bytes, dataSize);
    const double gain = std::clamp(volume, 0, 100) / 100.0 * 0.55;
    for (std::uint32_t i = 0; i < sampleCount; ++i) {
        const double position = static_cast<double>(i) / sampleCount;
        const double frequency = position < 0.5 ? firstHz : secondHz;
        const double envelope = std::min({1.0, position * 25.0, (1.0 - position) * 14.0});
        const double wave = std::sin(2.0 * std::numbers::pi * frequency * i / sampleRate);
        appendU16(bytes, static_cast<std::uint16_t>(static_cast<std::int16_t>(32767.0 * gain * envelope * wave)));
    }
    return bytes;
}

void playAlert(bool completion = false) {
    if (SendMessageW(g.mute, BM_GETCHECK, 0, 0) == BST_CHECKED) return;
    const int selection = static_cast<int>(SendMessageW(g.soundCombo, CB_GETCURSEL, 0, 0));
    const int volume = static_cast<int>(SendMessageW(g.volumeSlider, TBM_GETPOS, 0, 0));
    const std::array<std::array<double, 2>, 3> tones{{{{659.25, 880.0}}, {{880.0, 1174.66}}, {{523.25, 783.99}}}};
    const auto& tone = tones[std::clamp(selection, 0, 2)];
    PlaySoundW(nullptr, nullptr, 0);
    g.soundData = makeTone(tone[0], completion ? tone[1] * 1.25 : tone[1], volume, completion ? 520 : 320);
    PlaySoundW(reinterpret_cast<LPCWSTR>(g.soundData.data()), nullptr, SND_MEMORY | SND_ASYNC | SND_NODEFAULT);
}

void applyFont(HWND control, HFONT font) {
    if (control) SendMessageW(control, WM_SETFONT, reinterpret_cast<WPARAM>(font), TRUE);
}

void createFonts() {
    HFONT newTitle = CreateFontW(-MulDiv(22, static_cast<int>(g.dpi), 72), 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Segoe UI");
    HFONT newBody = CreateFontW(-MulDiv(11, static_cast<int>(g.dpi), 72), 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Segoe UI");
    HFONT newTimer = CreateFontW(-MulDiv(38, static_cast<int>(g.dpi), 72), 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Segoe UI");
    if (!newTitle || !newBody || !newTimer) {
        if (newTitle) DeleteObject(newTitle);
        if (newBody) DeleteObject(newBody);
        if (newTimer) DeleteObject(newTimer);
        return;
    }
    const HFONT oldTitle = g.titleFont;
    const HFONT oldBody = g.bodyFont;
    const HFONT oldTimer = g.timerFont;
    g.titleFont = newTitle;
    g.bodyFont = newBody;
    g.timerFont = newTimer;
    applyFont(g.title, g.titleFont);
    applyFont(g.timer, g.timerFont);
    for (HWND control : {g.subtitle, g.targetLabel, g.restLabel, g.targetInput, g.restInput, g.preset30, g.preset60,
            g.preset90, g.delayFirst, g.mute, g.soundLabel, g.soundCombo, g.volumeLabel, g.startButton, g.progress,
            g.status, g.completeButton, g.pauseButton, g.skipButton, g.resetButton, g.cancelButton}) {
        applyFont(control, g.bodyFont);
    }
    if (oldTitle) DeleteObject(oldTitle);
    if (oldBody) DeleteObject(oldBody);
    if (oldTimer) DeleteObject(oldTimer);
}

void move(HWND control, int x, int y, int width, int height) {
    if (control) MoveWindow(control, scaled(x), scaled(y), scaled(width), scaled(height), TRUE);
}

void layoutControls(int widthPixels, int heightPixels) {
    const int width = MulDiv(widthPixels, 96, static_cast<int>(g.dpi));
    const int height = MulDiv(heightPixels, 96, static_cast<int>(g.dpi));
    constexpr int margin = 32;
    constexpr int gap = 16;
    const int content = std::max(500, width - margin * 2);
    const int half = (content - gap) / 2;
    const int bottomRow = height - 50;
    const int primaryRow = bottomRow - 54;
    const int quarter = (content - gap * 3) / 4;

    move(g.title, margin, 20, content, 40);
    move(g.subtitle, margin, 61, content, 24);
    move(g.targetLabel, margin, 100, half, 22);
    move(g.restLabel, margin + half + gap, 100, half, 22);
    move(g.targetInput, margin, 126, half, 36);
    move(g.restInput, margin + half + gap, 126, half, 36);
    move(g.preset30, margin + half + gap, 169, 72, 30);
    move(g.preset60, margin + half + gap + 80, 169, 72, 30);
    move(g.preset90, margin + half + gap + 160, 169, 72, 30);
    move(g.delayFirst, margin, 169, half, 30);
    move(g.mute, margin, 211, 100, 30);
    move(g.soundLabel, margin + 112, 215, 52, 22);
    move(g.soundCombo, margin + 164, 209, 150, 200);
    move(g.volumeLabel, margin + 330, 215, 58, 22);
    move(g.volumeSlider, margin + 390, 207, std::max(100, content - 390), 34);
    move(g.startButton, margin, 254, content, 42);
    move(g.progress, margin, 318, content, 26);
    move(g.timer, margin, 350, content, 72);
    move(g.status, margin, 428, content, 28);
    move(g.completeButton, margin, primaryRow, content, 42);
    move(g.pauseButton, margin, bottomRow, quarter, 36);
    move(g.skipButton, margin + quarter + gap, bottomRow, quarter, 36);
    move(g.resetButton, margin + (quarter + gap) * 2, bottomRow, quarter, 36);
    move(g.cancelButton, margin + (quarter + gap) * 3, bottomRow, quarter, 36);
}

void enableSetup(bool enabled) {
    for (HWND control : {g.targetInput, g.restInput, g.preset30, g.preset60, g.preset90, g.delayFirst, g.startButton})
        EnableWindow(control, enabled);
}

void refreshUi() {
    using rep_counter::WorkoutState;
    const auto state = g.session.state();
    const bool setup = state == WorkoutState::Setup;
    const bool resting = state == WorkoutState::Resting;
    const bool paused = state == WorkoutState::Paused;
    const bool ready = state == WorkoutState::Ready;
    const bool complete = state == WorkoutState::Complete;
    enableSetup(setup);
    EnableWindow(g.completeButton, ready);
    EnableWindow(g.pauseButton, resting || paused);
    EnableWindow(g.skipButton, resting || paused);
    EnableWindow(g.resetButton, !setup);
    EnableWindow(g.cancelButton, !setup && !complete);
    setText(g.pauseButton, paused ? L"Resume" : L"Pause");
    setText(g.progress, setup ? L"SET UP YOUR WORKOUT" :
        L"REP " + std::to_wstring(g.session.completed()) + L" / " + std::to_wstring(g.session.target()));

    if (setup) {
        KillTimer(g.window, kTimerId);
        setText(g.timer, L"--:--");
        setText(g.status, L"Choose a target and rest time. The first rep starts immediately.");
    } else if (resting) {
        setText(g.timer, formatTime(g.session.remainingSeconds()));
        setText(g.status, L"Resting before rep " + std::to_wstring(g.session.completed() + 1));
        if (SetTimer(g.window, kTimerId, 100, nullptr) == 0)
            setText(g.status, L"The system timer could not start. Use Skip rest or New workout.");
    } else if (paused) {
        KillTimer(g.window, kTimerId);
        setText(g.timer, formatTime(g.session.remainingSeconds()));
        setText(g.status, L"Rest paused before rep " + std::to_wstring(g.session.completed() + 1));
    } else if (ready) {
        KillTimer(g.window, kTimerId);
        setText(g.timer, L"GO");
        setText(g.status, L"Time for rep " + std::to_wstring(g.session.completed() + 1));
        SetFocus(g.completeButton);
    } else if (complete) {
        KillTimer(g.window, kTimerId);
        setText(g.timer, L"DONE");
        setText(g.status, L"Workout complete. Great work!");
        SetFocus(g.resetButton);
    }
    InvalidateRect(g.window, nullptr, TRUE);
}

void startWorkout() {
    const int target = getInteger(g.targetInput, 10000);
    const int restSeconds = getInteger(g.restInput, 3600);
    if (target <= 0 || restSeconds < 0) {
        setText(g.status, L"Enter 1-10,000 reps and a rest time from 0-3,600 seconds.");
        MessageBeep(MB_ICONWARNING);
        return;
    }
    const bool delayFirst = SendMessageW(g.delayFirst, BM_GETCHECK, 0, 0) == BST_CHECKED;
    if (g.session.start(target, restSeconds, delayFirst)) {
        refreshUi();
        if (g.session.state() == rep_counter::WorkoutState::Ready) playAlert();
    }
}

void completeRep() {
    if (!g.session.completeRep()) return;
    const bool complete = g.session.state() == rep_counter::WorkoutState::Complete;
    refreshUi();
    if (complete) playAlert(true);
    else if (g.session.state() == rep_counter::WorkoutState::Ready) playAlert();
}

void togglePause() {
    bool becameReady = false;
    if (g.session.state() == rep_counter::WorkoutState::Resting) {
        g.session.pause();
        becameReady = g.session.state() == rep_counter::WorkoutState::Ready;
    }
    else if (g.session.state() == rep_counter::WorkoutState::Paused) g.session.resume();
    refreshUi();
    if (becameReady) playAlert();
}

void skipRest() {
    if (g.session.skipRest()) {
        refreshUi();
        playAlert();
    }
}

void resetWorkout() {
    PlaySoundW(nullptr, nullptr, 0);
    g.session.reset();
    refreshUi();
    SetFocus(g.targetInput);
}

void primaryAction() {
    if (g.session.state() == rep_counter::WorkoutState::Setup) startWorkout();
    else if (g.session.state() == rep_counter::WorkoutState::Ready) completeRep();
    else if (g.session.state() == rep_counter::WorkoutState::Complete) resetWorkout();
}

HWND addControl(DWORD extendedStyle, const wchar_t* className, const wchar_t* text, DWORD style, int id) {
    HWND control = CreateWindowExW(extendedStyle, className, text, WS_CHILD | WS_VISIBLE | style,
        0, 0, 0, 0, g.window, reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)), g.instance, nullptr);
    if (control) applyFont(control, g.bodyFont);
    return control;
}

void handleCommand(int id) {
    switch (id) {
    case kStartButton: startWorkout(); break;
    case kCompleteButton: completeRep(); break;
    case kPauseButton: togglePause(); break;
    case kSkipButton: skipRest(); break;
    case kResetButton: resetWorkout(); break;
    case kCancelButton: resetWorkout(); break;
    case kPrimaryAction: primaryAction(); break;
    case kPreset30: setText(g.restInput, L"30"); break;
    case kPreset60: setText(g.restInput, L"60"); break;
    case kPreset90: setText(g.restInput, L"90"); break;
    default: break;
    }
}

LRESULT CALLBACK windowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
    case WM_COMMAND:
        handleCommand(LOWORD(wParam));
        return 0;
    case WM_TIMER:
        if (wParam == kTimerId && g.session.state() == rep_counter::WorkoutState::Resting) {
            if (g.session.tick()) {
                refreshUi();
                playAlert();
            } else {
                setText(g.timer, formatTime(g.session.remainingSeconds()));
            }
        }
        return 0;
    case WM_GETMINMAXINFO: {
        auto* info = reinterpret_cast<MINMAXINFO*>(lParam);
        info->ptMinTrackSize = {scaled(700), scaled(640)};
        return 0;
    }
    case WM_SIZE:
        if (g.targetInput && wParam != SIZE_MINIMIZED) layoutControls(LOWORD(lParam), HIWORD(lParam));
        return 0;
    case WM_DPICHANGED: {
        g.dpi = HIWORD(wParam);
        const auto* suggested = reinterpret_cast<RECT*>(lParam);
        SetWindowPos(window, nullptr, suggested->left, suggested->top,
            suggested->right - suggested->left, suggested->bottom - suggested->top,
            SWP_NOACTIVATE | SWP_NOZORDER);
        createFonts();
        RECT client{};
        GetClientRect(window, &client);
        layoutControls(client.right, client.bottom);
        return 0;
    }
    case WM_CTLCOLOREDIT:
    case WM_CTLCOLORLISTBOX: {
        HDC dc = reinterpret_cast<HDC>(wParam);
        SetTextColor(dc, kText);
        SetBkColor(dc, kSurface);
        return reinterpret_cast<LRESULT>(g.surfaceBrush);
    }
    case WM_CTLCOLORSTATIC:
    case WM_CTLCOLORBTN: {
        HDC dc = reinterpret_cast<HDC>(wParam);
        const HWND control = reinterpret_cast<HWND>(lParam);
        const bool prominent = control == g.title || control == g.timer || control == g.progress ||
            control == g.delayFirst || control == g.mute;
        SetTextColor(dc, prominent ? kText : kMutedText);
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
        const auto oldFont = SelectObject(draw->hDC, g.bodyFont);
        wchar_t text[64]{};
        GetWindowTextW(draw->hwndItem, text, static_cast<int>(std::size(text)));
        RECT textRect = draw->rcItem;
        DrawTextW(draw->hDC, text, -1, &textRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
        if ((draw->itemState & ODS_FOCUS) != 0) {
            RECT focus = draw->rcItem;
            InflateRect(&focus, -scaled(3), -scaled(3));
            DrawFocusRect(draw->hDC, &focus);
        }
        SelectObject(draw->hDC, oldFont);
        return TRUE;
    }
    case WM_DESTROY:
        KillTimer(window, kTimerId);
        PlaySoundW(nullptr, nullptr, 0);
        if (g.accelerators) DestroyAcceleratorTable(g.accelerators);
        if (g.titleFont) DeleteObject(g.titleFont);
        if (g.bodyFont) DeleteObject(g.bodyFont);
        if (g.timerFont) DeleteObject(g.timerFont);
        if (g.backgroundBrush) DeleteObject(g.backgroundBrush);
        if (g.surfaceBrush) DeleteObject(g.surfaceBrush);
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(window, message, wParam, lParam);
}

bool registerWindowClass() {
    WNDCLASSEXW windowClass{sizeof(WNDCLASSEXW)};
    windowClass.hInstance = g.instance;
    windowClass.lpszClassName = L"RepCounterWindow";
    windowClass.lpfnWndProc = windowProc;
    windowClass.hCursor = LoadCursor(nullptr, IDC_ARROW);
    windowClass.hbrBackground = g.backgroundBrush;
    windowClass.hIcon = LoadIconW(g.instance, MAKEINTRESOURCEW(1));
    windowClass.hIconSm = LoadIconW(g.instance, MAKEINTRESOURCEW(1));
    return RegisterClassExW(&windowClass) != 0;
}

bool createMainWindow(int showCommand) {
    const std::wstring title = std::wstring(L"Rep Counter v") + REP_COUNTER_VERSION_W;
    g.window = CreateWindowExW(0, L"RepCounterWindow", title.c_str(), WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, scaled(760), scaled(720), nullptr, nullptr, g.instance, nullptr);
    if (!g.window) return false;
    const BOOL darkMode = TRUE;
    DwmSetWindowAttribute(g.window, 20, &darkMode, sizeof(darkMode));

    g.title = addControl(0, L"STATIC", L"REP COUNTER", SS_LEFT, 0);
    g.subtitle = addControl(0, L"STATIC", L"v" REP_COUNTER_VERSION_W L"  •  A focused workout timer", SS_LEFT, 0);
    g.targetLabel = addControl(0, L"STATIC", L"Rep target (1-10,000)", SS_LEFT, 0);
    g.restLabel = addControl(0, L"STATIC", L"Rest between reps (seconds)", SS_LEFT, 0);
    g.targetInput = addControl(WS_EX_CLIENTEDGE, L"EDIT", L"10", ES_CENTER | ES_NUMBER | WS_TABSTOP, kTargetInput);
    g.restInput = addControl(WS_EX_CLIENTEDGE, L"EDIT", L"60", ES_CENTER | ES_NUMBER | WS_TABSTOP, kRestInput);
    g.preset30 = addControl(0, L"BUTTON", L"30 sec", BS_OWNERDRAW | WS_TABSTOP, kPreset30);
    g.preset60 = addControl(0, L"BUTTON", L"60 sec", BS_OWNERDRAW | WS_TABSTOP, kPreset60);
    g.preset90 = addControl(0, L"BUTTON", L"90 sec", BS_OWNERDRAW | WS_TABSTOP, kPreset90);
    g.delayFirst = addControl(0, L"BUTTON", L"Rest before first rep", BS_AUTOCHECKBOX | WS_TABSTOP, kDelayFirstCheck);
    g.mute = addControl(0, L"BUTTON", L"Mute", BS_AUTOCHECKBOX | WS_TABSTOP, kMuteCheck);
    SetWindowTheme(g.delayFirst, L"", L"");
    SetWindowTheme(g.mute, L"", L"");
    g.soundLabel = addControl(0, L"STATIC", L"Sound", SS_LEFT, 0);
    g.soundCombo = addControl(0, WC_COMBOBOXW, L"", CBS_DROPDOWNLIST | WS_VSCROLL | WS_TABSTOP, kSoundCombo);
    g.volumeLabel = addControl(0, L"STATIC", L"Volume", SS_LEFT, 0);
    g.volumeSlider = addControl(0, TRACKBAR_CLASSW, L"", TBS_AUTOTICKS | WS_TABSTOP, kVolumeSlider);
    g.startButton = addControl(0, L"BUTTON", L"Start workout", BS_OWNERDRAW | WS_TABSTOP, kStartButton);
    g.progress = addControl(0, L"STATIC", L"SET UP YOUR WORKOUT", SS_CENTER, 0);
    g.timer = addControl(0, L"STATIC", L"--:--", SS_CENTER, 0);
    g.status = addControl(0, L"STATIC", L"", SS_CENTER, 0);
    g.completeButton = addControl(0, L"BUTTON", L"Complete rep", BS_OWNERDRAW | WS_TABSTOP, kCompleteButton);
    g.pauseButton = addControl(0, L"BUTTON", L"Pause", BS_OWNERDRAW | WS_TABSTOP, kPauseButton);
    g.skipButton = addControl(0, L"BUTTON", L"Skip rest", BS_OWNERDRAW | WS_TABSTOP, kSkipButton);
    g.resetButton = addControl(0, L"BUTTON", L"New workout", BS_OWNERDRAW | WS_TABSTOP, kResetButton);
    g.cancelButton = addControl(0, L"BUTTON", L"Cancel", BS_OWNERDRAW | WS_TABSTOP, kCancelButton);

    const std::array<HWND, 23> required{{g.title, g.subtitle, g.targetLabel, g.restLabel, g.targetInput, g.restInput,
        g.preset30, g.preset60, g.preset90, g.delayFirst, g.mute, g.soundLabel, g.soundCombo, g.volumeLabel,
        g.volumeSlider, g.startButton, g.progress, g.timer, g.status, g.completeButton, g.pauseButton, g.skipButton,
        g.resetButton}};
    if (std::any_of(required.begin(), required.end(), [](HWND control) { return control == nullptr; }) || !g.cancelButton) return false;

    SendMessageW(g.targetInput, EM_SETLIMITTEXT, 5, 0);
    SendMessageW(g.restInput, EM_SETLIMITTEXT, 4, 0);
    SetWindowTheme(g.targetInput, L"DarkMode_Explorer", nullptr);
    SetWindowTheme(g.restInput, L"DarkMode_Explorer", nullptr);
    SendMessageW(g.soundCombo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"Gentle"));
    SendMessageW(g.soundCombo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"Strong"));
    SendMessageW(g.soundCombo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"Classic"));
    SendMessageW(g.soundCombo, CB_SETCURSEL, 0, 0);
    SendMessageW(g.volumeSlider, TBM_SETRANGE, TRUE, MAKELONG(0, 100));
    SendMessageW(g.volumeSlider, TBM_SETPOS, TRUE, 70);
    SendMessageW(g.volumeSlider, TBM_SETTICFREQ, 10, 0);
    createFonts();

    const ACCEL acceleratorEntries[]{{FVIRTKEY, VK_RETURN, kPrimaryAction},
        {FVIRTKEY, VK_ESCAPE, kCancelButton},
        {FCONTROL | FVIRTKEY, 'N', kResetButton},
        {FCONTROL | FVIRTKEY, 'P', kPauseButton},
        {FCONTROL | FVIRTKEY, 'S', kSkipButton}};
    g.accelerators = CreateAcceleratorTableW(const_cast<LPACCEL>(acceleratorEntries), static_cast<int>(std::size(acceleratorEntries)));
    if (!g.accelerators) return false;

    RECT client{};
    GetClientRect(g.window, &client);
    layoutControls(client.right, client.bottom);
    refreshUi();
    ShowWindow(g.window, showCommand);
    UpdateWindow(g.window);
    return true;
}
} // namespace

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, PWSTR, int showCommand) {
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
    INITCOMMONCONTROLSEX commonControls{sizeof(INITCOMMONCONTROLSEX), ICC_STANDARD_CLASSES | ICC_BAR_CLASSES};
    if (!InitCommonControlsEx(&commonControls)) {
        MessageBoxW(nullptr, L"Windows common controls could not be initialized.", L"Rep Counter", MB_OK | MB_ICONERROR);
        return 1;
    }

    g.instance = instance;
    g.dpi = GetDpiForSystem();
    g.backgroundBrush = CreateSolidBrush(kBackground);
    g.surfaceBrush = CreateSolidBrush(kSurface);
    createFonts();
    if (!g.backgroundBrush || !g.surfaceBrush || !g.titleFont || !g.bodyFont || !g.timerFont || !registerWindowClass()) {
        MessageBoxW(nullptr, L"Rep Counter could not initialize its window resources.", L"Rep Counter", MB_OK | MB_ICONERROR);
        return 1;
    }
    if (!createMainWindow(showCommand)) {
        MessageBoxW(nullptr, L"Rep Counter could not create all required controls.", L"Rep Counter", MB_OK | MB_ICONERROR);
        if (g.window) DestroyWindow(g.window);
        return 1;
    }

    MSG message{};
    BOOL result{};
    while ((result = GetMessageW(&message, nullptr, 0, 0)) > 0) {
        if (!TranslateAcceleratorW(g.window, g.accelerators, &message) && !IsDialogMessageW(g.window, &message)) {
            TranslateMessage(&message);
            DispatchMessageW(&message);
        }
    }
    return result == -1 ? 1 : static_cast<int>(message.wParam);
}
