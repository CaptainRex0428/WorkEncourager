#include "AppWindow.h"
#include "Resource.h"

#include <dwmapi.h>
#include <windowsx.h>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <cwchar>
#include <iterator>
#include <string>
#include <vector>

using Microsoft::WRL::ComPtr;

namespace
{
    constexpr wchar_t kWindowTitle[] = L"上班鼓励器";
    constexpr wchar_t kTrayHoverClassName[] = L"WorkEncourager.TrayHover";
    constexpr wchar_t kBodyFont[] = L"Microsoft YaHei UI";
    constexpr wchar_t kNumberFont[] = L"Bahnschrift";

    constexpr uint32_t kInk = 0x17191D;
    constexpr uint32_t kInkSoft = 0x44474E;
    constexpr uint32_t kMuted = 0x85888E;
    constexpr uint32_t kFaint = 0xA4A6A1;
    constexpr uint32_t kBorder = 0xE8E4DB;
    constexpr uint32_t kCardBorder = 0xECE7DE;
    constexpr uint32_t kSurface = 0xFFFFFF;
    constexpr uint32_t kBlue = 0x1D4ED8;
    constexpr uint32_t kBlueSoft = 0x3E6DF3;
    constexpr uint32_t kGreen = 0x0F7B55;
    constexpr uint32_t kAmber = 0xB66A16;
    constexpr uint32_t kCoral = 0xC9663B;
    constexpr uint32_t kCream = 0xF8F6F0;

    constexpr float kContentLeft = 32.0f;
    constexpr float kContentRight = 528.0f;
    constexpr float kContentWidth = kContentRight - kContentLeft;

    constexpr int kStartField = 0;
    constexpr int kEndField = 1;
    constexpr int kSalaryField = 2;

    constexpr UINT kTrayCallbackMessage = WM_APP + 11;
    constexpr UINT kTrayIconId = 1;
    constexpr UINT kTrayMenuOpen = 40001;
    constexpr UINT kTrayMenuTogglePause = 40002;
    constexpr UINT kTrayMenuExit = 40003;

    D2D1_RECT_F MakeRect(float left, float top, float right, float bottom)
    {
        return D2D1::RectF(left, top, right, bottom);
    }

    D2D1_RECT_F ExpandRect(const D2D1_RECT_F& rect, float dx, float dy)
    {
        return MakeRect(rect.left + dx, rect.top + dy, rect.right - dx, rect.bottom - dy);
    }

    D2D1_POINT_2F CenterOf(const D2D1_RECT_F& rect)
    {
        return D2D1::Point2F((rect.left + rect.right) * 0.5f, (rect.top + rect.bottom) * 0.5f);
    }

    bool Contains(const D2D1_RECT_F& rect, float x, float y)
    {
        return x >= rect.left && x <= rect.right && y >= rect.top && y <= rect.bottom;
    }

    float Clamp01(float value)
    {
        return std::clamp(value, 0.0f, 1.0f);
    }

    uint32_t MixColor(uint32_t from, uint32_t to, float amount)
    {
        const float t = Clamp01(amount);
        const uint32_t r = static_cast<uint32_t>(((from >> 16) & 0xFF) * (1.0f - t) + ((to >> 16) & 0xFF) * t);
        const uint32_t g = static_cast<uint32_t>(((from >> 8) & 0xFF) * (1.0f - t) + ((to >> 8) & 0xFF) * t);
        const uint32_t b = static_cast<uint32_t>((from & 0xFF) * (1.0f - t) + (to & 0xFF) * t);
        return (r << 16) | (g << 8) | b;
    }

    void Approach(float& value, bool active)
    {
        const float target = active ? 1.0f : 0.0f;
        value += (target - value) * 0.32f;
        if (std::fabs(target - value) < 0.006f)
        {
            value = target;
        }
    }

    D2D1_RECT_F LogoRect()
    {
        return MakeRect(20.0f, 12.0f, 48.0f, 40.0f);
    }

    D2D1_RECT_F GearHitRect()
    {
        return MakeRect(446.0f, 11.0f, 474.0f, 39.0f);
    }

    D2D1_RECT_F MinimizeHitRect()
    {
        return MakeRect(480.0f, 11.0f, 508.0f, 39.0f);
    }

    D2D1_RECT_F CloseHitRect()
    {
        return MakeRect(514.0f, 11.0f, 542.0f, 39.0f);
    }

    D2D1_RECT_F PrimaryButtonRect()
    {
        return MakeRect(32.0f, 366.0f, 528.0f, 414.0f);
    }

    D2D1_RECT_F GoalHeaderRect()
    {
        return MakeRect(32.0f, 297.0f, 528.0f, 316.0f);
    }

    D2D1_RECT_F ProgressCardRect()
    {
        return MakeRect(32.0f, 430.0f, 528.0f, 590.0f);
    }

    D2D1_RECT_F SettingsPanelRect()
    {
        return MakeRect(56.0f, 80.0f, 504.0f, 616.0f);
    }

    D2D1_RECT_F SettingsCloseRect()
    {
        return MakeRect(462.0f, 96.0f, 494.0f, 128.0f);
    }

    D2D1_RECT_F FieldBoxRect(int index)
    {
        switch (index)
        {
        case kStartField:
            return MakeRect(88.0f, 198.0f, 272.0f, 252.0f);
        case kEndField:
            return MakeRect(288.0f, 198.0f, 472.0f, 252.0f);
        case kSalaryField:
            return MakeRect(88.0f, 288.0f, 472.0f, 342.0f);
        default:
            return MakeRect(0, 0, 0, 0);
        }
    }

    D2D1_RECT_F FieldLabelRect(int index)
    {
        const D2D1_RECT_F box = FieldBoxRect(index);
        return MakeRect(box.left, box.top - 24.0f, box.right, box.top - 4.0f);
    }

    D2D1_RECT_F PreviewRect()
    {
        return MakeRect(88.0f, 360.0f, 472.0f, 450.0f);
    }

    D2D1_RECT_F DialogSaveRect()
    {
        return MakeRect(288.0f, 500.0f, 472.0f, 548.0f);
    }

    D2D1_RECT_F DialogCancelRect()
    {
        return MakeRect(88.0f, 500.0f, 272.0f, 548.0f);
    }

    uint32_t StatusColor(WorkPhase phase)
    {
        switch (phase)
        {
        case WorkPhase::Weekend:
            return 0x2C6954;
        case WorkPhase::BeforeWork:
            return 0x6D6F75;
        case WorkPhase::Working:
            return kBlue;
        case WorkPhase::Paused:
            return kAmber;
        case WorkPhase::Finished:
            return 0x2C6954;
        }
        return kInk;
    }

    HRESULT CreatePolygonGeometry(ID2D1Factory* factory, const D2D1_POINT_2F* points, size_t count,
                                  ID2D1PathGeometry** geometry)
    {
        if (factory == nullptr || points == nullptr || count < 3 || geometry == nullptr)
        {
            return E_INVALIDARG;
        }

        ComPtr<ID2D1PathGeometry> result;
        HRESULT hr = factory->CreatePathGeometry(result.GetAddressOf());
        if (FAILED(hr))
        {
            return hr;
        }

        ComPtr<ID2D1GeometrySink> sink;
        hr = result->Open(sink.GetAddressOf());
        if (FAILED(hr))
        {
            return hr;
        }

        sink->BeginFigure(points[0], D2D1_FIGURE_BEGIN_FILLED);
        for (size_t index = 1; index < count; ++index)
        {
            sink->AddLine(points[index]);
        }
        sink->EndFigure(D2D1_FIGURE_END_CLOSED);

        hr = sink->Close();
        if (FAILED(hr))
        {
            return hr;
        }

        *geometry = result.Detach();
        return S_OK;
    }

    std::wstring NormalizeTypedCharacter(wchar_t character)
    {
        if (character >= 0xFF10 && character <= 0xFF19)
        {
            return std::wstring(1, static_cast<wchar_t>(L'0' + (character - 0xFF10)));
        }

        switch (character)
        {
        case 0xFF1A:
            return L":";
        case 0xFF0E:
        case 0x3002:
            return L".";
        default:
            return std::wstring(1, character);
        }
    }

    bool IsDigit(wchar_t character)
    {
        return character >= L'0' && character <= L'9';
    }

    LRESULT CALLBACK TrayHoverWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
    {
        switch (message)
        {
        case WM_NCHITTEST:
            return HTTRANSPARENT;

        case WM_MOUSEACTIVATE:
            return MA_NOACTIVATE;

        case WM_ERASEBKGND:
            return 1;

        case WM_PAINT:
        {
            PAINTSTRUCT paint{};
            HDC deviceContext = BeginPaint(hwnd, &paint);
            RECT client{};
            GetClientRect(hwnd, &client);

            HBRUSH background = CreateSolidBrush(RGB(29, 31, 36));
            FillRect(deviceContext, &client, background);
            DeleteObject(background);

            UINT dpi = GetDpiForWindow(hwnd);
            if (dpi == 0)
            {
                dpi = 96;
            }
            const int accentWidth = MulDiv(3, static_cast<int>(dpi), 96);
            const int padding = MulDiv(16, static_cast<int>(dpi), 96);
            RECT accent{ 0, 0, accentWidth, client.bottom };
            HBRUSH accentBrush = CreateSolidBrush(RGB(47, 100, 255));
            FillRect(deviceContext, &accent, accentBrush);
            DeleteObject(accentBrush);

            HPEN borderPen = CreatePen(PS_SOLID, 1, RGB(62, 65, 74));
            HGDIOBJ oldPen = SelectObject(deviceContext, borderPen);
            HGDIOBJ oldBrush = SelectObject(deviceContext, GetStockObject(NULL_BRUSH));
            Rectangle(deviceContext, 0, 0, client.right, client.bottom);
            SelectObject(deviceContext, oldBrush);
            SelectObject(deviceContext, oldPen);
            DeleteObject(borderPen);

            wchar_t text[512]{};
            GetWindowTextW(hwnd, text, static_cast<int>(std::size(text)));
            HFONT font = reinterpret_cast<HFONT>(SendMessageW(hwnd, WM_GETFONT, 0, 0));
            HGDIOBJ oldFont = font != nullptr ? SelectObject(deviceContext, font) : nullptr;
            SetBkMode(deviceContext, TRANSPARENT);
            SetTextColor(deviceContext, RGB(246, 244, 237));
            RECT textRect{
                padding + accentWidth,
                padding - MulDiv(3, static_cast<int>(dpi), 96),
                client.right - padding,
                client.bottom - padding
            };
            DrawTextW(deviceContext, text, -1, &textRect,
                      DT_LEFT | DT_TOP | DT_NOPREFIX | DT_WORDBREAK);
            if (oldFont != nullptr)
            {
                SelectObject(deviceContext, oldFont);
            }

            EndPaint(hwnd, &paint);
            return 0;
        }

        default:
            break;
        }

        return DefWindowProcW(hwnd, message, wParam, lParam);
    }

    ATOM RegisterTrayHoverClass(HINSTANCE instance)
    {
        WNDCLASSEXW windowClass{};
        windowClass.cbSize = sizeof(windowClass);
        windowClass.style = CS_HREDRAW | CS_VREDRAW | CS_DROPSHADOW;
        windowClass.lpfnWndProc = &TrayHoverWndProc;
        windowClass.hInstance = instance;
        windowClass.hCursor = LoadCursorW(nullptr, IDC_ARROW);
        windowClass.lpszClassName = kTrayHoverClassName;

        const ATOM atom = RegisterClassExW(&windowClass);
        if (atom == 0 && GetLastError() != ERROR_CLASS_ALREADY_EXISTS)
        {
            return 0;
        }
        return atom;
    }
}

AppWindow::AppWindow()
    : settings_(SettingsStore::Load()),
      clock_(settings_)
{
}

AppWindow::~AppWindow()
{
    RemoveTrayIcon();
    DestroyTrayHoverWindow();
    if (hwnd_ != nullptr)
    {
        KillTimer(hwnd_, kAnimationTimer);
    }
    DiscardDeviceResources();
}

ATOM AppWindow::RegisterWindowClass(HINSTANCE instance)
{
    WNDCLASSEXW windowClass{};
    windowClass.cbSize = sizeof(windowClass);
    windowClass.style = CS_HREDRAW | CS_VREDRAW;
    windowClass.lpfnWndProc = &AppWindow::WndProc;
    windowClass.hInstance = instance;
    windowClass.hIcon = static_cast<HICON>(LoadImageW(instance, MAKEINTRESOURCEW(IDI_APP_ICON), IMAGE_ICON,
                                                     0, 0, LR_DEFAULTSIZE | LR_SHARED));
    windowClass.hIconSm = static_cast<HICON>(LoadImageW(instance, MAKEINTRESOURCEW(IDI_APP_ICON), IMAGE_ICON,
                                                        16, 16, LR_SHARED));
    windowClass.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    windowClass.lpszClassName = kWindowClassName;

    const ATOM atom = RegisterClassExW(&windowClass);
    if (atom == 0 && GetLastError() != ERROR_CLASS_ALREADY_EXISTS)
    {
        return 0;
    }
    return atom;
}

