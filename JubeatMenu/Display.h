#pragma once

#include <stdio.h>
#include <windows.h>

#include "Menu.h"
#include "IO.h"

#define CLASS_NAME L"Jubeat Touch Launcher"

#define ITEM_FONT_SIZE 20
#define INSTRUCTIONS_FONT_SIZE 28
#define ARROW_FONT_SIZE 40
#define SCREEN_WIDTH 768
#define SCREEN_HEIGHT 1360

#define VIEWPORT_TOP 0
#define VIEWPORT_BOTTOM 463

#define BUTTON_LEFT 8
#define BUTTON_TOP 600
#define BUTTON_WIDTH 159
#define BUTTON_HEIGHT 159

#define BUTTON_HORIZONTAL_STRIDE 197
#define BUTTON_VERTICAL_STRIDE 195

#define TEXT_PADDING 10

#define ANIMATION_SPEED 1000

/* Number of games to display on one screen */
#define GAMES_PER_PAGE 12

class Display
{
public:
    Display(HINSTANCE hInstance, IO *ioInst, Menu *mInst);
    ~Display();

    void Tick();
    bool WasClosed();

    void ButtonPress(int button);
    void ButtonRelease(int button);

    unsigned int GetSelectedItem();

private:
    void InvalidateOnUpdates();

    HINSTANCE inst;
    HWND hwnd;

    Menu *menu;
    IO *io;

    int newPage;
    unsigned int page;
    unsigned int selected;
    int lastLocation;
    unsigned int leftPresses;
    unsigned int rightPresses;
};
