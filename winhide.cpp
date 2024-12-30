#include <iostream>
#include <string>
#include <list>
#include <windows.h>
#include "winhide.h"
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "kernel32.lib")

std::list<HWND> windows;
bool are_windows_hidden = false;

// reads the specified int from an ini file and returns it
char ReadIniInt(LPCSTR category, LPCSTR key, int default_value, const char* ini_path) {
    int value = KEY_INVALID;
    int ini_value = GetPrivateProfileIntA(category, key, default_value, ini_path);
    auto last_error = GetLastError();
    if (last_error == false || last_error == ERROR_MORE_DATA)
        value = ini_value;
    return value;
}

// reads the specified string from an ini file and returns it
char ReadIniString(LPCSTR category, LPCSTR key, int default_value, const char* ini_path) {
    char value = '\0';
    LPSTR ini_value = new CHAR[BUFSIZ];
    GetPrivateProfileStringA(category, key, LPCSTR(default_value), ini_value, 2, ini_path);
    auto last_error = GetLastError();
    if (last_error == false || last_error == ERROR_MORE_DATA)
        value = (char)VkKeyScanExA(ini_value[0], GetKeyboardLayout(0));
    return value;
}

// ensures only a single instance is run
void SingleInstance() {
    HANDLE current_mutex = CreateMutexA(NULL, true, "Window Hider");
    if (current_mutex != 0 && GetLastError() == ERROR_ALREADY_EXISTS) {
        CloseHandle(current_mutex);
        exit(EXIT_FAILURE);
    }
}

// modifies the window state of the given handle
void WindowState(HWND handle, int state, std::string action_str) {
    ShowWindow(handle, state);
    if (GetWindowTextLengthA(handle) == 0)
        std::cout << action_str << ": " << handle << std::endl;
    else {
        char title[BUFSIZ];
        if (GetWindowTextA(handle, title, GetWindowTextLengthA(handle) + 1) == 0)
            std::cout << "Error getting window title: " << GetLastError() << std::endl;

        std::cout << action_str << ": \"" << title << "\"" << std::endl;
    }
}

// handles the Windows program exiting event to unhide all windows
BOOL WINAPI ExitHandler(DWORD type) {
    if (type == CTRL_C_EVENT || type == CTRL_CLOSE_EVENT) {
        for (HWND window : windows) {
            WindowState(window, SW_SHOW, "Window shown");
            windows.pop_front();
        }
        UnregisterHotKey(NULL, HOTKEY_HIDE);
        UnregisterHotKey(NULL, HOTKEY_TOGGLE);
        UnregisterHotKey(NULL, HOTKEY_FORGET);

        exit(EXIT_SUCCESS);
    }
    return true;
}

// determines if given file exists
BOOL FileExists(std::string path) {
    DWORD attrib = GetFileAttributesA(path.c_str());
    return (attrib != INVALID_FILE_ATTRIBUTES && !(attrib & FILE_ATTRIBUTE_DIRECTORY));
}

