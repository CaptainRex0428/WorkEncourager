#pragma once

#include <windows.h>
#include <shellapi.h>

#include <d2d1.h>
#include <dwrite.h>
#include <wrl/client.h>

#include <array>
#include <string>
#include <unordered_map>
#include <vector>

#include "AppSettings.h"
#include "WorkStats.h"

class AppWindow
{
public:
    AppWindow();
    ~AppWindow();

    bool Create(HINSTANCE instance, int showCommand);
    int RunMessageLoop();

    static constexpr wchar_t kWindowClassName[] = L"WorkEncourager.MainWindow";
    static constexpr UINT kShowWindowMessage = WM_APP + 12;
    static constexpr UINT kExitMessage = WM_APP + 13;

private:
    static constexpr float kWidth = 560.0f;
    static constexpr float kHeight = 720.0f;
    static constexpr UINT_PTR kAnimationTimer = 1;

    enum class Zone
    {
        None,
        TitleDrag,
        SettingsButton,
        MinimizeButton,
        CloseButton,
        PrimaryButton,
        DialogBackdrop,
        DialogClose,
        FieldStart,
        FieldEnd,
        FieldSalary,
        DialogSave,
        DialogCancel
    };

    enum class Field
    {
        None = -1,
        Start = 0,
        End = 1,
        Salary = 2
    };

    struct TextRun
    {
        std::wstring text;
        IDWriteTextFormat* format = nullptr;
        uint32_t color = 0;
    };

    static LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
    LRESULT HandleMessage(UINT message, WPARAM wParam, LPARAM lParam);
    static ATOM RegisterWindowClass(HINSTANCE instance);

    bool InitializeGraphics();
    bool EnsureDeviceResources();
    void DiscardDeviceResources();
    void ResetDeviceBrushes();

    void Paint();
    void Render();
    void DrawBackground();
    void DrawTitleBar();
    void DrawHero(const WorkSnapshot& snapshot);
    void DrawGoalSection(const WorkSnapshot& snapshot);
    void DrawPrimaryButton(const WorkSnapshot& snapshot);
    void DrawProgressCard(const WorkSnapshot& snapshot);
    void DrawFooter(const WorkSnapshot& snapshot);
    void DrawSettingsOverlay(const WorkSnapshot& snapshot);
    void DrawSettingsField(int index, const std::wstring& label, const D2D1_RECT_F& box,
                           const std::wstring& prefix, const std::wstring& suffix);
    void DrawOuterBorder();

    ID2D1SolidColorBrush* Brush(uint32_t rgb, float alpha = 1.0f);
    IDWriteTextFormat* FmtC(float size, DWRITE_FONT_WEIGHT weight,
                            DWRITE_TEXT_ALIGNMENT alignment = DWRITE_TEXT_ALIGNMENT_LEADING,
                            DWRITE_PARAGRAPH_ALIGNMENT paragraph = DWRITE_PARAGRAPH_ALIGNMENT_NEAR);
    IDWriteTextFormat* FmtN(float size, DWRITE_FONT_WEIGHT weight,
                            DWRITE_TEXT_ALIGNMENT alignment = DWRITE_TEXT_ALIGNMENT_LEADING,
                            DWRITE_PARAGRAPH_ALIGNMENT paragraph = DWRITE_PARAGRAPH_ALIGNMENT_NEAR);
    IDWriteTextFormat* GetFormat(const wchar_t* family, float size, DWRITE_FONT_WEIGHT weight,
                                 DWRITE_TEXT_ALIGNMENT alignment, DWRITE_PARAGRAPH_ALIGNMENT paragraph);

    void FillRounded(const D2D1_RECT_F& rect, float radius, uint32_t color, float alpha = 1.0f);
    void StrokeRounded(const D2D1_RECT_F& rect, float radius, uint32_t color, float alpha = 1.0f,
                       float strokeWidth = 1.0f);
    void FillEllipse(const D2D1_POINT_2F& center, float radiusX, float radiusY, uint32_t color,
                     float alpha = 1.0f);
    void StrokeLine(const D2D1_POINT_2F& start, const D2D1_POINT_2F& end, uint32_t color,
                    float alpha = 1.0f, float strokeWidth = 1.0f);
    void DrawCard(const D2D1_RECT_F& rect, float radius = 18.0f);
    void DrawText(const std::wstring& text, IDWriteTextFormat* format, const D2D1_RECT_F& rect,
                  uint32_t color, float alpha = 1.0f);
    float MeasureText(const std::wstring& text, IDWriteTextFormat* format);
    float DrawRun(const TextRun& run, float x, float y, float alpha = 1.0f);
    void DrawGeometryAt(ID2D1Geometry* geometry, const D2D1_RECT_F& rect, uint32_t color,
                        float alpha = 1.0f);
    void DrawBolt(const D2D1_RECT_F& rect, uint32_t color, float alpha = 1.0f);
    void DrawTShirt(const D2D1_RECT_F& rect, uint32_t color, float alpha = 1.0f);
    void DrawPlayIcon(const D2D1_RECT_F& rect, uint32_t color, float alpha = 1.0f);
    void DrawGear(const D2D1_POINT_2F& center, float radius, uint32_t color, float alpha = 1.0f);
    void DrawMinimizeIcon(const D2D1_POINT_2F& center, uint32_t color, float alpha = 1.0f);
    void DrawCloseIcon(const D2D1_POINT_2F& center, uint32_t color, float alpha = 1.0f);

