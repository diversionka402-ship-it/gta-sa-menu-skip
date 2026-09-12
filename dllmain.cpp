#include <windows.h>
#include <cstdint>
#include <MinHook.h>

// Адреса подтверждены для GTA SA 1.0 US (plugin-sdk и modloader независимо совпадают)
constexpr uintptr_t ADDR_FRONTEND_MENU_MANAGER = 0xBA6748;
constexpr uintptr_t ADDR_PROCESS_MENU_OPTIONS  = 0x576FE0;
constexpr uintptr_t ADDR_MENU_INITIALISE       = 0x5744D0; // CMenuManager::Initialise()

// Смещения полей внутри CMenuManager
constexpr int OFFSET_CURRENT_MENU_ENTRY = 0x54;  // int  m_nCurrentMenuEntry
constexpr int OFFSET_CURRENT_MENU_PAGE  = 0x15D; // char m_nCurrentMenuPage

constexpr char MENUPAGE_MAIN_MENU = 34;

using ProcessMenuOptions_t = void(__thiscall*)(void* thisPtr, char input, char* exitFlag, char enter);

// Хак для хука __thiscall-метода: __fastcall передаёт первый аргумент в ECX,
// точно так же, как __thiscall передаёт this. Второй параметр (EDX) не используется.
using Initialise_t = void(__fastcall*)(void* thisPtr, void* /*unused edx*/);

static Initialise_t oInitialise = nullptr;

static void PressEnterOnFirstEntry(void* mm, ProcessMenuOptions_t processMenuOptions) {
    *reinterpret_cast<int*>(reinterpret_cast<BYTE*>(mm) + OFFSET_CURRENT_MENU_ENTRY) = 0;
    char exitFlag = 0;
    processMenuOptions(mm, 0, &exitFlag, 1);
}

static void __fastcall hkInitialise(void* thisPtr, void* /*edx*/) {
    // Сначала даём игре нормально выполнить СВОЮ родную инициализацию:
    // загрузка текстур фронтенда, установка её собственных дефолтов и т.д.
    oInitialise(thisPtr, nullptr);

    // А теперь — синхронно, тем же (главным) потоком, СРАЗУ после родной
    // инициализации и ДО первого вызова отрисовки меню — подменяем состояние.
    // Никакого отдельного потока, никакой гонки, никакой видимой вспышки меню.
    auto processMenuOptions = reinterpret_cast<ProcessMenuOptions_t>(ADDR_PROCESS_MENU_OPTIONS);
    BYTE* mm = reinterpret_cast<BYTE*>(thisPtr);

    *reinterpret_cast<char*>(mm + OFFSET_CURRENT_MENU_PAGE) = MENUPAGE_MAIN_MENU;

    // Main Menu -> "Start Game" -> открывает подменю Game
    PressEnterOnFirstEntry(mm, processMenuOptions);
    // Game -> "New Game" -> запускает загрузку
    PressEnterOnFirstEntry(mm, processMenuOptions);
}

static void InstallHook() {
    if (MH_Initialize() != MH_OK) {
        return;
    }

    void* target = reinterpret_cast<void*>(ADDR_MENU_INITIALISE);
    if (MH_CreateHook(target, reinterpret_cast<void*>(&hkInitialise),
                       reinterpret_cast<void**>(&oInitialise)) != MH_OK) {
        return;
    }

    MH_EnableHook(target);
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID) {
    if (reason == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(hModule);
        InstallHook(); // без потоков, без Sleep — хук сработает сам в нужный момент
    }
    return TRUE;
}