bool AppWindow::Create(HINSTANCE instance, int showCommand)
{
    instance_ = instance;
    taskbarCreatedMessage_ = RegisterWindowMessageW(L"TaskbarCreated");

    if (!InitializeGraphics())
    {
        MessageBoxW(nullptr, L"无法初始化 Direct2D / DirectWrite 图形环境。", kWindowTitle, MB_ICONERROR | MB_OK);
        return false;
    }

    if (RegisterWindowClass(instance) == 0)
    {
        MessageBoxW(nullptr, L"无法注册主窗口类。", kWindowTitle, MB_ICONERROR | MB_OK);
        return false;
    }
    RegisterTrayHoverClass(instance);

    dpi_ = GetDpiForSystem();
    if (dpi_ == 0)
    {
        dpi_ = 96;
    }

    HMONITOR monitor = MonitorFromPoint(POINT{ 0, 0 }, MONITOR_DEFAULTTOPRIMARY);
    MONITORINFO monitorInfo{};
    monitorInfo.cbSize = sizeof(monitorInfo);
    GetMonitorInfoW(monitor, &monitorInfo);

    const float workWidth = static_cast<float>(monitorInfo.rcWork.right - monitorInfo.rcWork.left);
    const float workHeight = static_cast<float>(monitorInfo.rcWork.bottom - monitorInfo.rcWork.top);
    const float desiredScale = static_cast<float>(dpi_) / 96.0f;
    float scale = std::min(desiredScale, std::min((workWidth - 64.0f) / kWidth, (workHeight - 64.0f) / kHeight));
    scale = std::max(scale, 0.45f);
    uiScale_ = scale;

    const int width = std::max(1, static_cast<int>(std::lround(kWidth * scale)));
    const int height = std::max(1, static_cast<int>(std::lround(kHeight * scale)));
    const int x = monitorInfo.rcWork.left + (static_cast<int>(workWidth) - width) / 2;
    const int y = monitorInfo.rcWork.top + (static_cast<int>(workHeight) - height) / 2;

    const DWORD style = WS_POPUP | WS_THICKFRAME | WS_MINIMIZEBOX | WS_SYSMENU;
    hwnd_ = CreateWindowExW(WS_EX_APPWINDOW, kWindowClassName, kWindowTitle, style, x, y, width, height,
                            nullptr, nullptr, instance, this);
    if (hwnd_ == nullptr)
    {
        MessageBoxW(nullptr, L"无法创建应用窗口。", kWindowTitle, MB_ICONERROR | MB_OK);
        return false;
    }

    // Windows 11: rounded corners and no bright DWM border. Older Windows
    // versions simply ignore these attributes.
    const int cornerPreference = 2; // DWMWCP_ROUND
    DwmSetWindowAttribute(hwnd_, 33 /* DWMWA_WINDOW_CORNER_PREFERENCE */, &cornerPreference,
                          sizeof(cornerPreference));
    const COLORREF borderColor = 0xFFFFFFFE; // DWMWA_COLOR_NONE
    DwmSetWindowAttribute(hwnd_, 34 /* DWMWA_BORDER_COLOR */, &borderColor, sizeof(borderColor));

    const UINT actualDpi = GetDpiForWindow(hwnd_);
    if (actualDpi != 0 && actualDpi != dpi_)
    {
        UpdateWindowScale(actualDpi, true);
    }

    SetTimer(hwnd_, kAnimationTimer, 50, nullptr);
    AddTrayIcon();

    ShowWindow(hwnd_, showCommand == SW_SHOWDEFAULT ? SW_SHOWNORMAL : showCommand);
    UpdateWindow(hwnd_);
    SetForegroundWindow(hwnd_);
    return true;
}

int AppWindow::RunMessageLoop()
{
    MSG message{};
    while (GetMessageW(&message, nullptr, 0, 0) > 0)
    {
        TranslateMessage(&message);
        DispatchMessageW(&message);
    }
    return static_cast<int>(message.wParam);
}

std::wstring AppWindow::BuildTrayHoverText(const WorkSnapshot& snapshot) const
{
    std::wstring status;
    std::wstring detail;

    switch (snapshot.phase)
    {
    case WorkPhase::Working:
        status = L"上班中 · 今日已赚 " + FormatMoney(snapshot.earnedToday, 2);
        detail = L"距离下班 " + FormatDurationHm(snapshot.remainingSeconds);
        break;
    case WorkPhase::Paused:
        status = L"休息中 · 今日已赚 " + FormatMoney(snapshot.earnedToday, 2);
        detail = L"距离下班 " + FormatDurationHm(snapshot.remainingSeconds);
        break;
    case WorkPhase::BeforeWork:
        status = L"还未上班";
        detail = L"距离上班 " + FormatDurationHm(snapshot.untilStartSeconds);
        break;
    case WorkPhase::Finished:
        status = L"下班啦";
        detail = L"今日已赚 " + FormatMoney(snapshot.earnedToday, 2);
        break;
    case WorkPhase::Weekend:
        status = L"周末愉快";
        detail = L"今天不上班，好好休息";
        break;
    }

    return L"上班鼓励器\n" + status + L"\n" + detail;
}

bool AppWindow::AddTrayIcon()
{
    if (trayIconAdded_ || hwnd_ == nullptr)
    {
        return trayIconAdded_;
    }

    NOTIFYICONDATAW data{};
    data.cbSize = sizeof(data);
    data.hWnd = hwnd_;
    data.uID = kTrayIconId;
    data.uFlags = NIF_MESSAGE | NIF_ICON;
    data.uCallbackMessage = kTrayCallbackMessage;

    int iconSize = 16;
    if (dpi_ != 0)
    {
        const int dpiIconSize = GetSystemMetricsForDpi(SM_CXSMICON, dpi_);
        if (dpiIconSize > 0)
        {
            iconSize = dpiIconSize;
        }
    }

    HICON icon = static_cast<HICON>(LoadImageW(instance_, MAKEINTRESOURCEW(IDI_APP_ICON), IMAGE_ICON,
                                               iconSize, iconSize, LR_SHARED));
    if (icon == nullptr)
    {
        icon = static_cast<HICON>(LoadImageW(instance_, MAKEINTRESOURCEW(IDI_APP_ICON), IMAGE_ICON,
                                             0, 0, LR_DEFAULTSIZE | LR_SHARED));
    }
    data.hIcon = icon;

    if (!Shell_NotifyIconW(NIM_ADD, &data))
    {
        return false;
    }

    data.uVersion = NOTIFYICON_VERSION_4;
    Shell_NotifyIconW(NIM_SETVERSION, &data);

    trayIconData_ = data;
    trayIconAdded_ = true;
    trayHoverTracking_ = false;
    trayHoverStarted_ = 0;
    lastTrayHoverPoll_ = GetTickCount64();
    return true;
}

void AppWindow::RemoveTrayIcon()
{
    if (!trayIconAdded_)
    {
        return;
    }

    Shell_NotifyIconW(NIM_DELETE, &trayIconData_);
    trayIconAdded_ = false;
    trayHoverTracking_ = false;
    HideTrayHoverTip();
}

bool AppWindow::EnsureTrayHoverWindow()
{
    if (trayHoverWindow_ != nullptr)
    {
        return true;
    }

    trayHoverWindow_ = CreateWindowExW(
        WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE | WS_EX_TRANSPARENT,
        kTrayHoverClassName, L"", WS_POPUP, 0, 0, 10, 10, nullptr, nullptr, instance_, nullptr);
    if (trayHoverWindow_ == nullptr)
    {
        return false;
    }

    const int cornerPreference = 2; // DWMWCP_ROUND
    DwmSetWindowAttribute(trayHoverWindow_, 33, &cornerPreference, sizeof(cornerPreference));
    const COLORREF borderColor = 0xFFFFFFFE; // DWMWA_COLOR_NONE
    DwmSetWindowAttribute(trayHoverWindow_, 34, &borderColor, sizeof(borderColor));
    return true;
}

void AppWindow::DestroyTrayHoverWindow()
{
    HideTrayHoverTip();
    if (trayHoverWindow_ != nullptr)
    {
        DestroyWindow(trayHoverWindow_);
        trayHoverWindow_ = nullptr;
    }
    if (trayHoverFont_ != nullptr)
    {
        DeleteObject(trayHoverFont_);
        trayHoverFont_ = nullptr;
    }
    trayHoverDpi_ = 0;
    trayHoverTracking_ = false;
}

void AppWindow::ShowTrayHoverTip()
{
    if (!trayIconAdded_ || !EnsureTrayHoverWindow())
    {
        return;
    }

    RECT iconRect{};
    if (!GetTrayIconRect(iconRect))
    {
        return;
    }

    UINT dpi = GetDpiForWindow(hwnd_);
    if (dpi == 0)
    {
        dpi = 96;
    }

    if (trayHoverFont_ == nullptr || trayHoverDpi_ != dpi)
    {
        if (trayHoverFont_ != nullptr)
        {
            DeleteObject(trayHoverFont_);
            trayHoverFont_ = nullptr;
        }

        trayHoverFont_ = CreateFontW(
            -MulDiv(11, static_cast<int>(dpi), 72), 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, ANTIALIASED_QUALITY,
            DEFAULT_PITCH | FF_DONTCARE, L"Microsoft YaHei UI");
        trayHoverDpi_ = dpi;
        SendMessageW(trayHoverWindow_, WM_SETFONT, reinterpret_cast<WPARAM>(trayHoverFont_), TRUE);
    }

    const std::wstring text = BuildTrayHoverText(clock_.Snapshot());
    SetWindowTextW(trayHoverWindow_, text.c_str());

    const int gap = MulDiv(8, static_cast<int>(dpi), 96);
    const int width = MulDiv(300, static_cast<int>(dpi), 96);
    const int height = MulDiv(82, static_cast<int>(dpi), 96);
    const int iconCenterX = (iconRect.left + iconRect.right) / 2;

    int x = iconCenterX - width / 2;
    int y = iconRect.top - height - gap;

    MONITORINFO monitorInfo{};
    monitorInfo.cbSize = sizeof(monitorInfo);
    const HMONITOR monitor = MonitorFromPoint(
        POINT{ iconCenterX, (iconRect.top + iconRect.bottom) / 2 }, MONITOR_DEFAULTTONEAREST);
    if (GetMonitorInfoW(monitor, &monitorInfo))
    {
        if (y < monitorInfo.rcWork.top)
        {
            y = iconRect.bottom + gap;
        }
        x = std::clamp<int>(x, static_cast<int>(monitorInfo.rcWork.left) + gap,
                            static_cast<int>(monitorInfo.rcWork.right) - width - gap);
        y = std::clamp<int>(y, static_cast<int>(monitorInfo.rcWork.top) + gap,
                            static_cast<int>(monitorInfo.rcWork.bottom) - height - gap);
    }

    SetWindowPos(trayHoverWindow_, HWND_TOPMOST, x, y, width, height,
                 SWP_NOACTIVATE | SWP_SHOWWINDOW);
    ShowWindow(trayHoverWindow_, SW_SHOWNOACTIVATE);
    InvalidateRect(trayHoverWindow_, nullptr, TRUE);
    UpdateWindow(trayHoverWindow_);
    trayHoverVisible_ = true;
}

void AppWindow::HideTrayHoverTip()
{
    if (trayHoverWindow_ != nullptr && trayHoverVisible_)
    {
        ShowWindow(trayHoverWindow_, SW_HIDE);
    }
    trayHoverVisible_ = false;
}

bool AppWindow::GetTrayIconRect(RECT& iconRect) const
{
    if (!trayIconAdded_)
    {
        return false;
    }

    NOTIFYICONIDENTIFIER identifier{};
    identifier.cbSize = sizeof(identifier);
    identifier.hWnd = hwnd_;
    identifier.uID = kTrayIconId;
    return SUCCEEDED(Shell_NotifyIconGetRect(&identifier, &iconRect));
}

void AppWindow::PollTrayHover(ULONGLONG now)
{
    if (!trayIconAdded_ || hwnd_ == nullptr)
    {
        trayHoverTracking_ = false;
        HideTrayHoverTip();
        return;
    }

    RECT iconRect{};
    if (!GetTrayIconRect(iconRect))
    {
        trayHoverTracking_ = false;
        HideTrayHoverTip();
        return;
    }

    POINT cursor{};
    GetCursorPos(&cursor);
    if (PtInRect(&iconRect, cursor))
    {
        if (!trayHoverTracking_)
        {
            trayHoverTracking_ = true;
            trayHoverStarted_ = now;
        }

        if (!trayHoverVisible_ && now - trayHoverStarted_ >= 450)
        {
            ShowTrayHoverTip();
        }
    }
    else
    {
        trayHoverTracking_ = false;
        HideTrayHoverTip();
    }
}

void AppWindow::ShowMainWindow()
{
    if (hwnd_ == nullptr)
    {
        return;
    }

    trayHoverTracking_ = false;
    HideTrayHoverTip();

    if (IsIconic(hwnd_))
    {
        ShowWindow(hwnd_, SW_RESTORE);
    }
    else
    {
        ShowWindow(hwnd_, SW_SHOW);
    }
    SetForegroundWindow(hwnd_);
}

void AppWindow::ShowTrayMenu()
{
    if (hwnd_ == nullptr)
    {
        return;
    }

    trayHoverTracking_ = false;
    HideTrayHoverTip();

    HMENU menu = CreatePopupMenu();
    if (menu == nullptr)
    {
        return;
    }

    const WorkSnapshot snapshot = clock_.Snapshot();
    const bool pauseAvailable = snapshot.phase == WorkPhase::Working || snapshot.phase == WorkPhase::Paused;
    const wchar_t* pauseLabel = snapshot.phase == WorkPhase::Paused ? L"继续计时" : L"暂停计时";

    AppendMenuW(menu, MF_STRING, kTrayMenuOpen, L"打开上班鼓励器");
    AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(menu, MF_STRING | (pauseAvailable ? MF_ENABLED : MF_GRAYED),
                kTrayMenuTogglePause, pauseLabel);
    AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(menu, MF_STRING, kTrayMenuExit, L"退出");
    SetMenuDefaultItem(menu, kTrayMenuOpen, FALSE);

    POINT cursor{};
    GetCursorPos(&cursor);
    SetForegroundWindow(hwnd_);
    const UINT command = static_cast<UINT>(
        TrackPopupMenuEx(menu, TPM_LEFTALIGN | TPM_RIGHTBUTTON | TPM_RETURNCMD | TPM_NONOTIFY,
                         cursor.x, cursor.y, hwnd_, nullptr));
    DestroyMenu(menu);
    PostMessageW(hwnd_, WM_NULL, 0, 0);

    if (command == kTrayMenuOpen)
    {
        ShowMainWindow();
    }
    else if (command == kTrayMenuTogglePause)
    {
        if (pauseAvailable && clock_.TogglePause())
        {
            InvalidateRect(hwnd_, nullptr, FALSE);
        }
    }
    else if (command == kTrayMenuExit)
    {
        PostMessageW(hwnd_, kExitMessage, 0, 0);
    }
}

