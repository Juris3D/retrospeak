// =====================================================================
//  RetroSpeak - the Windows app: type, hear it, save it as a WAV.
//  With arguments it's the command line instead (retrospeak --help).
// =====================================================================
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <commctrl.h>
#include <commdlg.h>
#include <mmsystem.h>
#include <cstdio>
#include <string>
#include <vector>
#include "retrospeak.h"

int rsCli(int argc, char** argv);

namespace {
enum { ID_TEXT = 101, ID_PH, ID_USEPH, ID_MALE, ID_FEMALE, ID_E1982, ID_EFUT, ID_PITCH, ID_SPEED, ID_BITS, ID_SPEAK, ID_STOP, ID_SAVE,
       ID_PITCHV, ID_SPEEDV, ID_BITSV, ID_HELP, ID_STATUS };
HWND hText, hPh, hUsePh, hMale, hFemale, h1982, hFut, hPitch, hSpeed, hBits, hPitchV, hSpeedV, hBitsV, hStatus, hBitsL;
std::vector<unsigned char> playing;                         // (the WAV being played: it must live while it plays)
HFONT font, mono;

std::string getText(HWND h) { int n = GetWindowTextLengthA(h); std::string s(n + 1, '\0'); GetWindowTextA(h, &s[0], n + 1); s.resize(n); return s; }
RsVoice voice() {
    RsVoice v;
    v.female = SendMessageA(hFemale, BM_GETCHECK, 0, 0) == BST_CHECKED;
    v.engine = SendMessageA(hFut, BM_GETCHECK, 0, 0) == BST_CHECKED ? RS_FUTURE : RS_1982;
    v.pitch = (float)SendMessageA(hPitch, TBM_GETPOS, 0, 0);
    v.speed = SendMessageA(hSpeed, TBM_GETPOS, 0, 0) / 100.0f;
    v.bits = (int)SendMessageA(hBits, TBM_GETPOS, 0, 0);
    return v;
}
void refreshLabels() {
    char b[32];
    snprintf(b, sizeof b, "%+d", (int)SendMessageA(hPitch, TBM_GETPOS, 0, 0)); SetWindowTextA(hPitchV, b);
    snprintf(b, sizeof b, "%d %%", (int)SendMessageA(hSpeed, TBM_GETPOS, 0, 0)); SetWindowTextA(hSpeedV, b);
    snprintf(b, sizeof b, "%d", (int)SendMessageA(hBits, TBM_GETPOS, 0, 0)); SetWindowTextA(hBitsV, b);
    const bool chip = SendMessageA(h1982, BM_GETCHECK, 0, 0) == BST_CHECKED;
    EnableWindow(hBits, chip); EnableWindow(hBitsV, chip); EnableWindow(hBitsL, chip);
}
void textToPh() { if (SendMessageA(hUsePh, BM_GETCHECK, 0, 0) != BST_CHECKED) SetWindowTextA(hPh, rsTextToPhonemes(getText(hText)).c_str()); }
std::vector<float> render() {
    std::string ph = SendMessageA(hUsePh, BM_GETCHECK, 0, 0) == BST_CHECKED ? getText(hPh) : rsTextToPhonemes(getText(hText));
    for (char& c : ph) if (c == '\r' || c == '\n') c = ' ';
    return rsSpeak(ph, voice());
}
void speak() {
    PlaySoundA(nullptr, nullptr, 0);
    auto s = render();
    if (s.empty()) { SetWindowTextA(hStatus, "nothing to say"); return; }
    playing = rsWav(s);
    PlaySoundA((LPCSTR)playing.data(), nullptr, SND_MEMORY | SND_ASYNC | SND_NODEFAULT);
    char b[64]; snprintf(b, sizeof b, "speaking - %.1f s", s.size() / (float)RS_RATE); SetWindowTextA(hStatus, b);
}
void save(HWND w) {
    char file[MAX_PATH] = "retrospeak.wav";
    OPENFILENAMEA o = {}; o.lStructSize = sizeof o; o.hwndOwner = w; o.lpstrFilter = "WAV sound (*.wav)\0*.wav\0All files\0*.*\0";
    o.lpstrFile = file; o.nMaxFile = MAX_PATH; o.lpstrDefExt = "wav"; o.Flags = OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST;
    if (!GetSaveFileNameA(&o)) return;
    auto s = render(); auto wav = rsWav(s);
    FILE* f = fopen(file, "wb");
    if (!f) { MessageBoxA(w, "Can't write that file.", "RetroSpeak", MB_ICONERROR); return; }
    fwrite(wav.data(), 1, wav.size(), f); fclose(f);
    std::string m = std::string("saved: ") + file; SetWindowTextA(hStatus, m.c_str());
}

HWND ctl(HWND p, const char* cls, const char* txt, DWORD st, int x, int y, int w, int h, int id, HFONT f = nullptr) {
    HWND c = CreateWindowExA(!strcmp(cls, "EDIT") ? WS_EX_CLIENTEDGE : 0, cls, txt, WS_CHILD | WS_VISIBLE | st, x, y, w, h, p, (HMENU)(INT_PTR)id, GetModuleHandleA(nullptr), nullptr);
    SendMessageA(c, WM_SETFONT, (WPARAM)(f ? f : font), TRUE);
    return c;
}
HWND bar(HWND p, int x, int y, int w, int id, int lo, int hi, int v) {
    HWND c = CreateWindowExA(0, TRACKBAR_CLASSA, "", WS_CHILD | WS_VISIBLE | TBS_HORZ | TBS_NOTICKS, x, y, w, 26, p, (HMENU)(INT_PTR)id, GetModuleHandleA(nullptr), nullptr);
    SendMessageA(c, TBM_SETRANGE, TRUE, MAKELPARAM(lo, hi)); SendMessageA(c, TBM_SETPOS, TRUE, v); return c;
}

LRESULT CALLBACK proc(HWND w, UINT m, WPARAM wp, LPARAM lp) {
    switch (m) {
    case WM_CREATE: {
        font = CreateFontA(-15, 0, 0, 0, FW_NORMAL, 0, 0, 0, ANSI_CHARSET, 0, 0, CLEARTYPE_QUALITY, 0, "Segoe UI");
        mono = CreateFontA(-15, 0, 0, 0, FW_NORMAL, 0, 0, 0, ANSI_CHARSET, 0, 0, CLEARTYPE_QUALITY, FIXED_PITCH, "Consolas");
        ctl(w, "STATIC", "Text to say:", 0, 12, 10, 300, 20, 0);
        hText = ctl(w, "EDIT", "Missiles reloaded. Ship repaired. Hello, I am a computer. Do you want to play a game?",
                    ES_MULTILINE | ES_AUTOVSCROLL | WS_VSCROLL | WS_TABSTOP, 12, 30, 656, 96, ID_TEXT);
        ctl(w, "STATIC", "Phonemes (what it will say - tick the box to edit them by hand):", 0, 12, 134, 520, 20, 0);
        hPh = ctl(w, "EDIT", "", ES_MULTILINE | ES_AUTOVSCROLL | WS_VSCROLL | WS_TABSTOP, 12, 154, 656, 76, ID_PH, mono);
        hUsePh = ctl(w, "BUTTON", "Speak the phonemes as written (not the text)", BS_AUTOCHECKBOX | WS_TABSTOP, 12, 234, 400, 22, ID_USEPH);
        ctl(w, "BUTTON", "Voice", BS_GROUPBOX, 12, 262, 150, 78, 0);
        hMale = ctl(w, "BUTTON", "Male robot", BS_AUTORADIOBUTTON | WS_GROUP | WS_TABSTOP, 24, 284, 130, 22, ID_MALE);
        hFemale = ctl(w, "BUTTON", "Female robot", BS_AUTORADIOBUTTON, 24, 308, 130, 22, ID_FEMALE);
        ctl(w, "BUTTON", "Engine", BS_GROUPBOX, 172, 262, 220, 78, 0);
        h1982 = ctl(w, "BUTTON", "1982 speech chip", BS_AUTORADIOBUTTON | WS_GROUP | WS_TABSTOP, 184, 284, 200, 22, ID_E1982);
        hFut = ctl(w, "BUTTON", "Retro-future (fictional)", BS_AUTORADIOBUTTON, 184, 308, 200, 22, ID_EFUT);
        SendMessageA(hMale, BM_SETCHECK, BST_CHECKED, 0); SendMessageA(h1982, BM_SETCHECK, BST_CHECKED, 0);
        ctl(w, "STATIC", "Pitch", 0, 404, 266, 50, 20, 0); hPitch = bar(w, 452, 262, 160, ID_PITCH, -12, 12, 0); hPitchV = ctl(w, "STATIC", "", 0, 618, 266, 50, 20, ID_PITCHV);
        ctl(w, "STATIC", "Speed", 0, 404, 292, 50, 20, 0); hSpeed = bar(w, 452, 288, 160, ID_SPEED, 50, 200, 100); hSpeedV = ctl(w, "STATIC", "", 0, 618, 292, 50, 20, ID_SPEEDV);
        hBitsL = ctl(w, "STATIC", "Bits", 0, 404, 318, 50, 20, 0); hBits = bar(w, 452, 314, 160, ID_BITS, 3, 8, 6); hBitsV = ctl(w, "STATIC", "", 0, 618, 318, 50, 20, ID_BITSV);
        ctl(w, "BUTTON", "Speak  (F5)", BS_DEFPUSHBUTTON | WS_TABSTOP, 12, 352, 140, 34, ID_SPEAK);
        ctl(w, "BUTTON", "Stop", WS_TABSTOP, 160, 352, 90, 34, ID_STOP);
        ctl(w, "BUTTON", "Save WAV...", WS_TABSTOP, 258, 352, 130, 34, ID_SAVE);
        hStatus = ctl(w, "STATIC", "", 0, 404, 360, 264, 22, ID_STATUS);
        ctl(w, "STATIC", rsPhonemeHelp(), 0, 12, 398, 656, 118, ID_HELP);
        textToPh(); refreshLabels();
        return 0; }
    case WM_COMMAND:
        switch (LOWORD(wp)) {
        case ID_TEXT: if (HIWORD(wp) == EN_CHANGE) textToPh(); break;
        case ID_USEPH: textToPh(); break;
        case ID_SPEAK: speak(); break;
        case ID_STOP: PlaySoundA(nullptr, nullptr, 0); SetWindowTextA(hStatus, ""); break;
        case ID_SAVE: save(w); break;
        case ID_E1982: case ID_EFUT: refreshLabels(); break;
        }
        return 0;
    case WM_HSCROLL: refreshLabels(); return 0;
    case WM_CTLCOLORSTATIC: { HDC dc = (HDC)wp; SetBkMode(dc, TRANSPARENT); return (LRESULT)GetSysColorBrush(COLOR_BTNFACE); }
    case WM_DESTROY: PlaySoundA(nullptr, nullptr, 0); PostQuitMessage(0); return 0;
    }
    return DefWindowProcA(w, m, wp, lp);
}
}

