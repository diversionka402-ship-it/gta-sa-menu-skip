#include <windows.h>
#include <cstdint>

// Адреса подтверждены для GTA SA 1.0 US (plugin-sdk и modloader независимо совпадают)
constexpr uintptr_t ADDR_FRONTEND_MENU_MANAGER = 0xBA6748;
constexpr uintptr_t ADDR_PROCESS_USER_INPUT    = 0x57B480; // CMenuManager::ProcessUserInput

// Смещения полей внутри CMenuManager
constexpr int OFFSET_CURRENT_MENU_ENTRY = 0x54;  // int  m_nCurrentMenuEntry
constexpr int OFFSET_MENU_ACTIVE        = 0x5C;  // bool m_bMenuActive
constexpr int OFFSET_CURRENT_MENU_PAGE  = 0x15D; // char m_nCurrentMenuPage

constexpr char MENUPAGE_MAIN_MENU = 34;

// void CMenuManager::ProcessUserInput(char down, char up, char enter, char exit, char input)
using ProcessUserInput_t = void(__thiscall*)(void* thisPtr, char down, char up, char enter, char exit, char input);

static void PressEnter(BYTE* mm, ProcessUserInput_t processUserInput) {
    *reinterpret_cast<int*>(mm + OFFSET_CURRENT_MENU_ENTRY) = 0;
    // down=0, up=0, enter=1, exit=0, input=0 — это именно то, что генерирует
    // реальный обработчик клавиатуры/мыши игры при нажатии Enter/клика
    processUserInput(mm, 0, 0, 1, 0, 0);
}

static DWORD WINAPI MainThread(LPVOID) {
    BYTE* mm = reinterpret_cast<BYTE*>(ADDR_FRONTEND_MENU_MANAGER);
    auto processUserInput = reinterpret_cast<ProcessUserInput_t>(ADDR_PROCESS_USER_INPUT);

    SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL);

    while (!*reinterpret_cast<volatile bool*>(mm + OFFSET_MENU_ACTIVE)) {
        // busy-wait, намеренно без Sleep
    }

    *reinterpret_cast<char*>(mm + OFFSET_CURRENT_MENU_PAGE) = MENUPAGE_MAIN_MENU;

    // Main Menu -> "Start Game" -> открывает подменю Game
    PressEnter(mm, processUserInput);

    // Game -> "New Game" -> запускает загрузку.
    // ProcessUserInput сама разберётся с последствиями (в т.ч. корректным
    // закрытием фронтенда), в отличие от голого ProcessMenuOptions.
    PressEnter(mm, processUserInput);

    return 0;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID) {
    if (reason == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(hModule);
        CreateThread(nullptr, 0, MainThread, nullptr, 0, nullptr);
    }
    return TRUE;
}