void AppWindow::HandleTrayEvent(WPARAM wParam, LPARAM lParam)
{
    (void)wParam;
    const UINT notification = LOWORD(lParam);
    if (notification == WM_LBUTTONUP || notification == WM_LBUTTONDBLCLK ||
        notification == NIN_SELECT || notification == NIN_KEYSELECT)
    {
        ShowMainWindow();
    }
    else if (notification == WM_RBUTTONUP || notification == WM_CONTEXTMENU)
    {
        ShowTrayMenu();
    }
    else if (notification == WM_MOUSEMOVE)
    {
        if (!trayHoverTracking_)
        {
            trayHoverTracking_ = true;
            trayHoverStarted_ = GetTickCount64();
        }
    }
    else if (notification == NIN_POPUPOPEN)
    {
        trayHoverStarted_ = GetTickCount64() - 500;
        ShowTrayHoverTip();
    }
    else if (notification == NIN_POPUPCLOSE)
    {
        trayHoverTracking_ = false;
        HideTrayHoverTip();
    }
}

LRESULT CALLBACK AppWindow::WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    AppWindow* self = reinterpret_cast<AppWindow*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));

    if (message == WM_NCCREATE)
    {
        auto* createStruct = reinterpret_cast<CREATESTRUCTW*>(lParam);
        self = static_cast<AppWindow*>(createStruct->lpCreateParams);
        if (self != nullptr)
        {
            SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
            self->hwnd_ = hwnd;
        }
    }

    if (self != nullptr)
    {
        return self->HandleMessage(message, wParam, lParam);
    }
    return DefWindowProcW(hwnd, message, wParam, lParam);
}

LRESULT AppWindow::HandleMessage(UINT message, WPARAM wParam, LPARAM lParam)
{
    if (taskbarCreatedMessage_ != 0 && message == taskbarCreatedMessage_)
    {
        trayIconAdded_ = false;
        trayHoverTracking_ = false;
        HideTrayHoverTip();
        AddTrayIcon();
        return 0;
    }

    if (message == kTrayCallbackMessage)
    {
        HandleTrayEvent(wParam, lParam);
        return 0;
    }

    if (message == kShowWindowMessage)
    {
        ShowMainWindow();
        return 0;
    }

    if (message == kExitMessage)
    {
        exitRequested_ = true;
        PostMessageW(hwnd_, WM_CLOSE, 0, 0);
        return 0;
    }

    switch (message)
    {
    case WM_NCCALCSIZE:
        if (wParam == TRUE)
        {
            // Keep the native resize frame/shadow, but let the client area
            // cover the whole window so the UI can paint edge-to-edge.
            return 0;
        }
        break;

    case WM_NCHITTEST:
        return HTCLIENT;

    case WM_NCLBUTTONDBLCLK:
        return 0;

    case WM_ERASEBKGND:
        return 1;

    case WM_PAINT:
        Paint();
        return 0;

    case WM_TIMER:
        if (wParam == kAnimationTimer)
        {
            const ULONGLONG now = GetTickCount64();
            if (trayIconAdded_ && now - lastTrayHoverPoll_ >= 100)
            {
                lastTrayHoverPoll_ = now;
                PollTrayHover(now);
            }
            if (IsWindowVisible(hwnd_) && !IsIconic(hwnd_))
            {
                InvalidateRect(hwnd_, nullptr, FALSE);
            }
        }
        return 0;

    case WM_SIZE:
        if (renderTarget_ != nullptr && wParam != SIZE_MINIMIZED)
        {
            const UINT width = LOWORD(lParam);
            const UINT height = HIWORD(lParam);
            if (width > 0 && height > 0)
            {
                renderTarget_->Resize(D2D1::SizeU(width, height));
            }
        }
        InvalidateRect(hwnd_, nullptr, FALSE);
        return 0;

    case WM_DPICHANGED:
    {
        const UINT newDpi = HIWORD(wParam);
        if (newDpi != 0)
        {
            UpdateWindowScale(newDpi, true);
            InvalidateRect(hwnd_, nullptr, FALSE);
        }
        return 0;
    }

    case WM_SYSCOMMAND:
        if ((wParam & 0xFFF0) == SC_MAXIMIZE)
        {
            return 0;
        }
        break;

    case WM_MOUSEMOVE:
        OnMouseMove(static_cast<float>(GET_X_LPARAM(lParam)), static_cast<float>(GET_Y_LPARAM(lParam)));
        return 0;

    case WM_MOUSELEAVE:
        OnMouseLeave();
        return 0;

    case WM_LBUTTONDOWN:
        OnMouseDown(static_cast<float>(GET_X_LPARAM(lParam)), static_cast<float>(GET_Y_LPARAM(lParam)));
        return 0;

    case WM_LBUTTONUP:
        OnMouseUp(static_cast<float>(GET_X_LPARAM(lParam)), static_cast<float>(GET_Y_LPARAM(lParam)));
        return 0;

    case WM_CAPTURECHANGED:
        if (reinterpret_cast<HWND>(lParam) != hwnd_)
        {
            pressedZone_ = Zone::None;
            draggingWindow_ = false;
        }
        return 0;

    case WM_SETCURSOR:
        if (LOWORD(lParam) == HTCLIENT)
        {
            POINT point{};
            GetCursorPos(&point);
            ScreenToClient(hwnd_, &point);
            const D2D1_POINT_2F designPoint = ClientToDesign(point);
            const Zone zone = HitTest(designPoint.x, designPoint.y);

            if (IsFieldZone(zone))
            {
                SetCursor(LoadCursorW(nullptr, IDC_IBEAM));
            }
            else if (IsClickable(zone))
            {
                SetCursor(LoadCursorW(nullptr, IDC_HAND));
            }
            else
            {
                SetCursor(LoadCursorW(nullptr, IDC_ARROW));
            }
            return TRUE;
        }
        break;

    case WM_KEYDOWN:
    {
        const bool control = (GetKeyState(VK_CONTROL) & 0x8000) != 0;
        const bool shift = (GetKeyState(VK_SHIFT) & 0x8000) != 0;

        if (settingsOpen_)
        {
            switch (wParam)
            {
            case VK_ESCAPE:
                CloseSettings();
                return 0;
            case VK_RETURN:
                SaveSettings();
                return 0;
            case VK_TAB:
                CycleField(shift ? -1 : 1);
                return 0;
            case VK_UP:
                CycleField(-1);
                return 0;
            case VK_DOWN:
                CycleField(1);
                return 0;
            case VK_BACK:
                BackspaceField();
                return 0;
            case L'A':
                if (control)
                {
                    SelectFocusedFieldText();
                    return 0;
                }
                break;
            default:
                break;
            }
        }
        else if (control && wParam == VK_OEM_COMMA)
        {
            OpenSettings();
            return 0;
        }
        break;
    }

    case WM_CHAR:
        if (settingsOpen_ && focusedField_ != Field::None && wParam >= 32 && wParam != 127)
        {
            InsertCharacter(static_cast<wchar_t>(wParam));
            return 0;
        }
        break;

    case WM_CLOSE:
        // X / Alt+F4 normally parks the app in the notification area. The
        // explicit tray-menu Exit command sets exitRequested_ first, and a
        // missing tray icon falls back to a real exit so the process can
        // never become unreachable.
        if (exitRequested_ || !trayIconAdded_)
        {
            DestroyWindow(hwnd_);
        }
        else
        {
            ShowWindow(hwnd_, SW_HIDE);
        }
        return 0;

    case WM_QUERYENDSESSION:
        return TRUE;

    case WM_ENDSESSION:
        if (wParam)
        {
            exitRequested_ = true;
            DestroyWindow(hwnd_);
        }
        return 0;

    case WM_DESTROY:
        KillTimer(hwnd_, kAnimationTimer);
        RemoveTrayIcon();
        DestroyTrayHoverWindow();
        DiscardDeviceResources();
        hwnd_ = nullptr;
        PostQuitMessage(0);
        return 0;

    default:
        break;
    }

    return DefWindowProcW(hwnd_, message, wParam, lParam);
}

bool AppWindow::InitializeGraphics()
{
    D2D1_FACTORY_OPTIONS options{};
    HRESULT hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, __uuidof(ID2D1Factory), &options,
                                   reinterpret_cast<void**>(d2dFactory_.GetAddressOf()));
    if (FAILED(hr))
    {
        return false;
    }

    IDWriteFactory* writeFactory = nullptr;
    hr = DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory),
                             reinterpret_cast<IUnknown**>(&writeFactory));
    if (FAILED(hr) || writeFactory == nullptr)
    {
        return false;
    }
    dwriteFactory_.Attach(writeFactory);

    const D2D1_POINT_2F boltPoints[] = {
        D2D1::Point2F(0.58f, 0.08f),
        D2D1::Point2F(0.22f, 0.56f),
        D2D1::Point2F(0.46f, 0.56f),
        D2D1::Point2F(0.38f, 0.92f),
        D2D1::Point2F(0.78f, 0.42f),
        D2D1::Point2F(0.53f, 0.42f),
    };
    const D2D1_POINT_2F shirtPoints[] = {
        D2D1::Point2F(0.29f, 0.12f),
        D2D1::Point2F(0.39f, 0.22f),
        D2D1::Point2F(0.61f, 0.22f),
        D2D1::Point2F(0.71f, 0.12f),
        D2D1::Point2F(0.92f, 0.31f),
        D2D1::Point2F(0.74f, 0.49f),
        D2D1::Point2F(0.66f, 0.40f),
        D2D1::Point2F(0.66f, 0.90f),
        D2D1::Point2F(0.34f, 0.90f),
        D2D1::Point2F(0.34f, 0.40f),
        D2D1::Point2F(0.26f, 0.49f),
        D2D1::Point2F(0.08f, 0.31f),
    };
    const D2D1_POINT_2F playPoints[] = {
        D2D1::Point2F(0.10f, 0.04f),
        D2D1::Point2F(0.10f, 0.96f),
        D2D1::Point2F(0.92f, 0.50f),
    };

    if (FAILED(CreatePolygonGeometry(d2dFactory_.Get(), boltPoints, std::size(boltPoints),
                                     boltGeometry_.GetAddressOf())) ||
        FAILED(CreatePolygonGeometry(d2dFactory_.Get(), shirtPoints, std::size(shirtPoints),
                                     tshirtGeometry_.GetAddressOf())) ||
        FAILED(CreatePolygonGeometry(d2dFactory_.Get(), playPoints, std::size(playPoints),
                                     playGeometry_.GetAddressOf())))
    {
        return false;
    }

    return true;
}

bool AppWindow::EnsureDeviceResources()
{
    if (renderTarget_ != nullptr)
    {
        return true;
    }
    if (hwnd_ == nullptr || d2dFactory_ == nullptr)
    {
        return false;
    }

    RECT clientRect{};
    GetClientRect(hwnd_, &clientRect);
    const UINT width = static_cast<UINT>(std::max(1L, clientRect.right - clientRect.left));
    const UINT height = static_cast<UINT>(std::max(1L, clientRect.bottom - clientRect.top));

    D2D1_RENDER_TARGET_PROPERTIES renderTargetProperties = D2D1::RenderTargetProperties();
    renderTargetProperties.dpiX = 96.0f;
    renderTargetProperties.dpiY = 96.0f;

    const D2D1_HWND_RENDER_TARGET_PROPERTIES hwndProperties =
        D2D1::HwndRenderTargetProperties(hwnd_, D2D1::SizeU(width, height));
    HRESULT hr = d2dFactory_->CreateHwndRenderTarget(renderTargetProperties, hwndProperties,
                                                     renderTarget_.GetAddressOf());
    if (FAILED(hr))
    {
        return false;
    }

    renderTarget_->SetAntialiasMode(D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);
    renderTarget_->SetTextAntialiasMode(D2D1_TEXT_ANTIALIAS_MODE_CLEARTYPE);

    D2D1_GRADIENT_STOP backgroundStops[] = {
        { 0.0f, D2D1::ColorF(0xFCFBF7) },
        { 1.0f, D2D1::ColorF(0xF3F0E9) },
    };
    ComPtr<ID2D1GradientStopCollection> backgroundCollection;
    hr = renderTarget_->CreateGradientStopCollection(backgroundStops, static_cast<UINT32>(std::size(backgroundStops)),
                                                     D2D1_GAMMA_2_2, D2D1_EXTEND_MODE_CLAMP,
                                                     backgroundCollection.GetAddressOf());
    if (FAILED(hr))
    {
        DiscardDeviceResources();
        return false;
    }

    hr = renderTarget_->CreateLinearGradientBrush(
        D2D1::LinearGradientBrushProperties(D2D1::Point2F(0.0f, 0.0f), D2D1::Point2F(0.0f, kHeight)),
        backgroundCollection.Get(), backgroundBrush_.GetAddressOf());
    if (FAILED(hr))
    {
        DiscardDeviceResources();
        return false;
    }

    D2D1_GRADIENT_STOP blueStops[] = {
        { 0.0f, D2D1::ColorF(0xBFD2FF, 0.48f) },
        { 1.0f, D2D1::ColorF(0xBFD2FF, 0.0f) },
    };
    ComPtr<ID2D1GradientStopCollection> blueCollection;
    hr = renderTarget_->CreateGradientStopCollection(blueStops, static_cast<UINT32>(std::size(blueStops)),
                                                     D2D1_GAMMA_2_2, D2D1_EXTEND_MODE_CLAMP,
                                                     blueCollection.GetAddressOf());
    if (FAILED(hr))
    {
        DiscardDeviceResources();
        return false;
    }

    const D2D1_POINT_2F blueCenter = D2D1::Point2F(520.0f, 46.0f);
    hr = renderTarget_->CreateRadialGradientBrush(
        D2D1::RadialGradientBrushProperties(blueCenter, D2D1::Point2F(0.0f, 0.0f), 350.0f, 270.0f),
        blueCollection.Get(), blueGlowBrush_.GetAddressOf());
    if (FAILED(hr))
    {
        DiscardDeviceResources();
        return false;
    }

    D2D1_GRADIENT_STOP warmStops[] = {
        { 0.0f, D2D1::ColorF(0xFFDCC4, 0.32f) },
        { 1.0f, D2D1::ColorF(0xFFDCC4, 0.0f) },
    };
    ComPtr<ID2D1GradientStopCollection> warmCollection;
    hr = renderTarget_->CreateGradientStopCollection(warmStops, static_cast<UINT32>(std::size(warmStops)),
                                                     D2D1_GAMMA_2_2, D2D1_EXTEND_MODE_CLAMP,
                                                     warmCollection.GetAddressOf());
    if (FAILED(hr))
    {
        DiscardDeviceResources();
        return false;
    }

    const D2D1_POINT_2F warmCenter = D2D1::Point2F(-18.0f, 672.0f);
    hr = renderTarget_->CreateRadialGradientBrush(
        D2D1::RadialGradientBrushProperties(warmCenter, D2D1::Point2F(0.0f, 0.0f), 330.0f, 260.0f),
        warmCollection.Get(), warmGlowBrush_.GetAddressOf());
    if (FAILED(hr))
    {
        DiscardDeviceResources();
        return false;
    }

    D2D1_GRADIENT_STOP logoStops[] = {
        { 0.0f, D2D1::ColorF(0x3B78FF) },
        { 1.0f, D2D1::ColorF(0x133FC4) },
    };
    ComPtr<ID2D1GradientStopCollection> logoCollection;
    hr = renderTarget_->CreateGradientStopCollection(logoStops, static_cast<UINT32>(std::size(logoStops)),
                                                     D2D1_GAMMA_2_2, D2D1_EXTEND_MODE_CLAMP,
                                                     logoCollection.GetAddressOf());
    if (FAILED(hr))
    {
        DiscardDeviceResources();
        return false;
    }

    hr = renderTarget_->CreateLinearGradientBrush(
        D2D1::LinearGradientBrushProperties(D2D1::Point2F(20.0f, 12.0f), D2D1::Point2F(48.0f, 40.0f)),
        logoCollection.Get(), logoBrush_.GetAddressOf());
    if (FAILED(hr))
    {
        DiscardDeviceResources();
        return false;
    }

    D2D1_GRADIENT_STOP progressStops[] = {
        { 0.0f, D2D1::ColorF(0x2454E8) },
        { 1.0f, D2D1::ColorF(0x5A82FF) },
    };
    ComPtr<ID2D1GradientStopCollection> progressCollection;
    hr = renderTarget_->CreateGradientStopCollection(progressStops, static_cast<UINT32>(std::size(progressStops)),
                                                     D2D1_GAMMA_2_2, D2D1_EXTEND_MODE_CLAMP,
                                                     progressCollection.GetAddressOf());
    if (FAILED(hr))
    {
        DiscardDeviceResources();
        return false;
    }

    hr = renderTarget_->CreateLinearGradientBrush(
        D2D1::LinearGradientBrushProperties(D2D1::Point2F(52.0f, 0.0f), D2D1::Point2F(508.0f, 0.0f)),
        progressCollection.Get(), progressBrush_.GetAddressOf());
    if (FAILED(hr))
    {
        DiscardDeviceResources();
        return false;
    }

    return true;
}