int WINAPI WinMain(HINSTANCE inst, HINSTANCE, LPSTR, int show) {
    if (__argc > 1) {                                       // the COMMAND LINE: its output to the console it was started from
        if (AttachConsole(ATTACH_PARENT_PROCESS)) { freopen("CONOUT$", "w", stdout); freopen("CONOUT$", "w", stderr); printf("\n"); }
        return rsCli(__argc, __argv);
    }
    INITCOMMONCONTROLSEX ic = {sizeof ic, ICC_BAR_CLASSES}; InitCommonControlsEx(&ic);
    WNDCLASSA wc = {}; wc.lpfnWndProc = proc; wc.hInstance = inst; wc.lpszClassName = "RetroSpeak";
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW); wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1); wc.hIcon = LoadIconA(inst, "APPICON");
    RegisterClassA(&wc);
    RECT r = {0, 0, 680, 524}; AdjustWindowRect(&r, WS_OVERLAPPEDWINDOW & ~WS_THICKFRAME & ~WS_MAXIMIZEBOX, FALSE);
    HWND w = CreateWindowA("RetroSpeak", "RetroSpeak - an early-80s speech synthesiser", WS_OVERLAPPEDWINDOW & ~WS_THICKFRAME & ~WS_MAXIMIZEBOX,
                           CW_USEDEFAULT, CW_USEDEFAULT, r.right - r.left, r.bottom - r.top, nullptr, nullptr, inst, nullptr);
    ShowWindow(w, show);
    MSG msg;
    while (GetMessageA(&msg, nullptr, 0, 0)) {
        if (msg.message == WM_KEYDOWN && msg.wParam == VK_F5) { speak(); continue; }
        if (IsDialogMessageA(w, &msg)) continue;
        TranslateMessage(&msg); DispatchMessageA(&msg);
    }
    return 0;
}
