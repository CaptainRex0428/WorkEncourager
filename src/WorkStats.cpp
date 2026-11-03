#include "WorkStats.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cwchar>
#include <string>

namespace
{
    constexpr int kSecondsPerDay = 24 * 60 * 60;

    int SecondsOfDay(const SYSTEMTIME& time)
    {
        return static_cast<int>(time.wHour) * 3600 + static_cast<int>(time.wMinute) * 60 + static_cast<int>(time.wSecond);
    }

    double SecondsOfDayExact(const SYSTEMTIME& time)
    {
        return static_cast<double>(SecondsOfDay(time)) + static_cast<double>(time.wMilliseconds) / 1000.0;
    }

    int CountWeekdaysBefore(const SYSTEMTIME& time)
    {
        using namespace std::chrono;

        const year_month_day today{ year{ static_cast<int>(time.wYear) },
                                    month{ static_cast<unsigned>(time.wMonth) },
                                    day{ static_cast<unsigned>(time.wDay) } };
        if (!today.ok())
        {
            return 0;
        }

        int count = 0;
        for (unsigned dayNumber = 1; dayNumber < static_cast<unsigned>(time.wDay); ++dayNumber)
        {
            const year_month_day date{ today.year(), today.month(), std::chrono::day{ dayNumber } };
            const weekday wd{ sys_days{ date } };
            if (wd.iso_encoding() <= 5)
            {
                ++count;
            }
        }
        return count;
    }

    std::wstring GroupDigits(const std::wstring& digits)
    {
        if (digits.size() <= 3)
        {
            return digits;
        }

        std::wstring grouped;
        grouped.reserve(digits.size() + digits.size() / 3);
        const size_t firstGroup = digits.size() % 3 == 0 ? 3 : digits.size() % 3;
        grouped.append(digits, 0, firstGroup);
        for (size_t index = firstGroup; index < digits.size(); index += 3)
        {
            grouped.push_back(L',');
            grouped.append(digits, index, 3);
        }
        return grouped;
    }
}

const wchar_t* WeekdayName(int dayOfWeek)
{
    static constexpr std::array<const wchar_t*, 7> kNames{
        L"星期日", L"星期一", L"星期二", L"星期三", L"星期四", L"星期五", L"星期六"
    };
    if (dayOfWeek < 0 || dayOfWeek >= static_cast<int>(kNames.size()))
    {
        return L"星期?";
    }
    return kNames[static_cast<size_t>(dayOfWeek)];
}

const wchar_t* PhaseLabel(WorkPhase phase)
{
    switch (phase)
    {
    case WorkPhase::Weekend:
        return L"周末愉快";
    case WorkPhase::BeforeWork:
        return L"还未上班";
    case WorkPhase::Working:
        return L"上班中";
    case WorkPhase::Paused:
        return L"休息中";
    case WorkPhase::Finished:
        return L"下班啦";
    }
    return L"上班中";
}

const std::array<SmallGoal, 5>& BuiltInSmallGoals()
{
    static constexpr std::array<SmallGoal, 5> kGoals{{
        { L"咖啡", L"一杯咖啡", 19.00 },
        { L"午饭", L"一顿午饭", 35.00 },
        { L"T恤", L"一件 T 恤", 129.00 },
        { L"跑鞋", L"一双跑鞋", 399.00 },
        { L"耳机", L"一副耳机", 799.00 },
    }};
    return kGoals;
}

std::wstring FormatDateLine(const SYSTEMTIME& time)
{
    wchar_t buffer[64]{};
    swprintf_s(buffer, L"%04d年%d月%d日  %s", static_cast<int>(time.wYear), static_cast<int>(time.wMonth),
               static_cast<int>(time.wDay), WeekdayName(time.wDayOfWeek));
    return buffer;
}

std::wstring FormatClockLine(const SYSTEMTIME& time)
{
    wchar_t buffer[16]{};
    swprintf_s(buffer, L"%02d:%02d:%02d", static_cast<int>(time.wHour), static_cast<int>(time.wMinute),
               static_cast<int>(time.wSecond));
    return buffer;
}

std::wstring FormatDurationHms(int seconds)
{
    if (seconds < 0)
    {
        seconds = 0;
    }

    const int hours = seconds / 3600;
    const int minutes = (seconds % 3600) / 60;
    const int secs = seconds % 60;

    wchar_t buffer[64]{};
    if (hours > 0)
    {
        swprintf_s(buffer, L"%d 小时 %02d 分 %02d 秒", hours, minutes, secs);
    }
    else if (minutes > 0)
    {
        swprintf_s(buffer, L"%d 分 %02d 秒", minutes, secs);
    }
    else
    {
        swprintf_s(buffer, L"%d 秒", secs);
    }
    return buffer;
}

std::wstring FormatDurationHm(int seconds)
{
    if (seconds <= 0)
    {
        return L"不到 1 分钟";
    }

    const int totalMinutes = (seconds + 59) / 60;
    const int hours = totalMinutes / 60;
    const int minutes = totalMinutes % 60;

    wchar_t buffer[64]{};
    if (hours > 0 && minutes > 0)
    {
        swprintf_s(buffer, L"%d 小时 %d 分", hours, minutes);
    }
    else if (hours > 0)
    {
        swprintf_s(buffer, L"%d 小时", hours);
    }
    else
    {
        swprintf_s(buffer, L"%d 分钟", minutes);
    }
    return buffer;
}

