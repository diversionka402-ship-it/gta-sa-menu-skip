#include <windows.h>

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID) {
    if (reason == DLL_PROCESS_ATTACH) {
        MessageBoxA(NULL, "ASI zagruzilsya uspeshno!", "Test", MB_OK);
    }
    return TRUE;
}
