#include "pch.h"
#include <filesystem>
#include "dllmain.h"
#include "PluginLoadHook.h"
#include "ThreadManager.hpp"

static HINSTANCE hOriginalVersion = NULL;
static HMODULE gameModule;

// Function pointers for original version.dll functions
static FARPROC pGetFileVersionInfoSizeW = NULL;
static FARPROC pGetFileVersionInfoW = NULL;
static FARPROC pVerQueryValueW = NULL;
static FARPROC pGetFileVersionInfoSizeA = NULL;
static FARPROC pGetFileVersionInfoA = NULL;
static FARPROC pVerQueryValueA = NULL;

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID lpReserved) {
    if (reason == DLL_PROCESS_ATTACH) {
        // Store the module handle
        gameModule = hModule;

        std::string cmdArgs = GetCommandLineA();
        if (cmdArgs.find("--debug") != std::string::npos) {
            AllocConsole();
            InitializeConsole();
        }

        LogString(L"Version proxy DLL loaded...\n");

        // Load the original version.dll from system directory
        wchar_t systemPath[MAX_PATH];
        GetSystemDirectoryW(systemPath, MAX_PATH);
        std::wstring versionPath = std::wstring(systemPath) + L"\\version.dll";

        hOriginalVersion = LoadLibraryW(versionPath.c_str());
        if (!hOriginalVersion) {
            LogString(L"Failed to load original version.dll: " + GetLastErrorAsString() + L"\n");
            return FALSE;
        }

        LogString(L"Original version.dll loaded successfully\n");

        // Get function pointers
        pGetFileVersionInfoSizeW = GetProcAddress(hOriginalVersion, "GetFileVersionInfoSizeW");
        pGetFileVersionInfoW = GetProcAddress(hOriginalVersion, "GetFileVersionInfoW");
        pVerQueryValueW = GetProcAddress(hOriginalVersion, "VerQueryValueW");
        pGetFileVersionInfoSizeA = GetProcAddress(hOriginalVersion, "GetFileVersionInfoSizeA");
        pGetFileVersionInfoA = GetProcAddress(hOriginalVersion, "GetFileVersionInfoA");
        pVerQueryValueA = GetProcAddress(hOriginalVersion, "VerQueryValueA");

        if (!pGetFileVersionInfoSizeW) {
            LogString(L"Failed to get function pointers from version.dll\n");
        }

        // Disable thread calls to reduce overhead
        DisableThreadLibraryCalls(hModule);

        // Start our plugin system in a separate thread
        CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)executionThread, NULL, 0, NULL);
    }
    else if (reason == DLL_PROCESS_DETACH) {
        if (hOriginalVersion) {
            FreeLibrary(hOriginalVersion);
        }
    }
    return TRUE;
}

int executionThread() {
    InitializePluginHooks(gameModule);
    return 0;
}

// Export the functions without redefining them
// We'll use the .def file to handle the exports

// Wrapper functions that forward to the original version.dll
extern "C" {
    __declspec(dllexport) DWORD _GetFileVersionInfoSizeW(LPCWSTR lptstrFilename, LPDWORD lpdwHandle) {
        if (pGetFileVersionInfoSizeW) {
            return ((DWORD(WINAPI*)(LPCWSTR, LPDWORD))pGetFileVersionInfoSizeW)(lptstrFilename, lpdwHandle);
        }
        return 0;
    }

    __declspec(dllexport) BOOL _GetFileVersionInfoW(LPCWSTR lptstrFilename, DWORD dwHandle, DWORD dwLen, LPVOID lpData) {
        if (pGetFileVersionInfoW) {
            return ((BOOL(WINAPI*)(LPCWSTR, DWORD, DWORD, LPVOID))pGetFileVersionInfoW)(lptstrFilename, dwHandle, dwLen, lpData);
        }
        return FALSE;
    }

    __declspec(dllexport) BOOL _VerQueryValueW(LPCVOID pBlock, LPCWSTR lpSubBlock, LPVOID* lplpBuffer, PUINT puLen) {
        if (pVerQueryValueW) {
            return ((BOOL(WINAPI*)(LPCVOID, LPCWSTR, LPVOID*, PUINT))pVerQueryValueW)(pBlock, lpSubBlock, lplpBuffer, puLen);
        }
        return FALSE;
    }

    __declspec(dllexport) DWORD _GetFileVersionInfoSizeA(LPCSTR lptstrFilename, LPDWORD lpdwHandle) {
        if (pGetFileVersionInfoSizeA) {
            return ((DWORD(WINAPI*)(LPCSTR, LPDWORD))pGetFileVersionInfoSizeA)(lptstrFilename, lpdwHandle);
        }
        return 0;
    }

    __declspec(dllexport) BOOL _GetFileVersionInfoA(LPCSTR lptstrFilename, DWORD dwHandle, DWORD dwLen, LPVOID lpData) {
        if (pGetFileVersionInfoA) {
            return ((BOOL(WINAPI*)(LPCSTR, DWORD, DWORD, LPVOID))pGetFileVersionInfoA)(lptstrFilename, dwHandle, dwLen, lpData);
        }
        return FALSE;
    }

    __declspec(dllexport) BOOL _VerQueryValueA(LPCVOID pBlock, LPCSTR lpSubBlock, LPVOID* lplpBuffer, PUINT puLen) {
        if (pVerQueryValueA) {
            return ((BOOL(WINAPI*)(LPCVOID, LPCSTR, LPVOID*, PUINT))pVerQueryValueA)(pBlock, lpSubBlock, lplpBuffer, puLen);
        }
        return FALSE;
    }
}