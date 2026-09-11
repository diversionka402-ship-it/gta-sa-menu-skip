#include <windows.h>
#include <cstdint>

// Адреса подтверждены для GTA SA 1.0 US (plugin-sdk и modloader независимо совпадают)
constexpr uintptr_t ADDR_FRONTEND_MENU_MANAGER = 0xBA6748;
constexpr uintptr_t ADDR_PROCESS_MENU_OPTIONS  = 0x576FE0;

// Смещения полей внутри CMenuManager
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

    // Максимальный приоритет потока — чтобы среагировать раньше,
    // чем движок успеет отрисовать (Present) хотя бы один кадр меню.
    SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL);

    // БЕЗ Sleep вообще. Крутимся на максимальной скорости процессора,
    // проверяя флаг каждую итерацию, пока игра сама не выставит его в true.
    // Это окно (от установки флага до первого Present()) занимает миллисекунды —
    // busy-wait успевает среагировать внутри него, Sleep(даже 1мс) — не успевает
    // из-за задержки планировщика потоков Windows.
    while (!*reinterpret_cast<volatile bool*>(mm + OFFSET_MENU_ACTIVE)) {
        // намеренно пусто — activewait
    }

    *reinterpret_cast<char*>(mm + OFFSET_CURRENT_MENU_PAGE) = MENUPAGE_MAIN_MENU;

    // Шаг 1: Main Menu -> "Start Game" -> открывает подменю Game
    PressEnterOnFirstEntry(mm, processMenuOptions);

    // Шаг 2: сразу же, без единой паузы, Game -> "New Game" -> запускает загрузку
    PressEnterOnFirstEntry(mm, processMenuOptions);

    return 0;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID) {
    if (reason == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(hModule);
        CreateThread(nullptr, 0, MainThread, nullptr, 0, nullptr);
    }
    return TRUE;
}
