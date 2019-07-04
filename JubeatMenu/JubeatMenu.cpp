#include <windows.h>
#include <stdio.h>
#include <stdarg.h>
#include <tchar.h>
#include "Display.h"
#include "Menu.h"
#include "IO.h"

// Instance for passing to a window we render
static HMODULE globalModule = NULL;
static Menu *globalMenu = NULL;

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
    if (ul_reason_for_call == DLL_PROCESS_ATTACH)
    {
        /* Remember our own instance*/
        globalModule = hModule;

        /* Load the original DLL to force dependencies to load.
        This must be done BEFORE calling dll_entry_init or
        this menu can't be run with a hook DLL. */
        LoadLibraryA("jubeat.dll");

        /* Initialize the menu before any hooks run so our
        file operations don't get intercepted. */
        globalMenu = new Menu(L"games.ini");
    }

    return TRUE;
}

extern "C" __declspec(dllexport) int __cdecl dll_entry_init(int a1, int a2)
{	
    // Initialize the IO
    IO *io = new IO();
    if (!io->Ready())
    {
        // Failed to initialize, give up
        ExitProcess(0);
        return 0;
    }

    // Initialize the menu
    Menu *menu = globalMenu;
    if( menu->NumberOfEntries() < 1 )
    {
        io->ErrorMessage("No games configured to launch in games.ini!");

        delete menu;
        delete io;

        ExitProcess(0);
        return 0;
    }

    // Create menu screen
    Display *display = new Display(globalModule, io, menu);

    /* Actual game to load */
    char *path = NULL;

    /* It may have taken a long time to init */
    menu->ResetTimeout();

    // Input loop
    while(true) {
        io->Tick();
        menu->Tick();
        display->Tick();

        /* See if somebody killed the display window */
        if (display->WasClosed())
        {
            io->ErrorMessage("Main window closed, exiting!");
            break;
        }

        /* Check to see if we ran out of time waiting for input */
        if (menu->ShouldBootDefault())
        {
            io->ErrorMessage("Ran out of time, booting current selection!");
            int entry = display->GetSelectedItem();
            path = menu->GetEntryPath(entry);
            break;
        }

        /* Check to see if the user confirmed a selection */
        if (io->ButtonPressed(BUTTON_16)) {
            int entry = display->GetSelectedItem();
            path = menu->GetEntryPath(entry);
            break;
        }
    }

    // Close and free libraries
    delete display;
    delete menu;
    delete io;

    if (path != NULL)
    {
        /* Crate a batch file that launches the game after we exit */
        char tempPath[1024];
        GetTempPathA(1024, tempPath);
        strncat_s(tempPath, 1024, "/launch.bat", 11);
        HANDLE hBat = CreateFileA(tempPath, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

        char command[MAX_GAME_LOCATION_LENGTH + 128];
        sprintf_s(command, MAX_GAME_LOCATION_LENGTH + 128, "ping 127.0.0.1 -n 5 -w 1000 > nul\r\n%s\r\n", path);
        DWORD bytesWritten;
        WriteFile(hBat, command, strlen(command), &bytesWritten, NULL);
        CloseHandle(hBat);

        wchar_t* wCommand = new wchar_t[4096];
        MultiByteToWideChar(CP_ACP, 0, tempPath, -1, wCommand, 4096);

        STARTUPINFO info={sizeof(info)};
        PROCESS_INFORMATION processInfo;
        CreateProcess(NULL, wCommand, NULL, NULL, FALSE, 0, NULL, NULL, &info, &processInfo);

        delete wCommand;
    }

    // Return failure so launcher.exe doesn't attempt to
    // further initialize other threads such as the net thread.
    TerminateProcess(GetCurrentProcess(), 0);
    return 0;
}

extern "C" __declspec(dllexport) int __cdecl dll_entry_main()
{
    return 0;
}