#include <windows.h>
#include <cstdint>

// Адреса подтверждены для GTA SA 1.0 US (plugin-sdk и modloader независимо совпадают)
constexpr uintptr_t ADDR_FRONTEND_MENU_MANAGER = 0xBA6748;
constexpr uintptr_t ADDR_PROCESS_MENU_OPTIONS  = 0x576FE0;

// Смещения полей внутри CMenuManager (см. VALIDATE_OFFSET в исходном CMenuManager.h)
constexpr int OFFSET_SHUTDOWN_REQUESTED = 0x32;  // bool m_bShutDownFrontEndRequested
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

    SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL);

    // Ждём, пока игра САМА выставит m_bMenuActive = true — это естественный
    // момент, когда все нужные системы (стриминг, ресурсы) уже готовы.
    while (!*reinterpret_cast<volatile bool*>(mm + OFFSET_MENU_ACTIVE)) {
        // busy-wait, намеренно без Sleep
    }

    *reinterpret_cast<char*>(mm + OFFSET_CURRENT_MENU_PAGE) = MENUPAGE_MAIN_MENU;

    // Main Menu -> "Start Game" -> открывает подменю Game
    PressEnterOnFirstEntry(mm, processMenuOptions);

    // Game -> "New Game" -> запускает загрузку
    PressEnterOnFirstEntry(mm, processMenuOptions);

    // ВАЖНО: именно это игра обычно делает сама после выбора New Game —
    // просит фронтенд корректно закрыться. Без этого шага меню считает,
    // что должно оставаться "полуактивным", и после загрузки всплывает
    // призрачная пауза с чёрным экраном.
    *reinterpret_cast<bool*>(mm + OFFSET_SHUTDOWN_REQUESTED) = true;

    return 0;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID) {
    if (reason == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(hModule);
        CreateThread(nullptr, 0, MainThread, nullptr, 0, nullptr);
    }
    return TRUE;
}
