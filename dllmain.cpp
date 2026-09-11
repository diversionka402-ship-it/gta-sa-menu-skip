#include <windows.h>
#include <cstdint>

// Адреса подтверждены для GTA SA 1.0 US (совпадают в plugin-sdk и в modloader
// независимо друг от друга — см. game_sa/CMenuManager.cpp в DK22Pac/plugin-sdk
// и src/translator/gta3/sa/10us.hpp в thelink2012/modloader)
constexpr uintptr_t ADDR_FRONTEND_MENU_MANAGER = 0xBA6748;
constexpr uintptr_t ADDR_PROCESS_MENU_OPTIONS  = 0x576FE0;

// Смещения полей внутри CMenuManager (см. VALIDATE_OFFSET в исходном CMenuManager.h)
constexpr int OFFSET_CURRENT_MENU_ENTRY = 0x54;  // int  m_nCurrentMenuEntry
constexpr int OFFSET_CURRENT_MENU_PAGE  = 0x15D; // char m_nCurrentMenuPage

constexpr char MENUPAGE_MAIN_MENU = 34;

// Оригинал: void CMenuManager::ProcessMenuOptions(char input, char* exit, char enter)
// Это невиртуальный member-функция, скомпилированная как __thiscall (this в ECX)
using ProcessMenuOptions_t = void(__thiscall*)(void* thisPtr, char input, char* exitFlag, char enter);

static DWORD WINAPI MainThread(LPVOID) {
    // Ждём, пока игра проинициализирует меню
    Sleep(2000);

    BYTE* menuManager = reinterpret_cast<BYTE*>(ADDR_FRONTEND_MENU_MANAGER);

    // Переключаем страницу меню на главную и выбираем первый пункт
    *reinterpret_cast<char*>(menuManager + OFFSET_CURRENT_MENU_PAGE) = MENUPAGE_MAIN_MENU;
    *reinterpret_cast<int*>(menuManager + OFFSET_CURRENT_MENU_ENTRY) = 0;

    // Симулируем нажатие Enter на выбранном пункте
    char exitFlag = 0;
    auto processMenuOptions = reinterpret_cast<ProcessMenuOptions_t>(ADDR_PROCESS_MENU_OPTIONS);
    processMenuOptions(menuManager, 0, &exitFlag, 1);

    return 0;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID) {
    if (reason == DLL_PROCESS_ATTACH) {
        // Не делаем тяжёлую работу (Sleep, обращения к памяти игры) прямо в DllMain —
        // это может залипнуть на loader lock. Выносим всё в отдельный поток.
        DisableThreadLibraryCalls(hModule);
        CreateThread(nullptr, 0, MainThread, nullptr, 0, nullptr);
    }
    return TRUE;
}
