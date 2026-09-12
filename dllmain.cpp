#include <windows.h>
#include <cstdint>
#include <MinHook.h>

// Адреса подтверждены для GTA SA 1.0 US (plugin-sdk и modloader независимо совпадают)
constexpr uintptr_t ADDR_FRONTEND_MENU_MANAGER = 0xBA6748;
constexpr uintptr_t ADDR_PROCESS_MENU_OPTIONS  = 0x576FE0;
constexpr uintptr_t ADDR_MENU_PROCESS          = 0x57B440; // CMenuManager::Process()

// Смещения полей внутри CMenuManager
constexpr int OFFSET_CURRENT_MENU_ENTRY = 0x54;  // int  m_nCurrentMenuEntry
constexpr int OFFSET_MENU_ACTIVE        = 0x5C;  // bool m_bMenuActive
constexpr int OFFSET_CURRENT_MENU_PAGE  = 0x15D; // char m_nCurrentMenuPage

constexpr char MENUPAGE_MAIN_MENU = 34;

using ProcessMenuOptions_t = void(__thiscall*)(void* thisPtr, char input, char* exitFlag, char enter);

// Хак для хука __thiscall-метода: __fastcall кладёт первый аргумент в ECX,
// точно так же, как __thiscall кладёт this. EDX не используется.
using Process_t = int(__fastcall*)(void* thisPtr, void* /*unused edx*/);

static Process_t oProcess = nullptr;
static bool g_skipDone = false; // чтобы сработать РОВНО один раз за весь процесс

static void PressEnterOnFirstEntry(BYTE* mm, ProcessMenuOptions_t processMenuOptions) {
    *reinterpret_cast<int*>(mm + OFFSET_CURRENT_MENU_ENTRY) = 0;
    char exitFlag = 0;
    processMenuOptions(mm, 0, &exitFlag, 1);
}

static int __fastcall hkProcess(void* thisPtr, void* /*edx*/) {
    if (!g_skipDone) {
        BYTE* mm = reinterpret_cast<BYTE*>(thisPtr);
        bool menuActive = *reinterpret_cast<bool*>(mm + OFFSET_MENU_ACTIVE);

        // Ждём естественного момента, когда игра САМА готова показать меню
        // (то есть уже прошли заставки и все нужные системы инициализированы) —
        // именно в этот момент, а не раньше.
        if (menuActive) {
            g_skipDone = true; // больше никогда не вмешиваемся

            auto processMenuOptions = reinterpret_cast<ProcessMenuOptions_t>(ADDR_PROCESS_MENU_OPTIONS);

            *reinterpret_cast<char*>(mm + OFFSET_CURRENT_MENU_PAGE) = MENUPAGE_MAIN_MENU;

            // Main Menu -> "Start Game" -> открывает подменю Game
            PressEnterOnFirstEntry(mm, processMenuOptions);
            // Game -> "New Game" -> запускает загрузку
            PressEnterOnFirstEntry(mm, processMenuOptions);
        }
    }

    // Отдаём управление оригинальной функции — этот же кадр обработается
    // уже с нашим подменённым состоянием (если сработало выше), либо
    // полностью прозрачно, как обычно (если уже отработали один раз).
    return oProcess(thisPtr, nullptr);
}

static void InstallHook() {
    if (MH_Initialize() != MH_OK) {
        return;
    }

    void* target = reinterpret_cast<void*>(ADDR_MENU_PROCESS);
    if (MH_CreateHook(target, reinterpret_cast<void*>(&hkProcess),
                       reinterpret_cast<void**>(&oProcess)) != MH_OK) {
        return;
    }

    MH_EnableHook(target);
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID) {
    if (reason == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(hModule);
        InstallHook(); // без потоков, без Sleep — хук сам сработает в нужный момент
    }
    return TRUE;
}
