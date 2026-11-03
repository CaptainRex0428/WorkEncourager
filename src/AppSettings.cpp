#include "AppSettings.h"

#include <windows.h>
#include <shlobj.h>

#include <cmath>
#include <cwchar>
#include <cwctype>
#include <filesystem>
#include <iterator>
#include <string>
#include <system_error>

namespace
{
    std::wstring Trim(std::wstring_view text)
    {
        size_t begin = 0;
        while (begin < text.size() && std::iswspace(text[begin]))
        {
            ++begin;
        }

        size_t end = text.size();
        while (end > begin && std::iswspace(text[end - 1]))
        {
            --end;
        }

        return std::wstring(text.substr(begin, end - begin));
    }

    wchar_t NormalizeSeparator(wchar_t ch)
    {
        switch (ch)
        {
        case L'\uFF1A': // full-width colon
            return L':';
        case L'\uFF0E': // full-width full stop
        case L'\u3002': // ideographic full stop
            return L'.';
        default:
            return ch;
        }
    }

    std::wstring NormalizeNumberText(std::wstring_view text)
    {
        std::wstring normalized;
        normalized.reserve(text.size());
        for (wchar_t ch : text)
        {
            if (std::iswspace(ch))
            {
                continue;
            }
            normalized.push_back(NormalizeSeparator(ch));
        }
        return normalized;
    }

    bool ParseDoubleInvariant(const std::wstring& text, double& value)
    {
        if (text.empty())
        {
            return false;
        }

        wchar_t* end = nullptr;
        const double parsed = std::wcstod(text.c_str(), &end);
        if (end == text.c_str() || end == nullptr || *end != L'\0' || !std::isfinite(parsed))
        {
            return false;
        }

        value = parsed;
        return true;
    }

    bool SetError(std::wstring* reason, const wchar_t* message)
    {
        if (reason != nullptr)
        {
            *reason = message;
        }
        return false;
    }

    std::wstring ExecutableDirectory()
    {
        std::wstring buffer(MAX_PATH, L'\0');
        for (;;)
        {
            const DWORD length = GetModuleFileNameW(nullptr, buffer.data(), static_cast<DWORD>(buffer.size()));
            if (length == 0)
            {
                return L".";
            }
            if (length < buffer.size() - 1)
            {
                buffer.resize(length);
                break;
            }
            buffer.resize(buffer.size() * 2);
        }

        const size_t slash = buffer.find_last_of(L"\\/");
        return slash == std::wstring::npos ? L"." : buffer.substr(0, slash);
    }

    std::wstring BaseDirectory()
    {
        PWSTR localAppData = nullptr;
        if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_LocalAppData, KF_FLAG_CREATE, nullptr, &localAppData)) &&
            localAppData != nullptr)
        {
            std::wstring result(localAppData);
            CoTaskMemFree(localAppData);

            if (!result.empty())
            {
                result += L"\\WorkEncourager";
                return result;
            }
        }

        return ExecutableDirectory() + L"\\WorkEncouragerData";
    }

    std::wstring FormatDouble(double value, int decimals)
    {
        wchar_t buffer[64]{};
        swprintf_s(buffer, L"%.*f", decimals, value);
        return buffer;
    }
}

bool AppSettings::IsValid(std::wstring* reason) const
{
    if (startMinutes < 0 || startMinutes >= 24 * 60)
    {
        return SetError(reason, L"上班时间需要在 00:00 到 23:59 之间。");
    }
    if (endMinutes < 0 || endMinutes >= 24 * 60 || endMinutes <= startMinutes)
    {
        return SetError(reason, L"下班时间需要晚于上班时间，且不能跨过当天。");
    }
    if (!std::isfinite(monthlySalary) || monthlySalary <= 0.0 || monthlySalary > 10000000.0)
    {
        return SetError(reason, L"月薪需要大于 0，且不超过 10000000 元。");
    }
    return true;
}

bool ParseClockText(std::wstring_view text, int& minutes)
{
    std::wstring normalized = NormalizeNumberText(text);
    if (normalized.empty())
    {
        return false;
    }

    std::wstring hoursText;
    std::wstring minutesText;

    const size_t colon = normalized.find(L':');
    if (colon != std::wstring::npos)
    {
        if (normalized.find(L':', colon + 1) != std::wstring::npos)
        {
            return false;
        }
        hoursText = normalized.substr(0, colon);
        minutesText = normalized.substr(colon + 1);
    }
    else
    {
        for (wchar_t ch : normalized)
        {
            if (!std::iswdigit(ch))
            {
                return false;
            }
        }

        if (normalized.size() == 3)
        {
            hoursText = normalized.substr(0, 1);
            minutesText = normalized.substr(1);
        }
        else if (normalized.size() == 4)
        {
            hoursText = normalized.substr(0, 2);
            minutesText = normalized.substr(2);
        }
        else
        {
            return false;
        }
    }

    if (hoursText.empty() || hoursText.size() > 2 || minutesText.size() != 2)
    {
        return false;
    }
    for (wchar_t ch : hoursText)
    {
        if (!std::iswdigit(ch))
        {
            return false;
        }
    }
    for (wchar_t ch : minutesText)
    {
        if (!std::iswdigit(ch))
        {
            return false;
        }
    }

    const int hours = std::wcstol(hoursText.c_str(), nullptr, 10);
    const int mins = std::wcstol(minutesText.c_str(), nullptr, 10);
    if (hours < 0 || hours > 23 || mins < 0 || mins > 59)
    {
        return false;
    }

    minutes = hours * 60 + mins;
    return true;
}