std::wstring FormatNumber(double value, int decimals, bool grouping)
{
    if (!std::isfinite(value))
    {
        value = 0.0;
    }
    if (value < 0.0 && value > -0.0000001)
    {
        value = 0.0;
    }

    wchar_t buffer[96]{};
    swprintf_s(buffer, L"%.*f", std::clamp(decimals, 0, 6), value);

    std::wstring text(buffer);
    if (!grouping)
    {
        return text;
    }

    const size_t dot = text.find(L'.');
    const std::wstring integerPart = dot == std::wstring::npos ? text : text.substr(0, dot);
    const std::wstring fractionPart = dot == std::wstring::npos ? L"" : text.substr(dot);

    const bool negative = !integerPart.empty() && integerPart.front() == L'-';
    const std::wstring unsignedPart = negative ? integerPart.substr(1) : integerPart;
    std::wstring result = negative ? L"-" : L"";
    result += GroupDigits(unsignedPart);
    result += fractionPart;
    return result;
}

std::wstring FormatMoney(double value, int decimals, bool grouping)
{
    return L"¥" + FormatNumber(value, decimals, grouping);
}

std::wstring FormatPercent(double fraction)
{
    if (!std::isfinite(fraction))
    {
        fraction = 0.0;
    }
    const double percentage = std::clamp(fraction, 0.0, 1.0) * 100.0;
    return FormatNumber(percentage, 1) + L"%";
}

WorkClock::WorkClock(const AppSettings& settings)
    : settings_(settings)
{
}

void WorkClock::SetSettings(const AppSettings& settings)
{
    settings_ = settings;
}

bool WorkClock::TogglePause()
{
    SYSTEMTIME now{};
    GetLocalTime(&now);

    const bool isWeekday = now.wDayOfWeek >= 1 && now.wDayOfWeek <= 5;
    if (!isWeekday)
    {
        return false;
    }

    const int secondsNow = SecondsOfDay(now);
    const int startSeconds = settings_.startMinutes * 60;
    const int endSeconds = settings_.endMinutes * 60;
    if (secondsNow < startSeconds || secondsNow >= endSeconds)
    {
        return false;
    }

    if (paused_)
    {
        const auto pausedFor = std::chrono::steady_clock::now() - pauseStarted_;
        pausedSeconds_ += std::chrono::duration<double>(pausedFor).count();
        paused_ = false;
    }
    else
    {
        paused_ = true;
        pauseStarted_ = std::chrono::steady_clock::now();
    }
    return true;
}

double WorkClock::TotalPausedSeconds() const
{
    double total = pausedSeconds_;
    if (paused_)
    {
        const auto pausedFor = std::chrono::steady_clock::now() - pauseStarted_;
        total += std::chrono::duration<double>(pausedFor).count();
    }
    return total;
}

WorkSnapshot WorkClock::Snapshot() const
{
    WorkSnapshot result;
    GetLocalTime(&result.now);

    result.isWeekday = result.now.wDayOfWeek >= 1 && result.now.wDayOfWeek <= 5;
    result.shiftSeconds = settings_.DurationMinutes() * 60;
    result.shiftHours = settings_.DurationMinutes() / 60.0;
    result.hourlyRate = settings_.HourlyRate();
    result.perSecond = result.hourlyRate / 3600.0;
    result.fullDayPay = settings_.FullDayPay();
    result.workdaysBefore = CountWeekdaysBefore(result.now);

    const int secondsNow = SecondsOfDay(result.now);
    const double secondsNowExact = SecondsOfDayExact(result.now);
    const int startSeconds = settings_.startMinutes * 60;
    const int endSeconds = settings_.endMinutes * 60;

    if (!result.isWeekday)
    {
        result.phase = WorkPhase::Weekend;
    }
    else if (secondsNow < startSeconds)
    {
        result.phase = WorkPhase::BeforeWork;
        result.untilStartSeconds = std::max(0, static_cast<int>(std::ceil(startSeconds - secondsNowExact)));
    }
    else if (secondsNow >= endSeconds)
    {
        result.phase = WorkPhase::Finished;
    }
    else
    {
        result.phase = paused_ ? WorkPhase::Paused : WorkPhase::Working;
    }

    const double wallElapsed = std::clamp(secondsNowExact - static_cast<double>(startSeconds), 0.0,
                                          static_cast<double>(result.shiftSeconds));
    const double activeElapsed = std::clamp(wallElapsed - TotalPausedSeconds(), 0.0,
                                            static_cast<double>(result.shiftSeconds));

    result.elapsedSeconds = static_cast<int>(std::floor(activeElapsed));
    result.workedHours = activeElapsed / 3600.0;
    result.progress = result.shiftSeconds > 0 ? activeElapsed / static_cast<double>(result.shiftSeconds) : 0.0;
    result.earnedToday = result.isWeekday ? result.perSecond * activeElapsed : 0.0;

    if (result.phase == WorkPhase::Working || result.phase == WorkPhase::Paused)
    {
        result.remainingSeconds = std::max(0, static_cast<int>(std::ceil(endSeconds - secondsNowExact)));
    }
    else if (result.phase == WorkPhase::BeforeWork)
    {
        result.remainingSeconds = 0;
    }

    result.monthlyEarned = result.workdaysBefore * result.fullDayPay + result.earnedToday;

    return result;
}
