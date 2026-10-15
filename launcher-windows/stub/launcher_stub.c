// "WWE2K14 Launcher.exe": starts launcher.ps1 (next to this exe) with no console window.
// Point a Steam shortcut or a desktop shortcut at this file.
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <wchar.h>

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE prev, PWSTR cmdline, int show) {
  (void)instance; (void)prev; (void)cmdline; (void)show;
  wchar_t dir[MAX_PATH];
  DWORD n = GetModuleFileNameW(NULL, dir, MAX_PATH);
  if (n == 0 || n >= MAX_PATH) return 1;
  wchar_t* slash = wcsrchr(dir, L'\\');
  if (slash) *slash = 0;

  wchar_t script[MAX_PATH + 32];
  swprintf(script, MAX_PATH + 32, L"%ls\\launcher.ps1", dir);
  if (GetFileAttributesW(script) == INVALID_FILE_ATTRIBUTES) {
    MessageBoxW(NULL, L"launcher.ps1 is missing next to this launcher.", L"WWE 2K14 Recomp", MB_ICONWARNING);
    return 1;
  }

  wchar_t command[4 * MAX_PATH];
  swprintf(command, 4 * MAX_PATH,
           L"powershell.exe -NoProfile -ExecutionPolicy Bypass -WindowStyle Hidden -File \"%ls\"", script);

  STARTUPINFOW si = {0};
  si.cb = sizeof(si);  // no STARTF_USESHOWWINDOW: it would also hide the launcher's own window
  PROCESS_INFORMATION pi = {0};
  if (!CreateProcessW(NULL, command, NULL, NULL, FALSE, CREATE_NO_WINDOW, NULL, dir, &si, &pi)) {
    MessageBoxW(NULL, L"Couldn't start Windows PowerShell.", L"WWE 2K14 Recomp", MB_ICONWARNING);
    return 1;
  }
  // Stay alive until the launcher closes, so Steam shows the game as running.
  WaitForSingleObject(pi.hProcess, INFINITE);
  DWORD code = 0;
  GetExitCodeProcess(pi.hProcess, &code);
  CloseHandle(pi.hThread);
  CloseHandle(pi.hProcess);
  return (int)code;
}
