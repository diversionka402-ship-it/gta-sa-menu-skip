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

static DWORD WINAPI MainThread(LPVOID) {
    BYTE* mm = reinterpret_cast<BYTE*>(ADDR_FRONTEND_MENU_MANAGER);

    // Вместо фиксированного Sleep — ждём, пока меню РЕАЛЬНО не станет активным
    // (после того как игра доиграет заставки/логотипы). Таймаут 30 сек на всякий случай.
    for (int i = 0; i < 300; ++i) {
        bool menuActive = *reinterpret_cast<bool*>(mm + OFFSET_MENU_ACTIVE);
        if (menuActive) {
            break;
        }
        Sleep(100);
    }

    // Небольшой буфер, чтобы меню успело отрисовать первый кадр
    Sleep(300);

    // Переключаем страницу меню на главную и выбираем первый пункт
    *reinterpret_cast<char*>(mm + OFFSET_CURRENT_MENU_PAGE) = MENUPAGE_MAIN_MENU;
    *reinterpret_cast<int*>(mm + OFFSET_CURRENT_MENU_ENTRY) = 0;

    // Симулируем нажатие Enter на выбранном пункте
    char exitFlag = 0;
    auto processMenuOptions = reinterpret_cast<ProcessMenuOptions_t>(ADDR_PROCESS_MENU_OPTIONS);
    processMenuOptions(mm, 0, &exitFlag, 1);

    return 0;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID) {
    if (reason == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(hModule);
        CreateThread(nullptr, 0, MainThread, nullptr, 0, nullptr);
    }
    return TRUE;
}
