#include "pch.h"
#include "Injector.hpp"
#include <windows.h>
#include <Tlhelp32.h>

auto IsProcessAlive = [](std::int32_t pid) -> bool {
    HANDLE process = OpenProcess(SYNCHRONIZE, FALSE, pid);
    if (process)
    {
        DWORD ret = WaitForSingleObject(process, 0);
        CloseHandle(process);
        return ret == WAIT_TIMEOUT;
    }
    return false;
};

bool Injector::IsModuleLoaded(std::int32_t pid, const std::string& module_name) {
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, pid);
    if (snapshot == INVALID_HANDLE_VALUE) {
        return false;
    }

    MODULEENTRY32 moduleEntry;
    moduleEntry.dwSize = sizeof(MODULEENTRY32);

    // Convert std::string to std::wstring
    std::wstring wide_module_name(module_name.begin(), module_name.end());

    if (Module32First(snapshot, &moduleEntry)) {
        do {
            if (_wcsicmp(moduleEntry.szModule, wide_module_name.c_str()) == 0) {
                CloseHandle(snapshot);
                return true;
            }
        } while (Module32Next(snapshot, &moduleEntry));
    }

    CloseHandle(snapshot);
    return false;
}


bool Injector::Inject(std::string module_path, std::int32_t pid) noexcept {
    HMODULE hKernel32 = nullptr;
    void* RemoteAddress = nullptr;
    HANDLE ProcessHandle = nullptr, hThread = nullptr;
    LPTHREAD_START_ROUTINE LoadLibraryHandle = nullptr;

    if (!IsProcessAlive(pid)) {
        return false;
    }

    if (IsModuleLoaded(pid, module_path)) {
        return true;  // The module is already loaded; no need to inject again.
    }

    if ((ProcessHandle = OpenProcess(PROCESS_ALL_ACCESS, false, pid))) {
        if ((hKernel32 = GetModuleHandle(L"Kernel32.dll"))) {
            LoadLibraryHandle = reinterpret_cast<LPTHREAD_START_ROUTINE>(GetProcAddress(hKernel32, "LoadLibraryA"));
            if (LoadLibraryHandle) {
                RemoteAddress = VirtualAllocEx(ProcessHandle, nullptr, module_path.size() * sizeof(char), MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
                if (RemoteAddress) {
                    if (WriteProcessMemory(ProcessHandle, RemoteAddress, &module_path[0], module_path.size() * sizeof(char), nullptr)) {
                        hThread = CreateRemoteThread(ProcessHandle, nullptr, 0, LoadLibraryHandle, RemoteAddress, 0, nullptr);
                        if (hThread) {
                            WaitForSingleObject(hThread, 5000);
                        }
                    }
                }
            }
        }
    }

    // Cleanup
    if (hThread) {
        CloseHandle(hThread);
    }
    if (RemoteAddress) {
        VirtualFreeEx(ProcessHandle, RemoteAddress, module_path.size() * sizeof(char), MEM_RELEASE);
    }
    if (ProcessHandle) {
        CloseHandle(ProcessHandle);
    }

    return (hThread != nullptr);  // Return true if the thread was successfully created, false otherwise.
}
