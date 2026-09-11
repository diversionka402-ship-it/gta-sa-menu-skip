#include <windows.h>
#include "CMenuManager.h"

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID) {
    if (reason == DLL_PROCESS_ATTACH) {
        // Ждём немного, чтобы игра успела проинициализировать меню
        Sleep(2000);

        // Устанавливаем текущую страницу меню на главное меню
        FrontEndMenuManager.m_nCurrentMenuPage = 34; // MENUPAGE_MAIN_MENU
        FrontEndMenuManager.m_nCurrentMenuEntry = 0; // первый пункт (обычно "New Game")

        char exitFlag = 0;
        // Симулируем нажатие Enter на пункте меню
        FrontEndMenuManager.ProcessMenuOptions(0, &exitFlag, 1);
    }
    return TRUE;
}
