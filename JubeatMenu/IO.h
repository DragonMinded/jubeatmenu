#pragma once

#include <tchar.h>
#include <windows.h>

// Types and setup for logging to AVS standard output.
typedef int (*log_func)(char *cat, char *fmt, ...);

// Types for interacting with device.dll
typedef int(*INT_RET_NO_ARGS)();
typedef int(*INT_RET_ONE_ARG)(int in);
typedef int(*INT_RET_THREE_ARGS)(int x, int y, int *out);

// Debug that outputs to AVS logs.
#define debug(...) do { if (log_function != NULL) { log_function("menu", __VA_ARGS__); } } while(0)

// Button definitions
#define BUTTON_1 0x0001
#define BUTTON_2 0x0002
#define BUTTON_3 0x0004
#define BUTTON_4 0x0008
#define BUTTON_5 0x0010
#define BUTTON_6 0x0020
#define BUTTON_7 0x0040
#define BUTTON_8 0x0080
#define BUTTON_9 0x0100
#define BUTTON_10 0x0200
#define BUTTON_11 0x0400
#define BUTTON_12 0x0800
#define BUTTON_13 0x1000
#define BUTTON_14 0x2000
#define BUTTON_15 0x4000
#define BUTTON_16 0x8000

class IO
{
public:
    IO();
    ~IO();

    bool Ready();
    void Tick();
    void ErrorMessage(char *msg);
    unsigned int ButtonsHeld();
    unsigned int ButtonsPressed();
    bool ButtonHeld(unsigned int button);
    bool ButtonPressed(unsigned int button);
private:
    HMODULE core;
    HMODULE device;

    INT_RET_ONE_ARG device_initialize;
    INT_RET_NO_ARGS device_is_initialized;
    INT_RET_NO_ARGS device_get_status;
    INT_RET_ONE_ARG device_set_panel_mode;
    INT_RET_THREE_ARGS device_get_panel_trg_on;
    INT_RET_THREE_ARGS device_get_panel_trg_off;
    INT_RET_THREE_ARGS device_get_panel_trg_short_on;
    INT_RET_NO_ARGS device_update;
    INT_RET_NO_ARGS device_finalize;

    log_func log_function;

    bool is_ready;
    unsigned int buttons;
    unsigned int lastButtons;
};