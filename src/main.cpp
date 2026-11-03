#include "AppWindow.h"

#include <windows.h>
#include <objbase.h>
#include <shellapi.h>
#include <shobjidl.h>

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, PWSTR, int showCommand)
{
    // The embedded manifest opts into PerMonitorV2. Calling this as well is
    // harmless and keeps the app sharp when launched by unusual hosts.
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
    SetCurrentProcessExplicitAppUserModelID(L"Local.WorkEncourager");

    // Closing the window parks the app in the tray, so a second launch should
    // wake the existing instance instead of creating another hidden timer.
    HANDLE instanceMutex = CreateMutexW(nullptr, FALSE, L"Local\\WorkEncourager.SingleInstance");
    if (instanceMutex != nullptr && GetLastError() == ERROR_ALREADY_EXISTS)
    {
        HWND existingWindow = nullptr;
        for (int attempt = 0; attempt < 40 && existingWindow == nullptr; ++attempt)
        {
            existingWindow = FindWindowW(AppWindow::kWindowClassName, nullptr);
            if (existingWindow == nullptr)
            {
                Sleep(50);
            }
        }

        if (existingWindow != nullptr)
        {
            DWORD existingProcessId = 0;
            GetWindowThreadProcessId(existingWindow, &existingProcessId);
            AllowSetForegroundWindow(existingProcessId);
            PostMessageW(existingWindow, AppWindow::kShowWindowMessage, 0, 0);
        }

        CloseHandle(instanceMutex);
        return 0;
    }

    const HRESULT comResult = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);

    AppWindow appWindow;
    if (!appWindow.Create(instance, showCommand))
    {
        if (SUCCEEDED(comResult))
        {
            CoUninitialize();
        }
        if (instanceMutex != nullptr)
        {
            CloseHandle(instanceMutex);
        }
        return 1;
    }

    const int result = appWindow.RunMessageLoop();
    if (SUCCEEDED(comResult))
    {
        CoUninitialize();
    }
    if (instanceMutex != nullptr)
    {
        CloseHandle(instanceMutex);
    }
    return result;
}

