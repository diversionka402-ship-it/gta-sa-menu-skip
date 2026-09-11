#include <windows.h>
#include <cstdint>

// Адреса подтверждены для GTA SA 1.0 US (plugin-sdk и modloader независимо совпадают)
constexpr uintptr_t ADDR_FRONTEND_MENU_MANAGER = 0xBA6748;
constexpr uintptr_t ADDR_PROCESS_MENU_OPTIONS  = 0x576FE0;

// Смещения полей внутри CMenuManager (см. VALIDATE_OFFSET в исходном CMenuManager.h)
constexpr int OFFSET_CURRENT_MENU_ENTRY = 0x54;  // int  m_nCurrentMenuEntry
constexpr int OFFSET_MENU_ACTIVE        = 0x5C;  // bool m_bMenuActive
constexpr int OFFSET_CURRENT_MENU_PAGE  = 0x15D; // char m_nCurrentMenuPage

constexpr char MENUPAGE_MAIN_MENU = 34;

using ProcessMenuOptions_t = void(__thiscall*)(void* thisPtr, char input, char* exitFlag, char enter);

static void PressEnterOnFirstEntry(BYTE* mm, ProcessMenuOptions_t processMenuOptions) {
    *reinterpret_cast<int*>(mm + OFFSET_CURRENT_MENU_ENTRY) = 0;
    char exitFlag = 0;
    processMenuOptions(mm, 0, &exitFlag, 1);
}

static DWORD WINAPI MainThread(LPVOID) {
    BYTE* mm = reinterpret_cast<BYTE*>(ADDR_FRONTEND_MENU_MANAGER);
    auto processMenuOptions = reinterpret_cast<ProcessMenuOptions_t>(ADDR_PROCESS_MENU_OPTIONS);

    // Ждём, пока меню РЕАЛЬНО не станет активным (после заставок/логотипов)
    for (int i = 0; i < 300; ++i) {
        if (*reinterpret_cast<bool*>(mm + OFFSET_MENU_ACTIVE)) {
            break;
        }
        Sleep(100);
    }
    Sleep(300); // буфер, чтобы меню успело отрисовать первый кадр

    // Первый шаг: принудительно ставим главную страницу и жмём Enter на первом пункте
    // (Main Menu -> Start Game)
    *reinterpret_cast<char*>(mm + OFFSET_CURRENT_MENU_PAGE) = MENUPAGE_MAIN_MENU;
    PressEnterOnFirstEntry(mm, processMenuOptions);

    // Дальше идёт цепочка экранов: Game -> New Game -> "Are you sure?" -> ... -> геймплей.
    // Просто продолжаем жать Enter на первом пункте каждого следующего экрана,
    // пока меню не закроется целиком (m_bMenuActive == false = игра реально началась).
    for (int step = 0; step < 20; ++step) {
        Sleep(500);

        bool menuActive = *reinterpret_cast<bool*>(mm + OFFSET_MENU_ACTIVE);
        if (!menuActive) {
            break; // меню закрылось — игра началась, дальше делать нечего
        }

        PressEnterOnFirstEntry(mm, processMenuOptions);
    }

    return 0;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID) {
    if (reason == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(hModule);
        CreateThread(nullptr, 0, MainThread, nullptr, 0, nullptr);
    }
    return TRUE;
}