void AppWindow::DiscardDeviceResources()
{
    brushCache_.clear();
    progressBrush_.Reset();
    logoBrush_.Reset();
    warmGlowBrush_.Reset();
    blueGlowBrush_.Reset();
    backgroundBrush_.Reset();
    renderTarget_.Reset();
}

void AppWindow::ResetDeviceBrushes()
{
    brushCache_.clear();
}

ID2D1SolidColorBrush* AppWindow::Brush(uint32_t rgb, float alpha)
{
    if (renderTarget_ == nullptr)
    {
        return nullptr;
    }

    const uint32_t alphaByte = static_cast<uint32_t>(std::lround(Clamp01(alpha) * 255.0f));
    const uint32_t key = (rgb << 8) | alphaByte;
    const auto found = brushCache_.find(key);
    if (found != brushCache_.end())
    {
        return found->second.Get();
    }

    ComPtr<ID2D1SolidColorBrush> brush;
    const HRESULT hr = renderTarget_->CreateSolidColorBrush(
        D2D1::ColorF(rgb, static_cast<float>(alphaByte) / 255.0f), brush.GetAddressOf());
    if (FAILED(hr))
    {
        return nullptr;
    }

    auto inserted = brushCache_.emplace(key, std::move(brush));
    return inserted.first->second.Get();
}

IDWriteTextFormat* AppWindow::GetFormat(const wchar_t* family, float size, DWRITE_FONT_WEIGHT weight,
                                        DWRITE_TEXT_ALIGNMENT alignment, DWRITE_PARAGRAPH_ALIGNMENT paragraph)
{
    if (dwriteFactory_ == nullptr)
    {
        return nullptr;
    }

    wchar_t keyBuffer[320]{};
    swprintf_s(keyBuffer, L"%s|%d|%d|%d|%d", family, static_cast<int>(std::lround(size * 10.0f)),
               static_cast<int>(weight), static_cast<int>(alignment), static_cast<int>(paragraph));
    const std::wstring key(keyBuffer);

    const auto found = formatCache_.find(key);
    if (found != formatCache_.end())
    {
        return found->second.Get();
    }

    ComPtr<IDWriteTextFormat> format;
    const HRESULT hr = dwriteFactory_->CreateTextFormat(family, nullptr, weight, DWRITE_FONT_STYLE_NORMAL,
                                                        DWRITE_FONT_STRETCH_NORMAL, size, L"zh-CN",
                                                        format.GetAddressOf());
    if (FAILED(hr))
    {
        return nullptr;
    }

    format->SetTextAlignment(alignment);
    format->SetParagraphAlignment(paragraph);
    format->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP);

    auto inserted = formatCache_.emplace(key, std::move(format));
    return inserted.first->second.Get();
}

IDWriteTextFormat* AppWindow::FmtC(float size, DWRITE_FONT_WEIGHT weight, DWRITE_TEXT_ALIGNMENT alignment,
                                   DWRITE_PARAGRAPH_ALIGNMENT paragraph)
{
    return GetFormat(kBodyFont, size, weight, alignment, paragraph);
}

IDWriteTextFormat* AppWindow::FmtN(float size, DWRITE_FONT_WEIGHT weight, DWRITE_TEXT_ALIGNMENT alignment,
                                   DWRITE_PARAGRAPH_ALIGNMENT paragraph)
{
    return GetFormat(kNumberFont, size, weight, alignment, paragraph);
}

void AppWindow::FillRounded(const D2D1_RECT_F& rect, float radius, uint32_t color, float alpha)
{
    ID2D1SolidColorBrush* brush = Brush(color, alpha);
    if (renderTarget_ != nullptr && brush != nullptr)
    {
        renderTarget_->FillRoundedRectangle(D2D1::RoundedRect(rect, radius, radius), brush);
    }
}

void AppWindow::StrokeRounded(const D2D1_RECT_F& rect, float radius, uint32_t color, float alpha,
                              float strokeWidth)
{
    ID2D1SolidColorBrush* brush = Brush(color, alpha);
    if (renderTarget_ != nullptr && brush != nullptr)
    {
        renderTarget_->DrawRoundedRectangle(D2D1::RoundedRect(rect, radius, radius), brush, strokeWidth);
    }
}

void AppWindow::FillEllipse(const D2D1_POINT_2F& center, float radiusX, float radiusY, uint32_t color,
                            float alpha)
{
    ID2D1SolidColorBrush* brush = Brush(color, alpha);
    if (renderTarget_ != nullptr && brush != nullptr)
    {
        renderTarget_->FillEllipse(D2D1::Ellipse(center, radiusX, radiusY), brush);
    }
}

void AppWindow::StrokeLine(const D2D1_POINT_2F& start, const D2D1_POINT_2F& end, uint32_t color,
                           float alpha, float strokeWidth)
{
    ID2D1SolidColorBrush* brush = Brush(color, alpha);
    if (renderTarget_ != nullptr && brush != nullptr)
    {
        renderTarget_->DrawLine(start, end, brush, strokeWidth);
    }
}

void AppWindow::DrawCard(const D2D1_RECT_F& rect, float radius)
{
    for (int layer = 5; layer >= 1; --layer)
    {
        const float spread = static_cast<float>(layer) * 1.35f;
        D2D1_RECT_F shadow = ExpandRect(rect, -spread, -spread);
        shadow.top += 2.0f;
        shadow.bottom += 4.0f;
        FillRounded(shadow, radius + spread, 0x5A4B39, 0.012f);
    }

    FillRounded(rect, radius, kSurface);
    StrokeRounded(rect, radius, kCardBorder, 0.95f, 1.0f);
}

void AppWindow::DrawText(const std::wstring& text, IDWriteTextFormat* format, const D2D1_RECT_F& rect,
                         uint32_t color, float alpha)
{
    if (text.empty() || format == nullptr || renderTarget_ == nullptr)
    {
        return;
    }
    ID2D1SolidColorBrush* brush = Brush(color, alpha);
    if (brush == nullptr)
    {
        return;
    }

    renderTarget_->DrawTextW(text.c_str(), static_cast<UINT32>(text.size()), format, rect, brush,
                             D2D1_DRAW_TEXT_OPTIONS_CLIP, DWRITE_MEASURING_MODE_NATURAL);
}

float AppWindow::MeasureText(const std::wstring& text, IDWriteTextFormat* format)
{
    if (text.empty() || format == nullptr || dwriteFactory_ == nullptr)
    {
        return 0.0f;
    }

    ComPtr<IDWriteTextLayout> layout;
    const HRESULT hr = dwriteFactory_->CreateTextLayout(text.c_str(), static_cast<UINT32>(text.size()),
                                                        format, 10000.0f, 200.0f, layout.GetAddressOf());
    if (FAILED(hr))
    {
        return 0.0f;
    }

    DWRITE_TEXT_METRICS metrics{};
    if (FAILED(layout->GetMetrics(&metrics)))
    {
        return 0.0f;
    }
    return metrics.widthIncludingTrailingWhitespace;
}

float AppWindow::DrawRun(const TextRun& run, float x, float y, float alpha)
{
    if (run.text.empty() || run.format == nullptr || dwriteFactory_ == nullptr || renderTarget_ == nullptr)
    {
        return 0.0f;
    }

    ComPtr<IDWriteTextLayout> layout;
    HRESULT hr = dwriteFactory_->CreateTextLayout(run.text.c_str(), static_cast<UINT32>(run.text.size()),
                                                  run.format, 10000.0f, 200.0f, layout.GetAddressOf());
    if (FAILED(hr))
    {
        return 0.0f;
    }

    ID2D1SolidColorBrush* brush = Brush(run.color, alpha);
    if (brush != nullptr)
    {
        renderTarget_->DrawTextLayout(D2D1::Point2F(x, y), layout.Get(), brush,
                                      D2D1_DRAW_TEXT_OPTIONS_CLIP);
    }

    DWRITE_TEXT_METRICS metrics{};
    if (FAILED(layout->GetMetrics(&metrics)))
    {
        return 0.0f;
    }
    return metrics.widthIncludingTrailingWhitespace;
}

void AppWindow::DrawGeometryAt(ID2D1Geometry* geometry, const D2D1_RECT_F& rect, uint32_t color, float alpha)
{
    if (geometry == nullptr || renderTarget_ == nullptr)
    {
        return;
    }

    ID2D1SolidColorBrush* brush = Brush(color, alpha);
    if (brush == nullptr)
    {
        return;
    }

    D2D1_MATRIX_3X2_F previous{};
    renderTarget_->GetTransform(&previous);
    const D2D1_MATRIX_3X2_F local =
        D2D1::Matrix3x2F::Scale(rect.right - rect.left, rect.bottom - rect.top) *
        D2D1::Matrix3x2F::Translation(rect.left, rect.top);
    renderTarget_->SetTransform(local * previous);
    renderTarget_->FillGeometry(geometry, brush);
    renderTarget_->SetTransform(previous);
}

void AppWindow::DrawBolt(const D2D1_RECT_F& rect, uint32_t color, float alpha)
{
    DrawGeometryAt(boltGeometry_.Get(), rect, color, alpha);
}

void AppWindow::DrawTShirt(const D2D1_RECT_F& rect, uint32_t color, float alpha)
{
    DrawGeometryAt(tshirtGeometry_.Get(), rect, color, alpha);
}

void AppWindow::DrawPlayIcon(const D2D1_RECT_F& rect, uint32_t color, float alpha)
{
    DrawGeometryAt(playGeometry_.Get(), rect, color, alpha);
}

void AppWindow::DrawGear(const D2D1_POINT_2F& center, float radius, uint32_t color, float alpha)
{
    for (int index = 0; index < 8; ++index)
    {
        const float angle = static_cast<float>(index) * 3.14159265f / 4.0f;
        const D2D1_POINT_2F inner = D2D1::Point2F(center.x + std::cos(angle) * radius * 0.58f,
                                                  center.y + std::sin(angle) * radius * 0.58f);
        const D2D1_POINT_2F outer = D2D1::Point2F(center.x + std::cos(angle) * radius,
                                                  center.y + std::sin(angle) * radius);
        StrokeLine(inner, outer, color, alpha, 1.5f);
    }

    ID2D1SolidColorBrush* brush = Brush(color, alpha);
    if (renderTarget_ != nullptr && brush != nullptr)
    {
        renderTarget_->DrawEllipse(D2D1::Ellipse(center, radius * 0.43f, radius * 0.43f), brush, 1.3f);
        renderTarget_->FillEllipse(D2D1::Ellipse(center, radius * 0.16f, radius * 0.16f), brush);
    }
}

void AppWindow::DrawMinimizeIcon(const D2D1_POINT_2F& center, uint32_t color, float alpha)
{
    StrokeLine(D2D1::Point2F(center.x - 7.0f, center.y + 2.0f),
               D2D1::Point2F(center.x + 7.0f, center.y + 2.0f), color, alpha, 1.4f);
}

