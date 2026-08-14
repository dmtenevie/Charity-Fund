/* Run.exe — double-click launcher for the built app.
 * GUI subsystem (no console window). Finds charity-fund/build/charity_fund.exe
 * relative to its own location, so it works no matter where the project
 * folder is on disk, and launches it.
 *
 * Also makes sure the local PostgreSQL cluster (initialized per the README
 * at C:\pgdata) is actually running first: it isn't registered as a Windows
 * service, so after a reboot it stays stopped until someone starts it by
 * hand. Without this, every post-reboot launch would hit the app's "can't
 * connect to the database" warning for no reason the user can fix from
 * inside the app itself.
 */
#include <windows.h>
#include <string.h>

static int fileExists(const char* path) {
    return GetFileAttributesA(path) != INVALID_FILE_ATTRIBUTES;
}

/* Runs a command hidden (no flashing console window) and waits for it to
 * finish, returning its exit code, or -1 if it couldn't even be started. */
static int runHidden(const char* cmdLine) {
    STARTUPINFOA si;
    PROCESS_INFORMATION pi;
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;
    ZeroMemory(&pi, sizeof(pi));

    char mutableCmd[MAX_PATH * 2];
    lstrcpynA(mutableCmd, cmdLine, sizeof(mutableCmd));

    if (!CreateProcessA(NULL, mutableCmd, NULL, NULL, FALSE,
                         CREATE_NO_WINDOW, NULL, NULL, &si, &pi)) {
        return -1;
    }

    WaitForSingleObject(pi.hProcess, 15000);
    DWORD exitCode = 0;
    GetExitCodeProcess(pi.hProcess, &exitCode);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    return (int)exitCode;
}

/* Best-effort: if we can find pg_ctl.exe (MSYS2, same search order as
 * build.ps1) and the C:\pgdata cluster from the README setup steps, make
 * sure the server is up. Silently does nothing if either is missing —
 * the app's own startup warning dialog still covers that case. */
static void ensurePostgresRunning(void) {
    const char* pgdata = "C:\\pgdata";
    if (!fileExists(pgdata)) return;

    const char* candidates[] = {
        NULL, /* filled in below with %USERPROFILE%\msys64 */
        "C:\\msys64",
        "C:\\tools\\msys64",
    };
    char userProfileCandidate[MAX_PATH];
    char userProfile[MAX_PATH - 16];
    DWORD len = GetEnvironmentVariableA("USERPROFILE", userProfile, sizeof(userProfile));
    if (len > 0 && len < sizeof(userProfile)) {
        wsprintfA(userProfileCandidate, "%s\\msys64", userProfile);
        candidates[0] = userProfileCandidate;
    }

    char pgCtlPath[MAX_PATH];
    int found = 0;
    for (int i = 0; i < 3 && !found; ++i) {
        if (!candidates[i]) continue;
        wsprintfA(pgCtlPath, "%s\\mingw64\\bin\\pg_ctl.exe", candidates[i]);
        if (fileExists(pgCtlPath)) found = 1;
    }
    if (!found) return;

    /* "status" exits 0 only when the server is already up. */
    char statusCmd[MAX_PATH * 2];
    wsprintfA(statusCmd, "\"%s\" -D \"%s\" status", pgCtlPath, pgdata);
    if (runHidden(statusCmd) == 0) return;

    char logPath[MAX_PATH];
    wsprintfA(logPath, "%s\\log", pgdata);
    char startCmd[MAX_PATH * 3];
    wsprintfA(startCmd, "\"%s\" -D \"%s\" -l \"%s\" -w start", pgCtlPath, pgdata, logPath);
    runHidden(startCmd);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    (void)hInstance; (void)hPrevInstance; (void)lpCmdLine; (void)nCmdShow;

    ensurePostgresRunning();

    char selfPath[MAX_PATH];
    GetModuleFileNameA(NULL, selfPath, MAX_PATH);

    char* lastSlash = strrchr(selfPath, '\\');
    if (lastSlash) *lastSlash = '\0';

    char targetPath[MAX_PATH];
    wsprintfA(targetPath, "%s\\charity-fund\\build\\charity_fund.exe", selfPath);

    char workDir[MAX_PATH];
    wsprintfA(workDir, "%s\\charity-fund\\build", selfPath);

    if (GetFileAttributesA(targetPath) == INVALID_FILE_ATTRIBUTES) {
        MessageBoxA(NULL,
            "charity_fund.exe not found.\n\n"
            "Build the project first: double-click Build.exe in the project root.",
            "Charity Fund", MB_OK | MB_ICONWARNING);
        return 1;
    }

    STARTUPINFOA si;
    PROCESS_INFORMATION pi;
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));

    if (CreateProcessA(targetPath, NULL, NULL, NULL, FALSE, 0, NULL, workDir, &si, &pi)) {
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
        return 0;
    }

    MessageBoxA(NULL, "Failed to start charity_fund.exe.", "Charity Fund", MB_OK | MB_ICONERROR);
    return 1;
}
