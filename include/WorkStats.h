#pragma once

#include "AppSettings.h"

#include <windows.h>

#include <array>
#include <chrono>
#include <string>

enum class WorkPhase
{
    Weekend,
    BeforeWork,
    Working,
    Paused,
    Finished
};

struct SmallGoal
{
    const wchar_t* shortName;
    const wchar_t* fullName;
    double price;
};

// The encouragement targets are intentionally fixed in code. They form a
// small price ladder, so the card can show multiple rewards without asking
// the user to maintain a list.
const std::array<SmallGoal, 5>& BuiltInSmallGoals();

struct WorkSnapshot
{
    SYSTEMTIME now{};
    WorkPhase phase = WorkPhase::BeforeWork;
    bool isWeekday = false;

    int shiftSeconds = 0;
    int elapsedSeconds = 0;
    int remainingSeconds = 0;
    int untilStartSeconds = 0;

    double shiftHours = 0.0;
    double workedHours = 0.0;
    double progress = 0.0;
    double hourlyRate = 0.0;
    double perSecond = 0.0;
    double earnedToday = 0.0;
    double fullDayPay = 0.0;
    double monthlyEarned = 0.0;

    int workdaysBefore = 0;
};

class WorkClock
{
public:
    explicit WorkClock(const AppSettings& settings);

    void SetSettings(const AppSettings& settings);
    const AppSettings& Settings() const noexcept { return settings_; }

    // Starts or ends a break. Only valid while the shift is in progress.
    bool TogglePause();
    bool IsPaused() const noexcept { return paused_; }

    WorkSnapshot Snapshot() const;

private:
    double TotalPausedSeconds() const;

    AppSettings settings_;
    bool paused_ = false;
    std::chrono::steady_clock::time_point pauseStarted_{};
    double pausedSeconds_ = 0.0;
};

const wchar_t* WeekdayName(int dayOfWeek);
const wchar_t* PhaseLabel(WorkPhase phase);

std::wstring FormatDateLine(const SYSTEMTIME& time);
std::wstring FormatClockLine(const SYSTEMTIME& time);
std::wstring FormatDurationHms(int seconds);
std::wstring FormatDurationHm(int seconds);
std::wstring FormatNumber(double value, int decimals, bool grouping = false);
std::wstring FormatMoney(double value, int decimals, bool grouping = false);
std::wstring FormatPercent(double fraction);