void AppWindow::DrawCloseIcon(const D2D1_POINT_2F& center, uint32_t color, float alpha)
{
    StrokeLine(D2D1::Point2F(center.x - 5.0f, center.y - 5.0f),
               D2D1::Point2F(center.x + 5.0f, center.y + 5.0f), color, alpha, 1.4f);
    StrokeLine(D2D1::Point2F(center.x + 5.0f, center.y - 5.0f),
               D2D1::Point2F(center.x - 5.0f, center.y + 5.0f), color, alpha, 1.4f);
}

void AppWindow::DrawBackground()
{
    if (renderTarget_ == nullptr)
    {
        return;
    }

    if (backgroundBrush_ != nullptr)
    {
        renderTarget_->FillRectangle(D2D1::RectF(0.0f, 0.0f, kWidth, kHeight), backgroundBrush_.Get());
    }
    else
    {
        renderTarget_->Clear(D2D1::ColorF(kCream));
    }

    if (blueGlowBrush_ != nullptr)
    {
        renderTarget_->FillEllipse(D2D1::Ellipse(D2D1::Point2F(520.0f, 46.0f), 350.0f, 270.0f),
                                   blueGlowBrush_.Get());
    }
    if (warmGlowBrush_ != nullptr)
    {
        renderTarget_->FillEllipse(D2D1::Ellipse(D2D1::Point2F(-18.0f, 672.0f), 330.0f, 260.0f),
                                   warmGlowBrush_.Get());
    }
}

void AppWindow::DrawTitleBar()
{
    const D2D1_RECT_F logo = LogoRect();
    if (logoBrush_ != nullptr)
    {
        renderTarget_->FillRoundedRectangle(D2D1::RoundedRect(logo, 8.0f, 8.0f), logoBrush_.Get());
    }
    else
    {
        FillRounded(logo, 8.0f, kBlue);
    }
    StrokeRounded(logo, 8.0f, 0xFFFFFF, 0.18f, 1.0f);
    DrawBolt(MakeRect(27.0f, 18.5f, 41.0f, 34.0f), 0xFFFFFF);

    DrawText(L"上班鼓励器", FmtC(13.4f, DWRITE_FONT_WEIGHT_SEMI_BOLD, DWRITE_TEXT_ALIGNMENT_LEADING,
                                 DWRITE_PARAGRAPH_ALIGNMENT_CENTER),
             MakeRect(58.0f, 12.0f, 330.0f, 40.0f), 0x25272C);

    if (hoverGear_ > 0.01f)
    {
        FillRounded(GearHitRect(), 9.0f, 0x16181C, 0.055f * hoverGear_);
    }
    if (hoverMinimize_ > 0.01f)
    {
        FillRounded(MinimizeHitRect(), 9.0f, 0x16181C, 0.055f * hoverMinimize_);
    }
    if (hoverClose_ > 0.01f)
    {
        FillRounded(CloseHitRect(), 9.0f, 0xC64040, 0.10f * hoverClose_);
    }

    DrawGear(CenterOf(GearHitRect()), 7.1f, MixColor(0x73767C, 0x25272C, hoverGear_), 1.0f);
    DrawMinimizeIcon(CenterOf(MinimizeHitRect()), MixColor(0x73767C, 0x25272C, hoverMinimize_), 1.0f);
    DrawCloseIcon(CenterOf(CloseHitRect()), MixColor(0x73767C, 0xC64040, hoverClose_), 1.0f);

    StrokeLine(D2D1::Point2F(20.0f, 52.0f), D2D1::Point2F(540.0f, 52.0f), kBorder, 0.72f, 1.0f);
}

std::wstring AppWindow::BuildStatusPillLabel(const WorkSnapshot& snapshot) const
{
    switch (snapshot.phase)
    {
    case WorkPhase::Weekend:
        return L"休息日";
    case WorkPhase::BeforeWork:
        return L"等待开工";
    case WorkPhase::Working:
        return L"实时到账";
    case WorkPhase::Paused:
        return L"已暂停";
    case WorkPhase::Finished:
        return L"已结算";
    }
    return L"";
}

std::wstring AppWindow::BuildCountdownLabel(const WorkSnapshot& snapshot) const
{
    switch (snapshot.phase)
    {
    case WorkPhase::BeforeWork:
        return L"距离上班还有  ";
    case WorkPhase::Working:
        return L"距离下班还有  ";
    case WorkPhase::Paused:
        return L"已暂停  ·  距离下班还有  ";
    default:
        return L"";
    }
}

std::wstring AppWindow::BuildCountdownValue(const WorkSnapshot& snapshot) const
{
    switch (snapshot.phase)
    {
    case WorkPhase::Weekend:
        return L"今天不上班，愿你好好休息。";
    case WorkPhase::BeforeWork:
        return FormatDurationHms(snapshot.untilStartSeconds);
    case WorkPhase::Working:
    case WorkPhase::Paused:
        return FormatDurationHms(snapshot.remainingSeconds);
    case WorkPhase::Finished:
        return L"今天的工时已经结算，去过生活吧。";
    }
    return L"";
}

std::wstring AppWindow::BuildProgressStatus(const WorkSnapshot& snapshot) const
{
    switch (snapshot.phase)
    {
    case WorkPhase::Weekend:
        return L"今天休息";
    case WorkPhase::BeforeWork:
        return L"还没开始计时";
    case WorkPhase::Working:
        return L"预计 " + FormatClock(settings_.endMinutes) + L" 下班";
    case WorkPhase::Paused:
        return L"计时已暂停";
    case WorkPhase::Finished:
        return L"今日已结算";
    }
    return L"";
}

