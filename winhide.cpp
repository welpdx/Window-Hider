#include <iostream>
#include <list>
#include <windows.h>

#define HOTKEY_HIDE 1
#define HOTKEY_SHOW 2
#define KEY_B 0x42
#define KEY_C 0x43

using namespace std;

void main() {
    // ensure only one instance is running
    HANDLE current_mutex = CreateMutexA(NULL, true, "Window hider");
    DWORD last_error = GetLastError();
    if (last_error == ERROR_ALREADY_EXISTS)
    {
        if (current_mutex != 0)
            CloseHandle(current_mutex);
        return;
    }

    MSG msg = { 0 };
    list<HWND> windows;
    SetConsoleTitleA("Window Hider");
    cout << "Window Hider v1.0" << endl;
    if (RegisterHotKey(NULL, HOTKEY_HIDE, MOD_ALT | MOD_NOREPEAT, KEY_B) &&
        RegisterHotKey(NULL, HOTKEY_SHOW, MOD_ALT | MOD_NOREPEAT, KEY_C)) {
        cout << "Alt+B (hide window) and Alt-C (show window) hotkeys registered" << endl;
    }
    else {
        cout << "Failed to register hotkeys" << endl;
    }

    while (GetMessage(&msg, NULL, 0, 0) != 0) {
        if (msg.message == WM_HOTKEY && msg.wParam == HOTKEY_HIDE) {
            HWND handle = GetForegroundWindow();
            if (!windows.empty() && windows.front() == handle)
                continue; // prevent rehiding the same window
            char title[BUFSIZ];
            ShowWindow(handle, SW_HIDE);
            if (GetWindowTextLength(handle) == 0)
                cout << "The window: " << handle << " was hidden" << endl;
            else {
                GetWindowTextA(handle, title, GetWindowTextLength(handle) + 1);
                cout << "The window: \"" << title << "\" was hidden" << endl;
            }
            windows.push_front(handle);
        }
        if (msg.message == WM_HOTKEY && msg.wParam == HOTKEY_SHOW && !windows.empty()) {
            ShowWindow(windows.front(), SW_SHOW);
            windows.pop_front();
        }
    }
}