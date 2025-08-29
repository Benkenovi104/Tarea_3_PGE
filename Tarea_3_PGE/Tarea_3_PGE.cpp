#include "ui.h"

int APIENTRY wWinMain(HINSTANCE hInst, HINSTANCE, LPWSTR, int nCmd) {
#if DPI_AWARE
    // DPI awareness por proceso (Windows 10+). Ignora errores en sistemas viejos.
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
#endif

    RegisterChichiloWindow(hInst);

    HWND hWnd = CreateWindowExW(
        WS_EX_OVERLAPPEDWINDOW,
        kAppClass,
        L"Cantina Chichilo (Win32)",
        WS_OVERLAPPEDWINDOW | WS_VISIBLE,
        CW_USEDEFAULT, CW_USEDEFAULT, 1100, 720,
        nullptr, nullptr, hInst, nullptr
    );

    if (!hWnd) return 0;

    ShowWindow(hWnd, nCmd);
    UpdateWindow(hWnd);

    MSG msg{};
    while (GetMessageW(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
    return (int)msg.wParam;
}
