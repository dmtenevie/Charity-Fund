/* Build.exe — double-click launcher that builds the project.
 * Console subsystem (shows build output). Runs charity-fund/build.ps1
 * with the PowerShell execution policy bypassed for this run only —
 * does not touch the system's script policy.
 */
#include <windows.h>
#include <string.h>
#include <stdio.h>

int main(void) {
    char selfPath[MAX_PATH];
    GetModuleFileNameA(NULL, selfPath, MAX_PATH);

    char* lastSlash = strrchr(selfPath, '\\');
    if (lastSlash) *lastSlash = '\0';

    char scriptPath[MAX_PATH];
    wsprintfA(scriptPath, "%s\\charity-fund\\build.ps1", selfPath);

    if (GetFileAttributesA(scriptPath) == INVALID_FILE_ATTRIBUTES) {
        fprintf(stderr, "Cannot find charity-fund\\build.ps1 next to this executable.\n");
        printf("\nPress Enter to exit...");
        getchar();
        return 1;
    }

    char cmdLine[MAX_PATH * 2];
    wsprintfA(cmdLine, "powershell -NoProfile -ExecutionPolicy Bypass -File \"%s\"", scriptPath);

    STARTUPINFOA si;
    PROCESS_INFORMATION pi;
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));

    BOOL ok = CreateProcessA(NULL, cmdLine, NULL, NULL, FALSE, 0, NULL, selfPath, &si, &pi);
    if (!ok) {
        fprintf(stderr, "Failed to launch PowerShell.\n");
        printf("\nPress Enter to exit...");
        getchar();
        return 1;
    }

    WaitForSingleObject(pi.hProcess, INFINITE);
    DWORD exitCode = 0;
    GetExitCodeProcess(pi.hProcess, &exitCode);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    printf("\nPress Enter to exit...");
    getchar();
    return (int)exitCode;
}