std::wstring FormatClock(int minutes)
{
    if (minutes < 0)
    {
        minutes = 0;
    }
    wchar_t buffer[16]{};
    swprintf_s(buffer, L"%02d:%02d", minutes / 60, minutes % 60);
    return buffer;
}

bool ParseDecimalText(std::wstring_view text, double& value)
{
    const std::wstring normalized = NormalizeNumberText(text);
    return ParseDoubleInvariant(normalized, value);
}

namespace SettingsStore
{
    std::wstring FilePath()
    {
        return BaseDirectory() + L"\\settings.ini";
    }

    AppSettings Load()
    {
        AppSettings settings;
        const std::wstring path = FilePath();

        if (GetFileAttributesW(path.c_str()) == INVALID_FILE_ATTRIBUTES)
        {
            return settings;
        }

        settings.startMinutes = GetPrivateProfileIntW(L"work", L"startMinutes", settings.startMinutes, path.c_str());
        settings.endMinutes = GetPrivateProfileIntW(L"work", L"endMinutes", settings.endMinutes, path.c_str());

        wchar_t buffer[64]{};
        const bool hasMonthlySalary =
            GetPrivateProfileStringW(L"work", L"monthlySalary", L"", buffer,
                                     static_cast<DWORD>(std::size(buffer)), path.c_str()) > 0;
        if (hasMonthlySalary)
        {
            double parsed = 0.0;
            if (ParseDoubleInvariant(buffer, parsed))
            {
                settings.monthlySalary = parsed;
            }
        }
        else
        {
            // Version 1.0 stored an hourly rate. Preserve the same daily pay
            // when migrating an old settings.ini to the new monthly model.
            ZeroMemory(buffer, sizeof(buffer));
            if (GetPrivateProfileStringW(L"work", L"hourlyRate", L"", buffer,
                                         static_cast<DWORD>(std::size(buffer)), path.c_str()) > 0)
            {
                double hourly = 0.0;
                const int durationMinutes = settings.endMinutes - settings.startMinutes;
                if (ParseDoubleInvariant(buffer, hourly) && hourly > 0.0 && durationMinutes > 0)
                {
                    settings.monthlySalary =
                        hourly * (static_cast<double>(durationMinutes) / 60.0) * kAverageWorkdaysPerMonth;
                }
            }
        }

        if (!settings.IsValid())
        {
            return AppSettings{};
        }
        return settings;
    }

    bool Save(const AppSettings& settings, std::wstring* error)
    {
        if (!settings.IsValid(error))
        {
            return false;
        }

        const std::wstring path = FilePath();
        std::error_code directoryError;
        std::filesystem::create_directories(std::filesystem::path(path).parent_path(), directoryError);
        if (directoryError)
        {
            if (error != nullptr)
            {
                *error = L"无法创建设置目录：" + path;
            }
            return false;
        }

        const std::wstring start = std::to_wstring(settings.startMinutes);
        const std::wstring end = std::to_wstring(settings.endMinutes);
        const std::wstring salary = FormatDouble(settings.monthlySalary, 2);

        const bool written =
            WritePrivateProfileStringW(L"work", L"startMinutes", start.c_str(), path.c_str()) != FALSE &&
            WritePrivateProfileStringW(L"work", L"endMinutes", end.c_str(), path.c_str()) != FALSE &&
            WritePrivateProfileStringW(L"work", L"monthlySalary", salary.c_str(), path.c_str()) != FALSE;

        if (!written)
        {
            if (error != nullptr)
            {
                *error = L"无法写入设置文件：" + path;
            }
            return false;
        }

        // Clean up keys written by version 1.0. Failure to delete an obsolete
        // key does not invalidate the settings that were just saved.
        WritePrivateProfileStringW(L"work", L"hourlyRate", nullptr, path.c_str());
        WritePrivateProfileStringW(L"work", L"rewardPrice", nullptr, path.c_str());

        return true;
    }
}