int main() {
    SetConsoleTitleA("Window Hider");
    std::cout << "Window Hider v" << VERSION << std::endl;
    SingleInstance();
    SetConsoleCtrlHandler(ExitHandler, TRUE);
    int start_hidden = 0,
        toggle_modifiers = NO_MOD,
        hide_modifiers = NO_MOD,
        forget_modifiers = NO_MOD;
    char hide_key = KEY_C,
         toggle_key = KEY_V,
         forget_key = KEY_X,
         cwd[BUFSIZ];
    MSG msg = { FALSE };
    const std::list<std::string> exclusions = {
        CLASS_DESKTOP,
        CLASS_DESKTOP_LAYER,
        CLASS_TASKBAR,
        CLASS_MULTITASKBAR,
        CLASS_START_MENU,
        CLASS_NOTIFY_PANEL
    };

    if (GetCurrentDirectoryA(BUFSIZ, cwd) == FALSE) {
        std::cerr << "Could not obtain current working directory, exiting" << std::endl;
        exit(EXIT_FAILURE);
    }
    std::string ini_path = std::string(cwd) + "\\winhidecfg.ini";

    if (FileExists(ini_path)) {
        start_hidden = ReadIniInt("Settings", "StartHidden", FALSE, ini_path.c_str());

        if (ReadIniInt("HideHotKey", "Alt", NO_MOD, ini_path.c_str()) == TRUE)
            hide_modifiers |= MOD_ALT;
        if (ReadIniInt("HideHotKey", "Ctrl", NO_MOD, ini_path.c_str()) == TRUE)
            hide_modifiers |= MOD_CONTROL;
        if (ReadIniInt("HideHotKey", "Shift", NO_MOD, ini_path.c_str()) == TRUE)
            hide_modifiers |= MOD_SHIFT;
        if (ReadIniInt("HideHotKey", "WinKey", NO_MOD, ini_path.c_str()) == TRUE)
            hide_modifiers |= MOD_WIN;
        hide_key = ReadIniString("HideHotKey", "Key", NULL, ini_path.c_str());
        if (hide_key == KEY_INVALID || hide_key == NULL_CHAR || hide_modifiers == NO_MOD) {
            hide_modifiers = MOD_CONTROL | MOD_SHIFT;
            std::cerr << "Invalid hide key or no modifiers specified, using default: Ctrl+Shift+C" << std::endl;
        }

        if (ReadIniInt("ToggleHotKey", "Alt", NO_MOD, ini_path.c_str()) == TRUE)
            toggle_modifiers |= MOD_ALT;
        if (ReadIniInt("ToggleHotKey", "Ctrl", NO_MOD, ini_path.c_str()) == TRUE)
            toggle_modifiers |= MOD_CONTROL;
        if (ReadIniInt("ToggleHotKey", "Shift", NO_MOD, ini_path.c_str()) == TRUE)
            toggle_modifiers |= MOD_SHIFT;
        if (ReadIniInt("ToggleHotKey", "WinKey", NO_MOD, ini_path.c_str()) == TRUE)
            toggle_modifiers |= MOD_WIN;
        toggle_key = ReadIniString("ToggleHotKey", "Key", NULL, ini_path.c_str());
        if (toggle_key == KEY_INVALID || toggle_key == NULL_CHAR || toggle_modifiers == NO_MOD) {
            toggle_modifiers = MOD_CONTROL | MOD_SHIFT;
            std::cerr << "Invalid toggle key or no modifiers specified, using default: Ctrl+Shift+V" << std::endl;
        }

        if (ReadIniInt("ForgetHotKey", "Alt", NO_MOD, ini_path.c_str()) == TRUE)
            forget_modifiers |= MOD_ALT;
        if (ReadIniInt("ForgetHotKey", "Ctrl", NO_MOD, ini_path.c_str()) == TRUE)
            forget_modifiers |= MOD_CONTROL;
        if (ReadIniInt("ForgetHotKey", "Shift", NO_MOD, ini_path.c_str()) == TRUE)
            forget_modifiers |= MOD_SHIFT;
        if (ReadIniInt("ForgetHotKey", "WinKey", NO_MOD, ini_path.c_str()) == TRUE)
            forget_modifiers |= MOD_WIN;
        forget_key = ReadIniString("ForgetHotKey", "Key", NULL, ini_path.c_str());
        if (forget_key == KEY_INVALID || forget_key == NULL_CHAR || forget_modifiers == NO_MOD) {
            forget_modifiers = MOD_CONTROL | MOD_SHIFT;
            std::cerr << "Invalid forget key or no modifiers specified, using default: Ctrl+Shift+X" << std::endl;
        }
    }
    else {
        std::cout << "Could not find configuration file winhidecfg.ini, using default settings. Hide window: Ctrl+Shift+C, Toggle window: Ctrl+Shift+V, Forget window: Ctrl+Shift+X" << std::endl;
        toggle_modifiers = MOD_CONTROL | MOD_SHIFT;
        hide_modifiers = MOD_CONTROL | MOD_SHIFT;
        forget_modifiers = MOD_CONTROL | MOD_SHIFT;
    }

    if (RegisterHotKey(NULL, HOTKEY_HIDE, hide_modifiers | MOD_NOREPEAT, hide_key) &&
        RegisterHotKey(NULL, HOTKEY_TOGGLE, toggle_modifiers | MOD_NOREPEAT, toggle_key) &&
        RegisterHotKey(NULL, HOTKEY_FORGET, forget_modifiers | MOD_NOREPEAT, forget_key)) {
        std::cout << "Hotkeys registered successfully" << std::endl;
    }
    else {
        std::cerr << "Failed to register hotkeys, exiting" << std::endl;
        exit(EXIT_FAILURE);
    }

    if (start_hidden) {
        HWND console = GetConsoleWindow();
        WindowState(console, SW_HIDE, "Window hidden");
        windows.push_front(console);
    }

    while (GetMessage(&msg, NULL, FALSE, FALSE) != FALSE) {
        if (msg.message == WM_HOTKEY) {
            if (msg.wParam == HOTKEY_HIDE) {
                HWND handle = GetForegroundWindow();
                char classname[BUFSIZ];
                GetClassNameA(handle, classname, BUFSIZ);
                if ((!windows.empty() && windows.front() == handle) || // prevent hiding the same window
                    find(exclusions.begin(), exclusions.end(), classname) != exclusions.end()) // prevent hiding OS elements
                    continue;
                WindowState(handle, SW_HIDE, "Window hidden");
                windows.push_back(handle);
                std::cout << "Remembered window: " << handle << std::endl;
            }
            else if (msg.wParam == HOTKEY_TOGGLE) {
                are_windows_hidden = !are_windows_hidden;
                for (HWND window : windows) {
                    WindowState(window, are_windows_hidden ? SW_HIDE : SW_SHOW, are_windows_hidden ? "Window hidden" : "Window shown");
                }
            }
            else if (msg.wParam == HOTKEY_FORGET && !windows.empty()) {
                HWND forgotten = windows.back();
                windows.pop_back();
                std::cout << "Forgotten window: " << forgotten << std::endl;
                if (!windows.empty()) {
                    std::cout << "Last remembered window: " << windows.back() << std::endl;
                }
            }
        }
    }

    return EXIT_SUCCESS;
}

