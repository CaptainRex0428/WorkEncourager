#pragma once

#include <string>
#include <string_view>

constexpr int kDefaultStartMinutes = 10 * 60 + 30; // 10:30
constexpr int kDefaultEndMinutes = 18 * 60;        // 18:00
constexpr double kAverageWorkdaysPerMonth = 21.0;
constexpr double kDefaultMonthlySalary = 2715.30;

struct AppSettings
{
    int startMinutes = kDefaultStartMinutes;
    int endMinutes = kDefaultEndMinutes;
    double monthlySalary = kDefaultMonthlySalary;

    int DurationMinutes() const noexcept { return endMinutes - startMinutes; }

    // The salary model requested by the user: one month is averaged to 21
    // working days. The daily figure is then divided by this shift's hours.
    double FullDayPay() const noexcept { return monthlySalary / kAverageWorkdaysPerMonth; }
    double HourlyRate() const noexcept
    {
        const double shiftHours = static_cast<double>(DurationMinutes()) / 60.0;
        return shiftHours > 0.0 ? FullDayPay() / shiftHours : 0.0;
    }

    // Returns true when every field is inside a usable range. On failure,
    // `reason` receives a short, user-facing explanation.
    bool IsValid(std::wstring* reason = nullptr) const;
};

namespace SettingsStore
{
    // %LOCALAPPDATA%\WorkEncourager\settings.ini, with an executable-relative
    // fallback for unusual environments.
    std::wstring FilePath();
    AppSettings Load();
    bool Save(const AppSettings& settings, std::wstring* error = nullptr);
}

bool ParseClockText(std::wstring_view text, int& minutes);
std::wstring FormatClock(int minutes);
bool ParseDecimalText(std::wstring_view text, double& value);
