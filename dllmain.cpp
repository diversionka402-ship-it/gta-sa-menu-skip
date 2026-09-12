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

using ProcessUserInput_t = void(__thiscall*)(void* thisPtr, char down, char up, char enter, char exit, char input);

static void PressEnter(BYTE* mm, ProcessUserInput_t processUserInput) {
    *reinterpret_cast<int*>(mm + OFFSET_CURRENT_MENU_ENTRY) = 0;
    processUserInput(mm, 0, 0, 1, 0, 0);
}

static DWORD WINAPI MainThread(LPVOID) {
    BYTE* mm = reinterpret_cast<BYTE*>(ADDR_FRONTEND_MENU_MANAGER);
    auto processUserInput = reinterpret_cast<ProcessUserInput_t>(ADDR_PROCESS_USER_INPUT);

    // УБРАЛ THREAD_PRIORITY_TIME_CRITICAL — именно это, вероятно, душило
    // главный поток игры, когда окно теряло фокус (например, при переключении
    // на Cheat Engine). Обычный приоритет вполне достаточен.

    // Ждём, пока игра САМА выставит m_bMenuActive = true.
    // Sleep(0) отдаёт квант времени другим потокам той же приоритетности,
    // не давая нашему циклу монопольно жрать ядро процессора.
    while (!*reinterpret_cast<volatile bool*>(mm + OFFSET_MENU_ACTIVE)) {
        Sleep(0);
    }

    *reinterpret_cast<char*>(mm + OFFSET_CURRENT_MENU_PAGE) = MENUPAGE_MAIN_MENU;

    // Main Menu -> "Start Game" -> открывает подменю Game
    PressEnter(mm, processUserInput);

    // Game -> "New Game" -> запускает загрузку
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