void AppWindow::DrawGoalSection(const WorkSnapshot& snapshot)
{
    const auto& goals = BuiltInSmallGoals();
    int achievedCount = 0;
    int nextIndex = -1;
    for (size_t index = 0; index < goals.size(); ++index)
    {
        if (snapshot.earnedToday + 0.00001 >= goals[index].price)
        {
            ++achievedCount;
        }
        else if (nextIndex < 0)
        {
            nextIndex = static_cast<int>(index);
        }
    }

    const D2D1_RECT_F header = GoalHeaderRect();
    DrawText(L"小目标", FmtC(11.5f, DWRITE_FONT_WEIGHT_SEMI_BOLD, DWRITE_TEXT_ALIGNMENT_LEADING,
                             DWRITE_PARAGRAPH_ALIGNMENT_CENTER),
             MakeRect(32.0f, header.top, 80.0f, header.bottom), 0x5F6269);
    DrawText(L"已达成 " + std::to_wstring(achievedCount) + L" / " + std::to_wstring(goals.size()),
             FmtC(10.4f, DWRITE_FONT_WEIGHT_NORMAL, DWRITE_TEXT_ALIGNMENT_LEADING,
                  DWRITE_PARAGRAPH_ALIGNMENT_CENTER),
             MakeRect(79.0f, header.top, 220.0f, header.bottom),
             achievedCount > 0 ? 0x5D876F : 0x9B9C98);

    if (nextIndex >= 0)
    {
        const double remaining = goals[static_cast<size_t>(nextIndex)].price - snapshot.earnedToday;
        const std::wstring nextText = L"还差 " + FormatMoney(remaining, 2) + L" · " +
                                      goals[static_cast<size_t>(nextIndex)].fullName;
        DrawText(nextText, FmtC(10.4f, DWRITE_FONT_WEIGHT_NORMAL, DWRITE_TEXT_ALIGNMENT_TRAILING,
                                DWRITE_PARAGRAPH_ALIGNMENT_CENTER),
                 MakeRect(220.0f, header.top, 528.0f, header.bottom), 0xB06C4F);
    }
    else
    {
        DrawText(L"今天的小目标已全部拿下", FmtC(10.4f, DWRITE_FONT_WEIGHT_SEMI_BOLD,
                                                DWRITE_TEXT_ALIGNMENT_TRAILING,
                                                DWRITE_PARAGRAPH_ALIGNMENT_CENTER),
                 MakeRect(220.0f, header.top, 528.0f, header.bottom), kGreen);
    }

    constexpr float tileTop = 318.0f;
    constexpr float tileBottom = 350.0f;
    constexpr float gap = 6.0f;
    const float tileWidth = (kContentWidth - gap * static_cast<float>(goals.size() - 1)) /
                            static_cast<float>(goals.size());
    IDWriteTextFormat* nameFormat = FmtC(10.5f, DWRITE_FONT_WEIGHT_SEMI_BOLD,
                                         DWRITE_TEXT_ALIGNMENT_LEADING, DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
    IDWriteTextFormat* priceFormat = FmtN(10.5f, DWRITE_FONT_WEIGHT_SEMI_BOLD,
                                          DWRITE_TEXT_ALIGNMENT_LEADING, DWRITE_PARAGRAPH_ALIGNMENT_CENTER);

    for (size_t index = 0; index < goals.size(); ++index)
    {
        const bool achieved = snapshot.earnedToday + 0.00001 >= goals[index].price;
        const bool next = static_cast<int>(index) == nextIndex;
        const float left = kContentLeft + static_cast<float>(index) * (tileWidth + gap);
        const D2D1_RECT_F tile = MakeRect(left, tileTop, left + tileWidth, tileBottom);

        uint32_t fillColor = 0xF6F4EE;
        uint32_t borderColor = 0xEAE6DD;
        uint32_t textColor = 0x8E908A;
        uint32_t indicatorColor = 0xC8C4B9;
        if (achieved)
        {
            fillColor = 0xEAF6EF;
            borderColor = 0xD9ECE1;
            textColor = 0x2F6B54;
            indicatorColor = 0x3D8768;
        }
        else if (next)
        {
            fillColor = 0xFFF7F1;
            borderColor = 0xF1DCCE;
            textColor = 0x8B5A42;
            indicatorColor = 0xD9784E;
        }

        FillRounded(tile, 9.0f, fillColor);
        StrokeRounded(tile, 9.0f, borderColor, 0.95f, 1.0f);

        const float centerY = (tileTop + tileBottom) * 0.5f;
        const float indicatorWidth = achieved ? 8.0f : 7.0f;
        const float nameWidth = MeasureText(goals[index].shortName, nameFormat);
        const std::wstring priceText = FormatMoney(goals[index].price, 0);
        const float priceWidth = MeasureText(priceText, priceFormat);
        const float totalWidth = indicatorWidth + 5.0f + nameWidth + 4.0f + priceWidth;
        const float startX = tile.left + (tileWidth - totalWidth) * 0.5f;
        const float textX = startX + indicatorWidth + 5.0f;

        if (achieved)
        {
            StrokeLine(D2D1::Point2F(startX + 0.5f, centerY),
                       D2D1::Point2F(startX + 3.0f, centerY + 3.0f), indicatorColor, 1.0f, 1.5f);
            StrokeLine(D2D1::Point2F(startX + 3.0f, centerY + 3.0f),
                       D2D1::Point2F(startX + 8.0f, centerY - 4.0f), indicatorColor, 1.0f, 1.5f);
        }
        else
        {
            FillEllipse(D2D1::Point2F(startX + 3.5f, centerY), 2.4f, 2.4f, indicatorColor, next ? 0.95f : 0.8f);
        }

        DrawText(goals[index].shortName, nameFormat,
                 MakeRect(textX, tile.top, textX + nameWidth + 2.0f, tile.bottom), textColor);
        DrawText(priceText, priceFormat,
                 MakeRect(textX + nameWidth + 4.0f, tile.top, tile.right - 4.0f, tile.bottom), textColor);
    }
}

void AppWindow::DrawHero(const WorkSnapshot& snapshot)
{
    const std::wstring dateText = FormatDateLine(snapshot.now);
    const std::wstring clockText = FormatClockLine(snapshot.now);
    DrawText(dateText, FmtC(11.6f, DWRITE_FONT_WEIGHT_NORMAL), MakeRect(32.0f, 69.0f, 340.0f, 89.0f),
             0x8C8E94);
    DrawText(clockText, FmtN(11.6f, DWRITE_FONT_WEIGHT_NORMAL, DWRITE_TEXT_ALIGNMENT_TRAILING),
             MakeRect(360.0f, 69.0f, 528.0f, 89.0f), 0xA1A39F);

    const uint32_t statusColor = StatusColor(snapshot.phase);
    const D2D1_POINT_2F dotCenter = D2D1::Point2F(39.0f, 112.0f);
    if (snapshot.phase == WorkPhase::Working)
    {
        const double seconds = static_cast<double>(GetTickCount64()) / 1000.0;
        const float pulse = 0.5f + 0.5f * static_cast<float>(std::sin(seconds * 2.7));
        FillEllipse(dotCenter, 6.2f + pulse * 2.5f, 6.2f + pulse * 2.5f, statusColor, 0.10f);
        FillEllipse(dotCenter, 3.9f + pulse * 0.5f, 3.9f + pulse * 0.5f, statusColor);
    }
    else
    {
        FillEllipse(dotCenter, 3.8f, 3.8f, statusColor, 0.92f);
    }

    DrawText(PhaseLabel(snapshot.phase),
             FmtC(31.2f, DWRITE_FONT_WEIGHT_BOLD, DWRITE_TEXT_ALIGNMENT_LEADING,
                  DWRITE_PARAGRAPH_ALIGNMENT_CENTER),
             MakeRect(52.0f, 89.0f, 360.0f, 135.0f), statusColor);

    const std::wstring countdownLabel = BuildCountdownLabel(snapshot);
    const std::wstring countdownValue = BuildCountdownValue(snapshot);
    const D2D1_RECT_F countdownArea = MakeRect(32.0f, 140.0f, 528.0f, 164.0f);
    if (countdownLabel.empty())
    {
        DrawText(countdownValue, FmtC(14.2f, DWRITE_FONT_WEIGHT_NORMAL), countdownArea,
                 snapshot.phase == WorkPhase::Finished ? 0x476C5C : 0x747870);
    }
    else
    {
        const uint32_t labelColor = snapshot.phase == WorkPhase::Paused ? 0xAA6B1D : 0x7E8087;
        const float labelY = 140.0f;
        float x = 32.0f;
        TextRun labelRun{ countdownLabel, FmtC(14.2f, DWRITE_FONT_WEIGHT_NORMAL), labelColor };
        x += DrawRun(labelRun, x, labelY);

        TextRun valueRun{ countdownValue, FmtC(14.2f, DWRITE_FONT_WEIGHT_SEMI_BOLD), 0x27292E };
        DrawRun(valueRun, x, labelY);
    }

    DrawText(L"今日已赚", FmtC(12.3f, DWRITE_FONT_WEIGHT_NORMAL), MakeRect(32.0f, 182.0f, 240.0f, 202.0f),
             kMuted);

    const std::wstring pillLabel = BuildStatusPillLabel(snapshot);
    uint32_t pillBackground = 0xF0EFEA;
    uint32_t pillForeground = 0x85878C;
    uint32_t pillDot = 0xA7A9A4;
    switch (snapshot.phase)
    {
    case WorkPhase::Working:
        pillBackground = 0xE8F6EF;
        pillForeground = 0x157A57;
        pillDot = 0x1A9368;
        break;
    case WorkPhase::Paused:
        pillBackground = 0xFFF3E2;
        pillForeground = 0xA4620E;
        pillDot = 0xD08722;
        break;
    case WorkPhase::Finished:
        pillBackground = 0xEAF4EF;
        pillForeground = 0x3E725E;
        pillDot = 0x5D907C;
        break;
    case WorkPhase::Weekend:
        pillBackground = 0xF5F0E6;
        pillForeground = 0x8C7556;
        pillDot = 0xB89D78;
        break;
    default:
        break;
    }

    const D2D1_RECT_F pillRect = MakeRect(456.0f, 179.0f, 528.0f, 201.0f);
    FillRounded(pillRect, 11.0f, pillBackground);
    FillEllipse(D2D1::Point2F(468.0f, 190.0f), 2.4f, 2.4f, pillDot);
    DrawText(pillLabel, FmtC(10.1f, DWRITE_FONT_WEIGHT_SEMI_BOLD, DWRITE_TEXT_ALIGNMENT_LEADING,
                             DWRITE_PARAGRAPH_ALIGNMENT_CENTER),
             MakeRect(476.0f, 179.0f, 526.0f, 201.0f), pillForeground);

    const std::wstring amountText = FormatNumber(snapshot.earnedToday, 4);
    const std::wstring currencyText = L"¥";
    IDWriteTextFormat* currencyFormat = FmtC(23.0f, DWRITE_FONT_WEIGHT_NORMAL);
    IDWriteTextFormat* amountFormat = FmtN(56.0f, DWRITE_FONT_WEIGHT_SEMI_BOLD);
    const float currencyWidth = MeasureText(currencyText, currencyFormat);
    const float amountX = 32.0f + currencyWidth + 5.0f;
    float amountSize = 56.0f;
    while (amountFormat != nullptr && amountSize > 40.0f &&
           MeasureText(amountText, amountFormat) > (kContentRight - amountX))
    {
        amountSize -= 4.0f;
        amountFormat = FmtN(amountSize, DWRITE_FONT_WEIGHT_SEMI_BOLD);
    }

    DrawText(currencyText, currencyFormat, MakeRect(32.0f, 224.0f, 80.0f, 256.0f), 0x5E6168);
    DrawText(amountText, amountFormat, MakeRect(amountX, 196.0f, kContentRight, 280.0f), kInk);

    const std::wstring rateText = FormatMoney(snapshot.perSecond, 4);
    IDWriteTextFormat* rateLabelFormat = FmtC(12.2f, DWRITE_FONT_WEIGHT_NORMAL);
    IDWriteTextFormat* rateValueFormat = FmtN(12.8f, DWRITE_FONT_WEIGHT_SEMI_BOLD);
    IDWriteTextFormat* rateUnitFormat = FmtC(11.7f, DWRITE_FONT_WEIGHT_NORMAL);
    float rateX = 32.0f;
    DrawRun({ L"当前赚钱速度", rateLabelFormat, 0x8A8C92 }, rateX, 277.0f);
    rateX += MeasureText(L"当前赚钱速度", rateLabelFormat) + 8.0f;
    rateX += DrawRun({ rateText, rateValueFormat, 0x34373D }, rateX, 276.5f);
    DrawRun({ L"  /  秒", rateUnitFormat, 0x8A8C92 }, rateX, 277.0f);
}

void AppWindow::DrawPrimaryButton(const WorkSnapshot& snapshot)
{
    const D2D1_RECT_F button = PrimaryButtonRect();
    const bool active = snapshot.phase == WorkPhase::Working || snapshot.phase == WorkPhase::Paused;

    std::wstring label;
    switch (snapshot.phase)
    {
    case WorkPhase::Working:
        label = L"暂停计时器";
        break;
    case WorkPhase::Paused:
        label = L"继续计时";
        break;
    case WorkPhase::BeforeWork:
        label = L"还没到上班时间";
        break;
    case WorkPhase::Finished:
        label = L"今天已经下班啦";
        break;
    case WorkPhase::Weekend:
        label = L"今天不上班";
        break;
    }

    if (active)
    {
        D2D1_RECT_F shadow = button;
        shadow.top += 4.0f;
        shadow.bottom += 4.0f;
        FillRounded(ExpandRect(shadow, -1.0f, 0.0f), 14.0f, 0x0B0C10, 0.09f);

        uint32_t base = snapshot.phase == WorkPhase::Paused ? kBlue : 0x17191D;
        uint32_t hovered = snapshot.phase == WorkPhase::Paused ? 0x2A5CF0 : 0x2C3037;
        FillRounded(button, 13.0f, MixColor(base, hovered, hoverPrimary_));
    }
    else
    {
        FillRounded(button, 13.0f, 0xEFEDE7);
        StrokeRounded(button, 13.0f, 0xE9E5DC, 0.8f, 1.0f);
    }

    IDWriteTextFormat* textFormat = FmtC(14.2f, DWRITE_FONT_WEIGHT_SEMI_BOLD, DWRITE_TEXT_ALIGNMENT_LEADING,
                                         DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
    const float textWidth = MeasureText(label, textFormat);
    const float iconWidth = active ? 20.0f : 0.0f;
    const float startX = (kWidth - (textWidth + iconWidth)) * 0.5f;
    const uint32_t textColor = active ? 0xFFFFFF : 0x9B9C98;

    if (active)
    {
        const float centerY = (button.top + button.bottom) * 0.5f;
        if (snapshot.phase == WorkPhase::Working)
        {
            FillRounded(MakeRect(startX, centerY - 6.5f, startX + 3.2f, centerY + 6.5f), 1.6f, 0xFFFFFF);
            FillRounded(MakeRect(startX + 6.0f, centerY - 6.5f, startX + 9.2f, centerY + 6.5f), 1.6f,
                        0xFFFFFF);
        }
        else
        {
            DrawPlayIcon(MakeRect(startX, centerY - 7.0f, startX + 10.5f, centerY + 7.0f), 0xFFFFFF);
        }
    }

    DrawText(label, textFormat,
             MakeRect(startX + iconWidth, button.top, startX + iconWidth + textWidth + 6.0f, button.bottom),
             textColor);
}

void AppWindow::DrawProgressCard(const WorkSnapshot& snapshot)
{
    const D2D1_RECT_F card = ProgressCardRect();
    DrawCard(card);

    DrawText(L"今日工时进度", FmtC(12.2f, DWRITE_FONT_WEIGHT_NORMAL),
             MakeRect(52.0f, 444.0f, 260.0f, 466.0f), 0x808289);
    DrawText(FormatPercent(snapshot.progress),
             FmtN(13.2f, DWRITE_FONT_WEIGHT_SEMI_BOLD, DWRITE_TEXT_ALIGNMENT_TRAILING),
             MakeRect(360.0f, 442.0f, 508.0f, 468.0f), kBlue);

    float infoX = 52.0f;
    infoX += DrawRun({ L"已工作  ", FmtC(11.5f, DWRITE_FONT_WEIGHT_NORMAL), 0x96989C }, infoX, 469.0f);
    infoX += DrawRun({ FormatNumber(snapshot.workedHours, 2), FmtN(11.8f, DWRITE_FONT_WEIGHT_SEMI_BOLD),
                       0x45484E }, infoX, 468.5f);
    infoX += DrawRun({ L" / ", FmtC(11.5f, DWRITE_FONT_WEIGHT_NORMAL), 0x96989C }, infoX, 469.0f);
    infoX += DrawRun({ FormatNumber(snapshot.shiftHours, 2), FmtN(11.8f, DWRITE_FONT_WEIGHT_SEMI_BOLD),
                       0x45484E }, infoX, 468.5f);
    DrawRun({ L" 小时", FmtC(11.5f, DWRITE_FONT_WEIGHT_NORMAL), 0x96989C }, infoX, 469.0f);

    DrawText(BuildProgressStatus(snapshot),
             FmtC(10.6f, DWRITE_FONT_WEIGHT_NORMAL, DWRITE_TEXT_ALIGNMENT_TRAILING),
             MakeRect(330.0f, 468.0f, 508.0f, 489.0f), 0xB2B3AE);

    const D2D1_RECT_F track = MakeRect(52.0f, 498.0f, 508.0f, 505.0f);
    FillRounded(track, 3.5f, 0xEFEDE7);

    const float fraction = Clamp01(static_cast<float>(snapshot.progress));
    if (fraction > 0.001f)
    {
        const float fillWidth = std::max(7.0f, (track.right - track.left) * fraction);
        const D2D1_RECT_F fill = MakeRect(track.left, track.top, track.left + fillWidth, track.bottom);
        if (progressBrush_ != nullptr)
        {
            renderTarget_->FillRoundedRectangle(D2D1::RoundedRect(fill, 3.5f, 3.5f), progressBrush_.Get());
        }
        else
        {
            FillRounded(fill, 3.5f, kBlue);
        }

        if (fraction > 0.025f)
        {
            const D2D1_POINT_2F tip = D2D1::Point2F(std::min(fill.right, track.right), 501.5f);
            FillEllipse(tip, 5.0f, 5.0f, 0x2A5AF2, 0.12f);
            FillEllipse(tip, 2.4f, 2.4f, 0x2A5AF2, 0.95f);
        }
    }

    StrokeLine(D2D1::Point2F(52.0f, 523.0f), D2D1::Point2F(508.0f, 523.0f), kBorder, 0.86f, 1.0f);

    DrawText(L"本月累计", FmtC(12.2f, DWRITE_FONT_WEIGHT_NORMAL),
             MakeRect(52.0f, 537.0f, 180.0f, 557.0f), 0x808289);
    DrawText(FormatMoney(snapshot.monthlyEarned, 2, true),
             FmtN(22.5f, DWRITE_FONT_WEIGHT_SEMI_BOLD, DWRITE_TEXT_ALIGNMENT_TRAILING),
             MakeRect(232.0f, 527.0f, 508.0f, 560.0f), kInk);

    DrawText(L"已过的日子按满勤补算。本月只计已赚，不含亏损。",
             FmtC(10.6f, DWRITE_FONT_WEIGHT_NORMAL), MakeRect(52.0f, 562.0f, 508.0f, 581.0f),
             0xA6A7A2);
}

void AppWindow::DrawFooter(const WorkSnapshot& snapshot)
{
    StrokeLine(D2D1::Point2F(32.0f, 608.0f), D2D1::Point2F(528.0f, 608.0f), kBorder, 0.85f, 1.0f);
    StrokeLine(D2D1::Point2F(196.0f, 618.0f), D2D1::Point2F(196.0f, 658.0f), kBorder, 0.7f, 1.0f);
    StrokeLine(D2D1::Point2F(368.0f, 618.0f), D2D1::Point2F(368.0f, 658.0f), kBorder, 0.7f, 1.0f);

    DrawText(L"月薪", FmtC(10.2f, DWRITE_FONT_WEIGHT_NORMAL), MakeRect(32.0f, 616.0f, 180.0f, 633.0f),
             0x94968F);
    DrawRun({ FormatMoney(settings_.monthlySalary, 2), FmtN(17.4f, DWRITE_FONT_WEIGHT_SEMI_BOLD),
              0x25272C }, 32.0f, 633.0f);

    DrawText(L"折算时薪", FmtC(10.2f, DWRITE_FONT_WEIGHT_NORMAL), MakeRect(212.0f, 616.0f, 348.0f, 633.0f),
             0x94968F);
    DrawRun({ FormatMoney(snapshot.hourlyRate, 2), FmtN(17.4f, DWRITE_FONT_WEIGHT_SEMI_BOLD),
              0x25272C }, 212.0f, 633.0f);

    DrawText(L"今日满勤可得", FmtC(10.2f, DWRITE_FONT_WEIGHT_NORMAL),
             MakeRect(384.0f, 616.0f, 528.0f, 633.0f), 0x94968F);
    DrawText(FormatMoney(snapshot.fullDayPay, 2), FmtN(17.4f, DWRITE_FONT_WEIGHT_SEMI_BOLD),
             MakeRect(384.0f, 633.0f, 528.0f, 658.0f), 0x25272C);

    DrawText(L"所有数据仅保存在本机", FmtC(10.1f, DWRITE_FONT_WEIGHT_NORMAL),
             MakeRect(32.0f, 682.0f, 260.0f, 700.0f), 0xA6A7A2);
    DrawText(L"认真工作，也认真生活", FmtC(10.1f, DWRITE_FONT_WEIGHT_NORMAL,
                                          DWRITE_TEXT_ALIGNMENT_TRAILING),
             MakeRect(340.0f, 682.0f, 528.0f, 700.0f), 0xB08F76);
}

void AppWindow::DrawOuterBorder()
{
    StrokeRounded(MakeRect(0.5f, 0.5f, kWidth - 0.5f, kHeight - 0.5f), 13.0f, 0xDCD6CB, 0.7f, 1.0f);
}

void AppWindow::DrawSettingsField(int index, const std::wstring& label, const D2D1_RECT_F& box,
                                  const std::wstring& prefix, const std::wstring& suffix)
{
    const bool focused = focusedField_ == static_cast<Field>(index);
    const bool hasError = fieldErrors_[static_cast<size_t>(index)];
    const uint32_t labelColor = hasError ? 0xBC4343 : 0x83858C;

    DrawText(label, FmtC(10.7f, DWRITE_FONT_WEIGHT_NORMAL), FieldLabelRect(index), labelColor);

    if (focused && !hasError)
    {
        StrokeRounded(ExpandRect(box, -2.0f, -2.0f), 14.0f, kBlue, 0.08f, 3.0f);
    }

    FillRounded(box, 12.0f, focused ? 0xFBFDFF : 0xF8F6F1);
    StrokeRounded(box, 12.0f, hasError ? 0xD14B4B : (focused ? kBlue : 0xE7E2D8),
                  hasError || focused ? 1.0f : 0.95f, focused ? 1.6f : 1.0f);

    const std::wstring& value = editValues_[static_cast<size_t>(index)];
    IDWriteTextFormat* valueFormat = FmtN(19.0f, DWRITE_FONT_WEIGHT_SEMI_BOLD, DWRITE_TEXT_ALIGNMENT_LEADING,
                                          DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
    IDWriteTextFormat* prefixFormat = FmtC(13.6f, DWRITE_FONT_WEIGHT_NORMAL, DWRITE_TEXT_ALIGNMENT_LEADING,
                                           DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
    float valueX = box.left + 14.0f;
    if (!prefix.empty())
    {
        DrawText(prefix, prefixFormat, MakeRect(valueX, box.top, valueX + 22.0f, box.bottom), 0x96989D);
        valueX += 17.0f;
    }

    if (!suffix.empty())
    {
        DrawText(suffix, FmtC(10.1f, DWRITE_FONT_WEIGHT_NORMAL, DWRITE_TEXT_ALIGNMENT_TRAILING,
                              DWRITE_PARAGRAPH_ALIGNMENT_CENTER),
                 MakeRect(box.right - 112.0f, box.top, box.right - 14.0f, box.bottom), 0xAAABA6);
    }

    if (focused && selectAll_ && !value.empty())
    {
        const float width = MeasureText(value, valueFormat);
        FillRounded(MakeRect(valueX - 3.0f, box.top + 13.0f, valueX + width + 5.0f, box.bottom - 13.0f), 5.0f,
                    kBlue, 0.11f);
    }

    DrawText(value, valueFormat, MakeRect(valueX, box.top, box.right - (suffix.empty() ? 14.0f : 116.0f),
                                          box.bottom),
             hasError ? 0xA63D3D : kInk);

    if (focused && !selectAll_ && (GetTickCount64() / 540) % 2 == 0)
    {
        const float textWidth = MeasureText(value, valueFormat);
        const float caretX = std::min(valueX + textWidth + 1.5f, box.right - (suffix.empty() ? 16.0f : 118.0f));
        FillRounded(MakeRect(caretX, box.top + 16.0f, caretX + 1.5f, box.bottom - 16.0f), 0.75f, kBlue);
    }
}

void AppWindow::DrawSettingsOverlay(const WorkSnapshot&)
{
    ID2D1SolidColorBrush* dim = Brush(0x121318, 0.42f);
    if (dim != nullptr)
    {
        renderTarget_->FillRectangle(D2D1::RectF(0.0f, 0.0f, kWidth, kHeight), dim);
    }

    const D2D1_RECT_F panel = SettingsPanelRect();
    DrawCard(panel, 23.0f);

    DrawText(L"调整你的节奏", FmtC(22.0f, DWRITE_FONT_WEIGHT_BOLD), MakeRect(88.0f, 101.0f, 420.0f, 136.0f),
             kInk);
    DrawText(L"所有数据只保存在这台电脑上，月薪按平均 21 个工作日折算。",
             FmtC(11.2f, DWRITE_FONT_WEIGHT_NORMAL),
             MakeRect(88.0f, 138.0f, 440.0f, 160.0f), 0x85878C);

    if (hoveredZone_ == Zone::DialogClose)
    {
        FillRounded(SettingsCloseRect(), 9.0f, 0x16181C, 0.055f);
    }
    DrawCloseIcon(CenterOf(SettingsCloseRect()),
                  MixColor(0x73767C, 0xC64040, hoveredZone_ == Zone::DialogClose ? 1.0f : 0.0f), 1.0f);

    DrawSettingsField(kStartField, L"上班时间", FieldBoxRect(kStartField), L"", L"");
    DrawSettingsField(kEndField, L"下班时间", FieldBoxRect(kEndField), L"", L"");
    DrawSettingsField(kSalaryField, L"月薪", FieldBoxRect(kSalaryField), L"¥", L"元 / 月");

    const D2D1_RECT_F preview = PreviewRect();
    FillRounded(preview, 12.0f, 0xF7F5EF);
    int durationMinutes = 0;
    double dailyPay = 0.0;
    double hourlyRate = 0.0;
    const bool previewValid = PreviewShift(durationMinutes, dailyPay, hourlyRate);

    DrawText(L"折算基准", FmtC(9.9f, DWRITE_FONT_WEIGHT_NORMAL),
             MakeRect(104.0f, 369.0f, 168.0f, 387.0f), 0x979991);
    DrawText(L"21 个工作日 / 月", FmtC(10.8f, DWRITE_FONT_WEIGHT_SEMI_BOLD),
             MakeRect(168.0f, 368.0f, 286.0f, 388.0f), 0x34363B);
    DrawText(previewValid ? L"本班 " + FormatDurationHm(durationMinutes * 60) : L"本班 --",
             FmtC(9.9f, DWRITE_FONT_WEIGHT_NORMAL, DWRITE_TEXT_ALIGNMENT_TRAILING),
             MakeRect(286.0f, 369.0f, 456.0f, 387.0f), 0x9A9B95);

    StrokeLine(D2D1::Point2F(282.0f, 391.0f), D2D1::Point2F(282.0f, 430.0f), 0xE7E3D9, 0.9f, 1.0f);

    DrawText(L"满勤日薪", FmtC(9.9f, DWRITE_FONT_WEIGHT_NORMAL), MakeRect(104.0f, 391.0f, 250.0f, 408.0f),
             0x979991);
    DrawText(previewValid ? FormatMoney(dailyPay, 2) : L"--",
             FmtN(17.2f, DWRITE_FONT_WEIGHT_SEMI_BOLD), MakeRect(104.0f, 405.0f, 278.0f, 432.0f), 0x292B30);

    DrawText(L"折算时薪", FmtC(9.9f, DWRITE_FONT_WEIGHT_NORMAL), MakeRect(300.0f, 391.0f, 440.0f, 408.0f),
             0x979991);
    DrawText(previewValid ? FormatMoney(hourlyRate, 2) : L"--",
             FmtN(17.2f, DWRITE_FONT_WEIGHT_SEMI_BOLD), MakeRect(300.0f, 405.0f, 456.0f, 432.0f), 0x292B30);

    DrawText(L"日薪 = 月薪 ÷ 21；时薪 = 日薪 ÷ 本班时长。",
             FmtC(9.4f, DWRITE_FONT_WEIGHT_NORMAL), MakeRect(104.0f, 431.0f, 456.0f, 446.0f),
             0xA4A59F);

    if (!settingsError_.empty())
    {
        DrawText(settingsError_, FmtC(10.7f, DWRITE_FONT_WEIGHT_NORMAL),
                 MakeRect(88.0f, 463.0f, 472.0f, 484.0f), 0xBE4444);
    }
    else
    {
        DrawText(L"按每月 21 个工作日折算，周末不计入。", FmtC(10.4f, DWRITE_FONT_WEIGHT_NORMAL),
                 MakeRect(88.0f, 463.0f, 472.0f, 484.0f), 0xA4A59F);
    }

    const D2D1_RECT_F cancel = DialogCancelRect();
    const D2D1_RECT_F save = DialogSaveRect();
    FillRounded(cancel, 13.0f, MixColor(0xF2F0EA, 0xE8E4DC, hoverDialogCancel_));
    FillRounded(save, 13.0f, MixColor(0x17191D, 0x2D3037, hoverDialogSave_));

    DrawText(L"取消", FmtC(13.8f, DWRITE_FONT_WEIGHT_SEMI_BOLD, DWRITE_TEXT_ALIGNMENT_CENTER,
                           DWRITE_PARAGRAPH_ALIGNMENT_CENTER),
             cancel, 0x5B5E65);
    DrawText(L"保存设置", FmtC(13.8f, DWRITE_FONT_WEIGHT_SEMI_BOLD, DWRITE_TEXT_ALIGNMENT_CENTER,
                               DWRITE_PARAGRAPH_ALIGNMENT_CENTER),
             save, 0xFFFFFF);

    DrawText(L"Tab 切换  ·  Enter 保存  ·  Esc 取消", FmtC(10.0f, DWRITE_FONT_WEIGHT_NORMAL,
                                                          DWRITE_TEXT_ALIGNMENT_CENTER),
             MakeRect(88.0f, 558.0f, 472.0f, 574.0f), 0xB1B2AD);
}

void AppWindow::Render()
{
    if (!EnsureDeviceResources())
    {
        return;
    }

    const WorkSnapshot snapshot = clock_.Snapshot();
    const bool overlay = settingsOpen_;
    const bool primaryActive = snapshot.phase == WorkPhase::Working || snapshot.phase == WorkPhase::Paused;

    Approach(hoverPrimary_, !overlay && primaryActive && hoveredZone_ == Zone::PrimaryButton);
    Approach(hoverGear_, !overlay && hoveredZone_ == Zone::SettingsButton);
    Approach(hoverMinimize_, !overlay && hoveredZone_ == Zone::MinimizeButton);
    Approach(hoverClose_, !overlay && hoveredZone_ == Zone::CloseButton);
    Approach(hoverDialogSave_, overlay && hoveredZone_ == Zone::DialogSave);
    Approach(hoverDialogCancel_, overlay && hoveredZone_ == Zone::DialogCancel);

    renderTarget_->BeginDraw();
    renderTarget_->SetTransform(D2D1::Matrix3x2F::Identity());
    renderTarget_->Clear(D2D1::ColorF(kCream));
    renderTarget_->SetTransform(D2D1::Matrix3x2F::Scale(uiScale_, uiScale_));

    DrawBackground();
    DrawTitleBar();
    DrawHero(snapshot);
    DrawGoalSection(snapshot);
    DrawPrimaryButton(snapshot);
    DrawProgressCard(snapshot);
    DrawFooter(snapshot);
    DrawOuterBorder();

    if (overlay)
    {
        DrawSettingsOverlay(snapshot);
    }

    renderTarget_->SetTransform(D2D1::Matrix3x2F::Identity());
    const HRESULT hr = renderTarget_->EndDraw();
    if (hr == D2DERR_RECREATE_TARGET)
    {
        DiscardDeviceResources();
        InvalidateRect(hwnd_, nullptr, FALSE);
    }
}

void AppWindow::Paint()
{
    PAINTSTRUCT paint{};
    HDC deviceContext = BeginPaint(hwnd_, &paint);
    if (!EnsureDeviceResources())
    {
        FillRect(deviceContext, &paint.rcPaint, static_cast<HBRUSH>(GetStockObject(WHITE_BRUSH)));
    }
    else
    {
        Render();
    }
    EndPaint(hwnd_, &paint);
}

D2D1_POINT_2F AppWindow::ClientToDesign(POINT point) const
{
    const float scale = uiScale_ > 0.0f ? uiScale_ : 1.0f;
    return D2D1::Point2F(static_cast<float>(point.x) / scale, static_cast<float>(point.y) / scale);
}

void AppWindow::UpdateWindowScale(UINT dpi, bool reposition)
{
    dpi_ = dpi != 0 ? dpi : 96;

    HMONITOR monitor = MonitorFromWindow(hwnd_, MONITOR_DEFAULTTONEAREST);
    MONITORINFO monitorInfo{};
    monitorInfo.cbSize = sizeof(monitorInfo);
    GetMonitorInfoW(monitor, &monitorInfo);

    const float workWidth = static_cast<float>(monitorInfo.rcWork.right - monitorInfo.rcWork.left);
    const float workHeight = static_cast<float>(monitorInfo.rcWork.bottom - monitorInfo.rcWork.top);
    const float desiredScale = static_cast<float>(dpi_) / 96.0f;
    float scale = std::min(desiredScale, std::min((workWidth - 64.0f) / kWidth, (workHeight - 64.0f) / kHeight));
    scale = std::max(scale, 0.45f);
    uiScale_ = scale;

    if (reposition)
    {
        const int width = std::max(1, static_cast<int>(std::lround(kWidth * scale)));
        const int height = std::max(1, static_cast<int>(std::lround(kHeight * scale)));
        const int x = monitorInfo.rcWork.left + (static_cast<int>(workWidth) - width) / 2;
        const int y = monitorInfo.rcWork.top + (static_cast<int>(workHeight) - height) / 2;
        SetWindowPos(hwnd_, nullptr, x, y, width, height, SWP_NOZORDER | SWP_NOACTIVATE);
    }
}

void AppWindow::CenterWindowOnMonitor(HMONITOR monitor)
{
    MONITORINFO monitorInfo{};
    monitorInfo.cbSize = sizeof(monitorInfo);
    if (!GetMonitorInfoW(monitor, &monitorInfo))
    {
        return;
    }

    RECT windowRect{};
    GetWindowRect(hwnd_, &windowRect);
    const int width = windowRect.right - windowRect.left;
    const int height = windowRect.bottom - windowRect.top;
    const int workWidth = monitorInfo.rcWork.right - monitorInfo.rcWork.left;
    const int workHeight = monitorInfo.rcWork.bottom - monitorInfo.rcWork.top;
    const int x = monitorInfo.rcWork.left + (workWidth - width) / 2;
    const int y = monitorInfo.rcWork.top + (workHeight - height) / 2;
    SetWindowPos(hwnd_, nullptr, x, y, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
}

AppWindow::Zone AppWindow::HitTest(float x, float y) const
{
    if (settingsOpen_)
    {
        const D2D1_RECT_F panel = SettingsPanelRect();
        if (!Contains(panel, x, y))
        {
            return Zone::DialogBackdrop;
        }
        if (Contains(SettingsCloseRect(), x, y))
        {
            return Zone::DialogClose;
        }
        if (Contains(FieldBoxRect(kStartField), x, y))
        {
            return Zone::FieldStart;
        }
        if (Contains(FieldBoxRect(kEndField), x, y))
        {
            return Zone::FieldEnd;
        }
        if (Contains(FieldBoxRect(kSalaryField), x, y))
        {
            return Zone::FieldSalary;
        }
        if (Contains(DialogSaveRect(), x, y))
        {
            return Zone::DialogSave;
        }
        if (Contains(DialogCancelRect(), x, y))
        {
            return Zone::DialogCancel;
        }
        return Zone::None;
    }

    if (Contains(GearHitRect(), x, y))
    {
        return Zone::SettingsButton;
    }
    if (Contains(MinimizeHitRect(), x, y))
    {
        return Zone::MinimizeButton;
    }
    if (Contains(CloseHitRect(), x, y))
    {
        return Zone::CloseButton;
    }
    if (Contains(PrimaryButtonRect(), x, y))
    {
        return Zone::PrimaryButton;
    }
    if (y >= 0.0f && y < 52.0f)
    {
        return Zone::TitleDrag;
    }
    return Zone::None;
}

bool AppWindow::IsClickable(Zone zone) const
{
    switch (zone)
    {
    case Zone::SettingsButton:
    case Zone::MinimizeButton:
    case Zone::CloseButton:
    case Zone::DialogClose:
    case Zone::DialogSave:
    case Zone::DialogCancel:
        return true;
    case Zone::PrimaryButton:
    {
        const WorkSnapshot snapshot = clock_.Snapshot();
        return snapshot.phase == WorkPhase::Working || snapshot.phase == WorkPhase::Paused;
    }
    default:
        return false;
    }
}

bool AppWindow::IsFieldZone(Zone zone) const
{
    return zone == Zone::FieldStart || zone == Zone::FieldEnd || zone == Zone::FieldSalary;
}

AppWindow::Field AppWindow::FieldFromZone(Zone zone) const
{
    switch (zone)
    {
    case Zone::FieldStart:
        return Field::Start;
    case Zone::FieldEnd:
        return Field::End;
    case Zone::FieldSalary:
        return Field::Salary;
    default:
        return Field::None;
    }
}

void AppWindow::UpdateHover(float x, float y)
{
    mousePoint_ = D2D1::Point2F(x, y);
    const Zone zone = HitTest(x, y);
    if (zone != hoveredZone_)
    {
        hoveredZone_ = zone;
        InvalidateRect(hwnd_, nullptr, FALSE);
    }

    if (!trackingMouse_)
    {
        TRACKMOUSEEVENT track{};
        track.cbSize = sizeof(track);
        track.dwFlags = TME_LEAVE;
        track.hwndTrack = hwnd_;
        TrackMouseEvent(&track);
        trackingMouse_ = true;
    }
}

void AppWindow::OnMouseMove(float x, float y)
{
    if (draggingWindow_)
    {
        POINT cursor{};
        GetCursorPos(&cursor);
        const int dx = cursor.x - dragStartCursor_.x;
        const int dy = cursor.y - dragStartCursor_.y;
        SetWindowPos(hwnd_, nullptr, dragStartRect_.left + dx, dragStartRect_.top + dy, 0, 0,
                     SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
        return;
    }

    const float scale = uiScale_ > 0.0f ? uiScale_ : 1.0f;
    UpdateHover(x / scale, y / scale);
}

void AppWindow::OnMouseLeave()
{
    trackingMouse_ = false;
    if (!draggingWindow_)
    {
        hoveredZone_ = Zone::None;
        InvalidateRect(hwnd_, nullptr, FALSE);
    }
}

void AppWindow::OnMouseDown(float x, float y)
{
    SetFocus(hwnd_);
    const float scale = uiScale_ > 0.0f ? uiScale_ : 1.0f;
    const float designX = x / scale;
    const float designY = y / scale;
    const Zone zone = HitTest(designX, designY);

    if (settingsOpen_)
    {
        if (IsFieldZone(zone))
        {
            FocusField(FieldFromZone(zone));
            return;
        }
        if (zone == Zone::DialogClose || zone == Zone::DialogSave || zone == Zone::DialogCancel ||
            zone == Zone::DialogBackdrop)
        {
            pressedZone_ = zone;
            SetCapture(hwnd_);
            InvalidateRect(hwnd_, nullptr, FALSE);
        }
        return;
    }

    if (zone == Zone::TitleDrag)
    {
        BeginWindowDrag();
        return;
    }

    if (zone == Zone::SettingsButton || zone == Zone::MinimizeButton || zone == Zone::CloseButton ||
        (zone == Zone::PrimaryButton && IsClickable(zone)))
    {
        pressedZone_ = zone;
        SetCapture(hwnd_);
        InvalidateRect(hwnd_, nullptr, FALSE);
    }
}

void AppWindow::OnMouseUp(float x, float y)
{
    if (draggingWindow_)
    {
        EndWindowDrag();
        return;
    }

    const float scale = uiScale_ > 0.0f ? uiScale_ : 1.0f;
    const Zone released = HitTest(x / scale, y / scale);
    const Zone pressed = pressedZone_;
    pressedZone_ = Zone::None;

    if (GetCapture() == hwnd_)
    {
        ReleaseCapture();
    }

    if (pressed != Zone::None && released == pressed)
    {
        ActivateZone(pressed);
    }
    InvalidateRect(hwnd_, nullptr, FALSE);
}

void AppWindow::ActivateZone(Zone zone)
{
    switch (zone)
    {
    case Zone::SettingsButton:
        OpenSettings();
        break;
    case Zone::MinimizeButton:
        ShowWindow(hwnd_, SW_MINIMIZE);
        break;
    case Zone::CloseButton:
        PostMessageW(hwnd_, WM_CLOSE, 0, 0);
        break;
    case Zone::PrimaryButton:
        if (clock_.TogglePause())
        {
            InvalidateRect(hwnd_, nullptr, FALSE);
        }
        break;
    case Zone::DialogClose:
    case Zone::DialogCancel:
    case Zone::DialogBackdrop:
        CloseSettings();
        break;
    case Zone::DialogSave:
        SaveSettings();
        break;
    default:
        break;
    }
}

void AppWindow::BeginWindowDrag()
{
    if (settingsOpen_)
    {
        return;
    }

    GetCursorPos(&dragStartCursor_);
    GetWindowRect(hwnd_, &dragStartRect_);
    draggingWindow_ = true;
    SetCapture(hwnd_);
}

void AppWindow::EndWindowDrag()
{
    draggingWindow_ = false;
    if (GetCapture() == hwnd_)
    {
        ReleaseCapture();
    }
}

std::wstring AppWindow::FieldValue(Field field) const
{
    const size_t index = static_cast<size_t>(field);
    if (index >= editValues_.size())
    {
        return L"";
    }
    return editValues_[index];
}

std::wstring& AppWindow::FieldValueRef(Field field)
{
    static std::wstring empty;
    const size_t index = static_cast<size_t>(field);
    if (field == Field::None || index >= editValues_.size())
    {
        empty.clear();
        return empty;
    }
    return editValues_[index];
}

bool AppWindow::IsTimeField(Field field) const
{
    return field == Field::Start || field == Field::End;
}

void AppWindow::OpenSettings()
{
    if (settingsOpen_)
    {
        return;
    }

    editValues_[kStartField] = FormatClock(settings_.startMinutes);
    editValues_[kEndField] = FormatClock(settings_.endMinutes);
    editValues_[kSalaryField] = FormatNumber(settings_.monthlySalary, 2);
    fieldErrors_.fill(false);
    settingsError_.clear();
    focusedField_ = Field::Start;
    selectAll_ = true;
    caretEpoch_ = GetTickCount64();
    settingsOpen_ = true;
    hoveredZone_ = Zone::None;
    pressedZone_ = Zone::None;
    SetFocus(hwnd_);
    InvalidateRect(hwnd_, nullptr, FALSE);
}

void AppWindow::CloseSettings()
{
    if (!settingsOpen_)
    {
        return;
    }

    settingsOpen_ = false;
    focusedField_ = Field::None;
    selectAll_ = false;
    fieldErrors_.fill(false);
    settingsError_.clear();
    hoveredZone_ = Zone::None;
    pressedZone_ = Zone::None;
    InvalidateRect(hwnd_, nullptr, FALSE);
}

void AppWindow::FocusField(Field field)
{
    if (field == Field::None)
    {
        return;
    }
    focusedField_ = field;
    selectAll_ = true;
    settingsError_.clear();
    caretEpoch_ = GetTickCount64();
    InvalidateRect(hwnd_, nullptr, FALSE);
}

void AppWindow::SelectFocusedFieldText()
{
    if (focusedField_ != Field::None)
    {
        selectAll_ = true;
        InvalidateRect(hwnd_, nullptr, FALSE);
    }
}

void AppWindow::CycleField(int direction)
{
    int index = static_cast<int>(focusedField_);
    if (index < 0)
    {
        index = direction >= 0 ? -1 : 0;
    }
    index = (index + direction + 3) % 3;
    FocusField(static_cast<Field>(index));
}

void AppWindow::InsertCharacter(wchar_t character)
{
    if (focusedField_ == Field::None)
    {
        return;
    }

    const std::wstring normalized = NormalizeTypedCharacter(character);
    if (normalized.size() != 1)
    {
        return;
    }

    const wchar_t typed = normalized[0];
    std::wstring& value = FieldValueRef(focusedField_);
    const bool timeField = IsTimeField(focusedField_);
    const size_t maxLength = timeField ? 5 : 12;

    if (timeField)
    {
        if (!IsDigit(typed) && typed != L':')
        {
            return;
        }
    }
    else if (!IsDigit(typed) && typed != L'.')
    {
        return;
    }

    if (selectAll_)
    {
        value.clear();
        selectAll_ = false;
    }

    if (value.size() >= maxLength)
    {
        return;
    }

    if (timeField)
    {
        if (typed == L':')
        {
            if (value.empty() || value.find(L':') != std::wstring::npos)
            {
                return;
            }
            value.push_back(L':');
        }
        else
        {
            if (value.find(L':') == std::wstring::npos && value.size() == 2)
            {
                value.push_back(L':');
            }
            if (value.size() >= maxLength)
            {
                return;
            }
            value.push_back(typed);
        }
    }
    else
    {
        if (typed == L'.')
        {
            if (value.find(L'.') != std::wstring::npos)
            {
                return;
            }
            if (value.empty())
            {
                value = L"0";
            }
            value.push_back(L'.');
        }
        else
        {
            value.push_back(typed);
        }
    }

    fieldErrors_[static_cast<size_t>(focusedField_)] = false;
    settingsError_.clear();
    caretEpoch_ = GetTickCount64();
    InvalidateRect(hwnd_, nullptr, FALSE);
}

void AppWindow::BackspaceField()
{
    if (focusedField_ == Field::None)
    {
        return;
    }

    std::wstring& value = FieldValueRef(focusedField_);
    if (selectAll_)
    {
        value.clear();
        selectAll_ = false;
    }
    else if (!value.empty())
    {
        value.pop_back();
    }

    fieldErrors_[static_cast<size_t>(focusedField_)] = false;
    settingsError_.clear();
    caretEpoch_ = GetTickCount64();
    InvalidateRect(hwnd_, nullptr, FALSE);
}

bool AppWindow::PreviewShift(int& durationMinutes, double& dailyPay, double& hourlyRate) const
{
    int start = 0;
    int end = 0;
    double salary = 0.0;
    if (!ParseClockText(editValues_[kStartField], start) || !ParseClockText(editValues_[kEndField], end))
    {
        return false;
    }
    if (!ParseDecimalText(editValues_[kSalaryField], salary))
    {
        return false;
    }
    if (end <= start || salary <= 0.0 || salary > 10000000.0)
    {
        return false;
    }

    durationMinutes = end - start;
    dailyPay = salary / kAverageWorkdaysPerMonth;
    const double shiftHours = static_cast<double>(durationMinutes) / 60.0;
    if (shiftHours <= 0.0)
    {
        return false;
    }
    hourlyRate = dailyPay / shiftHours;
    return true;
}

void AppWindow::SaveSettings()
{
    if (!settingsOpen_)
    {
        return;
    }

    fieldErrors_.fill(false);
    settingsError_.clear();

    int start = 0;
    int end = 0;
    double salary = 0.0;
    bool valid = true;

    if (!ParseClockText(editValues_[kStartField], start))
    {
        fieldErrors_[kStartField] = true;
        settingsError_ = L"上班时间格式不对，试试 09:30。";
        valid = false;
    }
    if (!ParseClockText(editValues_[kEndField], end))
    {
        fieldErrors_[kEndField] = true;
        if (settingsError_.empty())
        {
            settingsError_ = L"下班时间格式不对，试试 18:00。";
        }
        valid = false;
    }
    if (!ParseDecimalText(editValues_[kSalaryField], salary) || salary <= 0.0 || salary > 10000000.0)
    {
        fieldErrors_[kSalaryField] = true;
        if (settingsError_.empty())
        {
            settingsError_ = L"月薪需要大于 0，且不超过 10000000 元。";
        }
        valid = false;
    }

    if (valid && end <= start)
    {
        fieldErrors_[kStartField] = true;
        fieldErrors_[kEndField] = true;
        settingsError_ = L"下班时间需要晚于上班时间。";
        valid = false;
    }

    if (!valid)
    {
        InvalidateRect(hwnd_, nullptr, FALSE);
        return;
    }

    AppSettings candidate;
    candidate.startMinutes = start;
    candidate.endMinutes = end;
    candidate.monthlySalary = salary;

    std::wstring reason;
    if (!candidate.IsValid(&reason))
    {
        settingsError_ = reason;
        InvalidateRect(hwnd_, nullptr, FALSE);
        return;
    }

    std::wstring saveError;
    if (!SettingsStore::Save(candidate, &saveError))
    {
        settingsError_ = saveError;
        InvalidateRect(hwnd_, nullptr, FALSE);
        return;
    }

    settings_ = candidate;
    clock_.SetSettings(candidate);
    CloseSettings();
}


