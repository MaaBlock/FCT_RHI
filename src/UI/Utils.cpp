#include "Utils.h"
#include <atomic>
#include <iostream>

namespace FCT
{
    static std::atomic<bool> g_headlessDialogs{false};

    void SetHeadlessDialogMode(bool headless) {
        g_headlessDialogs.store(headless, std::memory_order_release);
    }

    bool IsHeadlessDialogMode() {
        return g_headlessDialogs.load(std::memory_order_acquire);
    }

#ifdef _WIN32
    std::wstring stringToWString(const std::string& str) {
        if (str.empty()) return std::wstring();

        int size_needed = MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), NULL, 0);
        std::wstring wstrTo(size_needed, 0);
        MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), &wstrTo[0], size_needed);
        return wstrTo;
    }
#endif

    void ShowErrorDialog(const std::string& title, const std::string& message) {
        if (g_headlessDialogs.load(std::memory_order_acquire)) {
            std::cerr << "[ERROR] " << title << ": " << message << std::endl;
            return;
        }
#ifdef _WIN32
        std::wstring wTitle = stringToWString(title);
        std::wstring wMessage = stringToWString(message);
        MessageBoxW(nullptr, wMessage.c_str(), wTitle.c_str(), MB_OK | MB_ICONERROR);
#else
        std::cerr << "[ERROR] " << title << ": " << message << std::endl;
#endif
    }

    void ShowWarningDialog(const std::string& title, const std::string& message) {
        if (g_headlessDialogs.load(std::memory_order_acquire)) {
            std::cerr << "[WARNING] " << title << ": " << message << std::endl;
            return;
        }
#ifdef _WIN32
        std::wstring wTitle = stringToWString(title);
        std::wstring wMessage = stringToWString(message);
        MessageBoxW(nullptr, wMessage.c_str(), wTitle.c_str(), MB_OK | MB_ICONWARNING);
#else
        std::cerr << "[WARNING] " << title << ": " << message << std::endl;
#endif
    }

    bool ShowConfirmDialog(const std::string& title, const std::string& message) {
        if (g_headlessDialogs.load(std::memory_order_acquire)) {
            std::cout << "[CONFIRM] (Headless Auto-Yes) " << title << ": " << message << std::endl;
            return true;
        }
#ifdef _WIN32
        std::wstring wTitle = stringToWString(title);
        std::wstring wMessage = stringToWString(message);
        int result = MessageBoxW(nullptr, wMessage.c_str(), wTitle.c_str(), MB_YESNO | MB_ICONQUESTION);
        return result == IDYES;
#else
        std::cout << "[CONFIRM] " << title << ": " << message << " (y/n): ";
        char response;
        std::cin >> response;
        return response == 'y' || response == 'Y';
#endif
    }

    void ShowInfoDialog(const std::string& title, const std::string& message) {
        if (g_headlessDialogs.load(std::memory_order_acquire)) {
            std::cout << "[INFO] " << title << ": " << message << std::endl;
            return;
        }
#ifdef _WIN32
        std::wstring wTitle = stringToWString(title);
        std::wstring wMessage = stringToWString(message);
        MessageBoxW(nullptr, wMessage.c_str(), wTitle.c_str(), MB_OK | MB_ICONINFORMATION);
#else
        std::cout << "[INFO] " << title << ": " << message << std::endl;
#endif
    }
}