    Zone HitTest(float x, float y) const;
    bool IsClickable(Zone zone) const;
    bool IsFieldZone(Zone zone) const;
    Field FieldFromZone(Zone zone) const;
    void UpdateHover(float x, float y);
    void OnMouseDown(float x, float y);
    void OnMouseUp(float x, float y);
    void OnMouseMove(float x, float y);
    void OnMouseLeave();
    void ActivateZone(Zone zone);
    void BeginWindowDrag();
    void EndWindowDrag();

    void OpenSettings();
    void CloseSettings();
    void SaveSettings();
    void FocusField(Field field);
    void CycleField(int direction);
    void InsertCharacter(wchar_t character);
    void BackspaceField();
    void SelectFocusedFieldText();
    std::wstring FieldValue(Field field) const;
    std::wstring& FieldValueRef(Field field);
    bool IsTimeField(Field field) const;
    bool PreviewShift(int& durationMinutes, double& dailyPay, double& hourlyRate) const;

    std::wstring BuildStatusPillLabel(const WorkSnapshot& snapshot) const;
    std::wstring BuildCountdownLabel(const WorkSnapshot& snapshot) const;
    std::wstring BuildCountdownValue(const WorkSnapshot& snapshot) const;
    std::wstring BuildProgressStatus(const WorkSnapshot& snapshot) const;

    void UpdateWindowScale(UINT dpi, bool reposition);
    void CenterWindowOnMonitor(HMONITOR monitor);
    D2D1_POINT_2F ClientToDesign(POINT point) const;

    bool AddTrayIcon();
    void RemoveTrayIcon();
    void ShowTrayMenu();
    void HandleTrayEvent(WPARAM wParam, LPARAM lParam);
    void ShowMainWindow();
    bool EnsureTrayHoverWindow();
    void DestroyTrayHoverWindow();
    void PollTrayHover(ULONGLONG now);
    void ShowTrayHoverTip();
    void HideTrayHoverTip();
    bool GetTrayIconRect(RECT& iconRect) const;
    std::wstring BuildTrayHoverText(const WorkSnapshot& snapshot) const;

    HINSTANCE instance_ = nullptr;
    HWND hwnd_ = nullptr;
    UINT dpi_ = 96;
    float uiScale_ = 1.0f;

    NOTIFYICONDATAW trayIconData_{};
    bool trayIconAdded_ = false;
    bool exitRequested_ = false;
    UINT taskbarCreatedMessage_ = 0;
    HWND trayHoverWindow_ = nullptr;
    HFONT trayHoverFont_ = nullptr;
    UINT trayHoverDpi_ = 0;
    bool trayHoverTracking_ = false;
    bool trayHoverVisible_ = false;
    ULONGLONG trayHoverStarted_ = 0;
    ULONGLONG lastTrayHoverPoll_ = 0;

    AppSettings settings_;
    WorkClock clock_;
    bool settingsOpen_ = false;
    std::array<std::wstring, 3> editValues_{};
    std::array<bool, 3> fieldErrors_{};
    Field focusedField_ = Field::None;
    bool selectAll_ = false;
    std::wstring settingsError_;

    Zone hoveredZone_ = Zone::None;
    Zone pressedZone_ = Zone::None;
    bool trackingMouse_ = false;
    bool draggingWindow_ = false;
    POINT dragStartCursor_{};
    RECT dragStartRect_{};
    D2D1_POINT_2F mousePoint_{};

    float hoverPrimary_ = 0.0f;
    float hoverGear_ = 0.0f;
    float hoverMinimize_ = 0.0f;
    float hoverClose_ = 0.0f;
    float hoverDialogSave_ = 0.0f;
    float hoverDialogCancel_ = 0.0f;

    ULONGLONG caretEpoch_ = 0;

    Microsoft::WRL::ComPtr<ID2D1Factory> d2dFactory_;
    Microsoft::WRL::ComPtr<IDWriteFactory> dwriteFactory_;
    Microsoft::WRL::ComPtr<ID2D1HwndRenderTarget> renderTarget_;
    Microsoft::WRL::ComPtr<ID2D1LinearGradientBrush> backgroundBrush_;
    Microsoft::WRL::ComPtr<ID2D1RadialGradientBrush> blueGlowBrush_;
    Microsoft::WRL::ComPtr<ID2D1RadialGradientBrush> warmGlowBrush_;
    Microsoft::WRL::ComPtr<ID2D1LinearGradientBrush> logoBrush_;
    Microsoft::WRL::ComPtr<ID2D1LinearGradientBrush> progressBrush_;
    Microsoft::WRL::ComPtr<ID2D1PathGeometry> boltGeometry_;
    Microsoft::WRL::ComPtr<ID2D1PathGeometry> tshirtGeometry_;
    Microsoft::WRL::ComPtr<ID2D1PathGeometry> playGeometry_;

    std::unordered_map<uint32_t, Microsoft::WRL::ComPtr<ID2D1SolidColorBrush>> brushCache_;
    std::unordered_map<std::wstring, Microsoft::WRL::ComPtr<IDWriteTextFormat>> formatCache_;
};
