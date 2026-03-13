#include <windows.h>
#include <string>
#include <atomic>
#include <thread>
#include <chrono>

static std::atomic<bool> g_clicking(false);
static std::atomic<int> g_intervalMs(100);
static std::atomic<int> g_toggleVk('Q');

HWND hStatus = nullptr;

void updateStatus() {
    if (!hStatus) return;
    std::wstring msg = L"Status: ";
    msg += g_clicking ? L"RUNNING" : L"STOPPED";
    SetWindowTextW(hStatus, msg.c_str());
}

void clickLoop() {
    bool lastKeyState = false;
    while (true) {
        int vk = g_toggleVk.load();
        SHORT keyState = GetAsyncKeyState(vk);
        bool pressed = (keyState & 0x8000) != 0;
        if (pressed && !lastKeyState) {
            g_clicking = !g_clicking.load();
            updateStatus();
        }
        lastKeyState = pressed;

        if (g_clicking) {
            INPUT input[2] = {};
            input[0].type = INPUT_MOUSE;
            input[0].mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
            input[1].type = INPUT_MOUSE;
            input[1].mi.dwFlags = MOUSEEVENTF_LEFTUP;
            SendInput(2, input, sizeof(INPUT));
            int interval = g_intervalMs.load();
            if (interval < 1) interval = 1;
            std::this_thread::sleep_for(std::chrono::milliseconds(interval));
        } else {
            std::this_thread::sleep_for(std::chrono::milliseconds(30));
        }
    }
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    static HWND hCpsEdit, hKeyEdit;

    switch (msg) {
    case WM_CREATE: {
        CreateWindowW(L"static", L"Clicks per second:", WS_VISIBLE | WS_CHILD, 20, 20, 130, 20, hwnd, nullptr, nullptr, nullptr);
        hCpsEdit = CreateWindowW(L"edit", L"10", WS_VISIBLE | WS_CHILD | WS_BORDER | ES_NUMBER, 160, 18, 70, 24, hwnd, nullptr, nullptr, nullptr);

        CreateWindowW(L"static", L"Start/Stop key:", WS_VISIBLE | WS_CHILD, 20, 60, 130, 20, hwnd, nullptr, nullptr, nullptr);
        hKeyEdit = CreateWindowW(L"edit", L"Q", WS_VISIBLE | WS_CHILD | WS_BORDER | ES_UPPERCASE | ES_AUTOHSCROLL, 160, 58, 70, 24, hwnd, nullptr, nullptr, nullptr);

        CreateWindowW(L"button", L"Apply", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON, 20, 100, 70, 30, hwnd, (HMENU)1, nullptr, nullptr);

        hStatus = CreateWindowW(L"static", L"Status: STOPPED", WS_VISIBLE | WS_CHILD, 20, 150, 220, 20, hwnd, nullptr, nullptr, nullptr);
        updateStatus();
        break;
    }

    case WM_COMMAND:
        if (LOWORD(wParam) == 1) {
            wchar_t buf[64] = {};
            GetWindowTextW(hCpsEdit, buf, _countof(buf));
            int cps = _wtoi(buf);
            if (cps <= 0) cps = 10;
            if (cps > 1000) cps = 1000;
            int interval = 1000 / cps;
            g_intervalMs = interval;

            GetWindowTextW(hKeyEdit, buf, _countof(buf));
            if (buf[0] == L'\0') {
                g_toggleVk = 'Q';
                SetWindowTextW(hKeyEdit, L"Q");
            } else {
                wchar_t c = buf[0];
                if (c >= L'a' && c <= L'z') c -= 32;
                g_toggleVk = (int)c;
            }

            // Minimal GUI: no popup after applying settings.
        }
        break;

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

int APIENTRY wWinMain(HINSTANCE hInstance, HINSTANCE, LPWSTR, int nCmdShow) {
    WNDCLASSW wc = {};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = L"AutoClickerClass";
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);

    RegisterClassW(&wc);
    HWND hwnd = CreateWindowExW(0, wc.lpszClassName, L"The Toviah Clicker", WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        CW_USEDEFAULT, CW_USEDEFAULT, 260, 230, nullptr, nullptr, hInstance, nullptr);

    if (!hwnd) return 0;

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    std::thread(clickLoop).detach();

    MSG msg;
    while (GetMessageW(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    return (int)msg.wParam;
}

// MinGW with -mwindows expects WinMain entrypoint. Forward to wWinMain.
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    return wWinMain(hInstance, hPrevInstance, GetCommandLineW(), nCmdShow);
}
