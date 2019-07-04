#include <stdio.h>
#include <windows.h>

#include "Display.h"
#include "Animation.h"

static Menu *globalMenu;
static bool globalQuit;
static unsigned int globalButtonsHeld;
static unsigned int globalSelected;
static unsigned int globalPage;
static unsigned int globalSeconds;
static Animation *globalAnimation;

static const unsigned int SELECTION_MAPPING[12] = {
    0, 3, 6, 9, 1, 4, 7, 10, 2, 5, 8, 11
};

static const unsigned int DRAW_MAPPING[15] = {
    0, 3, 6, 9, 12, 1, 4, 7, 10, 13, 2, 5, 8, 11, 14
};

static const unsigned int HIGHLIGHT_MAPPING[15] = {
    0, 1, 2, 3, 16, 4, 5, 6, 7, 16, 8, 9, 10, 11, 16
};

void MessageHandler()
{
    /* Handle windows message pump */
    MSG msg = { };
    while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}

void DrawCenteredText(HDC hdc, RECT rect, wchar_t *text)
{
    /* Save original specs */
    int top = rect.top;
    int bottom = rect.bottom;
    int left = rect.left;
    int right = rect.right;

    /* Draw text, get the height, clip it, and then later draw a rectangle over it to erase. */
    int buttonHeight = bottom - top + 1;
    int textHeight = DrawText(hdc, text, -1, &rect, DT_HIDEPREFIX | DT_WORDBREAK | DT_CALCRECT);
    if (textHeight > buttonHeight)
    {
        textHeight = buttonHeight;
    }

    /* Now, draw text vertically centered in rectangle */
    rect.top = top + (buttonHeight - textHeight) / 2;
    rect.bottom = bottom;
    rect.left = left;
    rect.right = right;
    DrawText(hdc, text, -1, &rect, DT_HIDEPREFIX | DT_CENTER | DT_TOP | DT_WORDBREAK);
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_CLOSE:
        DestroyWindow(hwnd);
        globalQuit = true;
        return 0;
    case WM_DESTROY:
        PostQuitMessage(0);
        globalQuit = true;
        return 0;
    case WM_QUIT:
        globalQuit = true;
        return 0;
    case WM_PAINT:
        /* Grab the maximum number of menu items */
        unsigned int maxEntries = globalMenu->NumberOfEntries();

        /* Set up double buffer */
        PAINTSTRUCT ps;
        HDC windowHdc = BeginPaint(hwnd, &ps);
        HDC hdc = CreateCompatibleDC(windowHdc);
        HBITMAP Membitmap = CreateCompatibleBitmap(windowHdc, SCREEN_WIDTH, SCREEN_HEIGHT);
        SelectObject(hdc, Membitmap);

        /* Paint the window background */
        HBRUSH background = CreateSolidBrush(RGB(0,0,0));
        FillRect(hdc, &ps.rcPaint, background);
        DeleteObject(background);

        /* Set up text display */
        SetTextColor(hdc, RGB(240, 240, 240));
        SetBkMode(hdc, TRANSPARENT);
        HFONT hItemFont = CreateFont(ITEM_FONT_SIZE, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, L"Verdana");
        HFONT hInstructionsFont = CreateFont(INSTRUCTIONS_FONT_SIZE, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, L"Verdana");
        HFONT hArrowFont = CreateFont(ARROW_FONT_SIZE, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, L"Verdana");

        /* Set up rectangle display */
        HPEN redPen = CreatePen(PS_SOLID | PS_INSIDEFRAME, 2, RGB(255,0,0));
        HPEN whitePen = CreatePen(PS_SOLID | PS_INSIDEFRAME, 2, RGB(255,255,255));
        HPEN noPen = CreatePen(PS_NULL, 0, RGB(0, 0, 0));

        /* Draw top instructions */
        {
            SelectObject(hdc, GetStockObject(DC_BRUSH));
            SetDCBrushColor(hdc, RGB(24,24,24));
            SelectObject(hdc, noPen);
            Rectangle(hdc, 0, VIEWPORT_TOP, SCREEN_WIDTH + 1, VIEWPORT_BOTTOM + 1);

            RECT rect;
            rect.top = VIEWPORT_TOP + TEXT_PADDING;
            rect.bottom = VIEWPORT_BOTTOM - TEXT_PADDING;
            rect.left = TEXT_PADDING;
            rect.right = SCREEN_WIDTH - TEXT_PADDING;

            char instruction_text[1024];
            sprintf_s(
                instruction_text,
                1024,
                "Select a game by tapping its panel.\n"
                "Start %s by tapping the Start Game panel.\n"
                "The selected game will auto-start in %d seconds.",
                globalMenu->GetEntryName(globalSelected),
                globalSeconds
                );

            wchar_t* wString = new wchar_t[4096];
            MultiByteToWideChar(CP_ACP, 0, instruction_text, -1, wString, 4096);
            SelectObject(hdc, hInstructionsFont);
            DrawText(hdc, wString, -1, &rect, DT_HIDEPREFIX | DT_LEFT | DT_TOP | DT_WORDBREAK);
            delete wString;
        }

        /* Draw hover icons for every square, regardless of whether we put graphics in them */
        {
            /* Set up background colors */
            SelectObject(hdc, GetStockObject(DC_BRUSH));
            SetDCBrushColor(hdc, RGB(24,24,24));

            /* Intentionally skip button 16 because pressing it will always
               insta-launch the selected game, so we will never need to show
               a hover square. */
            for( unsigned int position = 12; position < 15; position++ )
            {
                /* Set up border color to show current held buttons */
                if (!((globalButtonsHeld >> position) & 1))
                {
                    // Don't show hover when not being pressed
                    SelectObject(hdc, noPen);
                }
                else
                {
                    // Held and not animating, show hover.
                    SelectObject(hdc, redPen);
                }

                unsigned int xPos = position % 4;
                unsigned int yPos = position / 4;

                /* There is an extra pixel of bump for the second half of the squares */
                unsigned int xBump = xPos >= 2 ? 1 : 0;
                unsigned int yBump = yPos >= 2 ? 1 : 0;

                unsigned int top = BUTTON_TOP + (BUTTON_VERTICAL_STRIDE * yPos) + yBump;
                unsigned int bottom = top + BUTTON_WIDTH;
                unsigned int left = BUTTON_LEFT + (BUTTON_HORIZONTAL_STRIDE * xPos) + xBump;
                unsigned int right = left + BUTTON_HEIGHT;

                // Draw bounding rectangle
                Rectangle(hdc, left, top, right, bottom);
            }
        }

        /* Draw selection button */
        {
            unsigned int top = BUTTON_TOP + (BUTTON_VERTICAL_STRIDE * 3) + 1;
            unsigned int bottom = top + BUTTON_WIDTH;
            unsigned int left = BUTTON_LEFT + (BUTTON_HORIZONTAL_STRIDE * 3) + 1;
            unsigned int right = left + BUTTON_HEIGHT;

            SelectObject(hdc, GetStockObject(DC_BRUSH));
            SetDCBrushColor(hdc, RGB(24,24,24));
            SelectObject(hdc, whitePen);
            Rectangle(hdc, left, top, right, bottom);

            RECT rect;
            rect.top = top + TEXT_PADDING;
            rect.bottom = bottom - TEXT_PADDING;
            rect.left = left + TEXT_PADDING;
            rect.right = right - TEXT_PADDING;

            char start_text[64];
            sprintf_s(start_text, 64, "Start Game\n\n%d seconds left", globalSeconds);

            wchar_t* wString = new wchar_t[4096];
            MultiByteToWideChar(CP_ACP, 0, start_text, -1, wString, 4096);
            SelectObject(hdc, hItemFont);
            DrawCenteredText(hdc, rect, wString);
            delete wString;
        }

        /* Draw previous/next page buttons */
        if (maxEntries > GAMES_PER_PAGE)
        {
            unsigned int max_pages = ((maxEntries - GAMES_PER_PAGE) + 2) / 3;

            SelectObject(hdc, GetStockObject(DC_BRUSH));
            SetDCBrushColor(hdc, RGB(24,24,24));

            /* Scroll left button */
            if (globalPage < max_pages)
            {
                unsigned int top = BUTTON_TOP + (BUTTON_VERTICAL_STRIDE * 3) + 1;
                unsigned int bottom = top + BUTTON_WIDTH;
                unsigned int left = BUTTON_LEFT;
                unsigned int right = left + BUTTON_HEIGHT;

                if ((globalButtonsHeld >> 12) & 1)
                {
                    SelectObject(hdc, redPen);
                }
                else
                {
                    SelectObject(hdc, whitePen);
                }

                // Draw bounding rectangle
                Rectangle(hdc, left, top, right, bottom);

                // Draw text
                RECT rect;
                rect.top = top + TEXT_PADDING;
                rect.bottom = bottom - TEXT_PADDING;
                rect.left = left + TEXT_PADDING;
                rect.right = right - TEXT_PADDING;

                wchar_t* wString = new wchar_t[4096];
                MultiByteToWideChar(CP_ACP, 0, "<<", -1, wString, 4096);
                SelectObject(hdc, hArrowFont);
                DrawText(hdc, wString, -1, &rect, DT_HIDEPREFIX | DT_CENTER | DT_VCENTER | DT_SINGLELINE);
                delete wString;
            }

            /* Scroll right button */
            if (globalPage > 0)
            {
                unsigned int top = BUTTON_TOP + (BUTTON_VERTICAL_STRIDE * 3) + 1;
                unsigned int bottom = top + BUTTON_WIDTH;
                unsigned int left = BUTTON_LEFT + BUTTON_HORIZONTAL_STRIDE;
                unsigned int right = left + BUTTON_HEIGHT;

                if ((globalButtonsHeld >> 13) & 1)
                {
                    SelectObject(hdc, redPen);
                }
                else
                {
                    SelectObject(hdc, whitePen);
                }

                // Draw bounding rectangle
                Rectangle(hdc, left, top, right, bottom);

                // Draw text
                RECT rect;
                rect.top = top + TEXT_PADDING;
                rect.bottom = bottom - TEXT_PADDING;
                rect.left = left + TEXT_PADDING;
                rect.right = right - TEXT_PADDING;

                wchar_t* wString = new wchar_t[4096];
                MultiByteToWideChar(CP_ACP, 0, ">>", -1, wString, 4096);
                SelectObject(hdc, hArrowFont);
                DrawText(hdc, wString, -1, &rect, DT_HIDEPREFIX | DT_CENTER | DT_VCENTER | DT_SINGLELINE);
                delete wString;
            }
        }

        /* Draw each menu item */
        for( unsigned int position = 0; position < 15; position++ )
        {
            /* Leave room for animating left/right swipe */
            unsigned int xPos = position % 5;
            unsigned int yPos = position / 5;

            /* Look up the actual item at this position */
            unsigned int item = DRAW_MAPPING[position] + (globalPage * 3);

            /* There is an extra pixel of bump for the second half of the squares */
            unsigned int xBump = xPos >= 2 ? 1 : 0;
            unsigned int yBump = yPos >= 2 ? 1 : 0;

            /* Adjust position for animations */
            int animationBump = 0;
            if (globalAnimation->IsAnimating())
            {
                animationBump = globalAnimation->Position();
            }

            int top = BUTTON_TOP + (BUTTON_VERTICAL_STRIDE * yPos) + yBump;
            int bottom = top + BUTTON_WIDTH;
            int left = BUTTON_LEFT + (BUTTON_HORIZONTAL_STRIDE * xPos) + xBump + animationBump;
            int right = left + BUTTON_HEIGHT;

            /* Set up background colors */
            SelectObject(hdc, GetStockObject(DC_BRUSH));
            if (globalSelected == item)
            {
                SetDCBrushColor(hdc, RGB(96,96,96));
            }
            else
            {
                SetDCBrushColor(hdc, RGB(24,24,24));
            }

            // Set up border color to show current held buttons
            if (!globalAnimation->IsAnimating() && ((globalButtonsHeld >> HIGHLIGHT_MAPPING[position]) & 1))
            {
                SelectObject(hdc, redPen);
            }
            else if (item >= maxEntries)
            {
                SelectObject(hdc, noPen);
            }
            else
            {
                SelectObject(hdc, whitePen);
            }

            /* Draw bounding rectangle */
            Rectangle(hdc, left, top, right, bottom);
            if (item >= maxEntries) { continue; }

            /* Draw text */
            RECT rect;
            rect.top = top + TEXT_PADDING;
            rect.bottom = bottom - TEXT_PADDING;
            rect.left = left + TEXT_PADDING;
            rect.right = right - TEXT_PADDING;

            wchar_t* wString = new wchar_t[4096];
            MultiByteToWideChar(CP_ACP, 0, globalMenu->GetEntryName(item), -1, wString, 4096);
            SelectObject(hdc, hItemFont);
            DrawCenteredText(hdc, rect, wString);
            delete wString;
        }

        /* Finally, draw black bars around sections that shouldn't have graphics. */
        {
            SelectObject(hdc, GetStockObject(DC_BRUSH));
            SetDCBrushColor(hdc, RGB(0,0,0));
            SelectObject(hdc, noPen);

            /* Outside edges */
            Rectangle(hdc, 0, BUTTON_TOP, BUTTON_LEFT, SCREEN_HEIGHT + 1);
            Rectangle(hdc, BUTTON_LEFT + (BUTTON_HORIZONTAL_STRIDE * 3) + BUTTON_WIDTH + 1, BUTTON_TOP, SCREEN_WIDTH + 1, SCREEN_HEIGHT + 1);

            /* In between buttons */
            Rectangle(hdc, BUTTON_LEFT + BUTTON_WIDTH, BUTTON_TOP, BUTTON_LEFT + BUTTON_HORIZONTAL_STRIDE, SCREEN_HEIGHT + 1);
            Rectangle(hdc, BUTTON_LEFT + BUTTON_HORIZONTAL_STRIDE + BUTTON_WIDTH, BUTTON_TOP, BUTTON_LEFT + (BUTTON_HORIZONTAL_STRIDE * 2) + 1, SCREEN_HEIGHT + 1);
            Rectangle(hdc, BUTTON_LEFT + (BUTTON_HORIZONTAL_STRIDE * 2) + BUTTON_WIDTH + 1, BUTTON_TOP, BUTTON_LEFT + (BUTTON_HORIZONTAL_STRIDE * 3) + 1, SCREEN_HEIGHT + 1);
        }

        DeleteObject(hItemFont);
        DeleteObject(hInstructionsFont);
        DeleteObject(hArrowFont);

        /* Copy double-buffer over */
        BitBlt(windowHdc, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, hdc, 0, 0, SRCCOPY);
        DeleteObject(Membitmap);
        DeleteDC(hdc);
        DeleteDC(windowHdc);

        EndPaint(hwnd, &ps);
        return 0;
    }

    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}


Display::Display(HINSTANCE hInstance, IO *ioInst, Menu *mInst)
{
    globalAnimation = new Animation();
    inst = hInstance;
    globalMenu = mInst;
    io = ioInst;
    menu = mInst;
    page = 0;
    selected = 0;
    newPage = -1;
    lastLocation = 0;
    leftPresses = 0;
    rightPresses = 0;

    // Register the callback
    WNDCLASS wc = { };
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = inst;
    wc.lpszClassName = CLASS_NAME;
    RegisterClass(&wc);
    globalQuit = false;
    globalButtonsHeld = 0;
    globalSelected = selected;
    globalPage = page;
    globalSeconds = menu->SecondsLeft();

    // Create an empty window
    hwnd = CreateWindow(CLASS_NAME, 0, WS_BORDER, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, NULL, NULL, inst, NULL);
    LONG lStyle = GetWindowLong(hwnd, GWL_STYLE);
    lStyle &= ~(WS_CAPTION | WS_THICKFRAME | WS_MINIMIZE | WS_MAXIMIZE | WS_SYSMENU);
    SetWindowLong(hwnd, GWL_STYLE, lStyle);
    LONG lExStyle = GetWindowLong(hwnd, GWL_EXSTYLE);
    lExStyle &= ~(WS_EX_DLGMODALFRAME | WS_EX_CLIENTEDGE | WS_EX_STATICEDGE);
    SetWindowLong(hwnd, GWL_EXSTYLE, lExStyle);

    /* Display it */
    SetWindowPos(hwnd, NULL, 0,0,0,0, SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOOWNERZORDER);
    ShowWindow(hwnd, SW_SHOW);
    UpdateWindow(hwnd);
    ShowCursor(false);
}

Display::~Display()
{
    ShowCursor(true);
    DestroyWindow(hwnd);
    UnregisterClass(CLASS_NAME, inst);
    delete globalAnimation;
}

void Display::InvalidateOnUpdates()
{
    unsigned int buttonsHeld = io->ButtonsHeld();
    bool update = false;

    if (globalButtonsHeld != buttonsHeld)
    {
        globalButtonsHeld = buttonsHeld;
        update = true;
    }
    if (globalSelected != selected)
    {
        globalSelected = selected;
        update = true;
    }
    if (lastLocation != globalAnimation->Position())
    {
        lastLocation = globalAnimation->Position();
        update = true;
    }
    if (globalPage != page)
    {
        globalPage = page;
        update = true;
    }
    if (globalSeconds != menu->SecondsLeft())
    {
        globalSeconds = menu->SecondsLeft();
        update = true;
    }

    if (update)
    {
        InvalidateRect(hwnd, NULL, FALSE);
        UpdateWindow(hwnd);
    }
}

void Display::Tick(void)
{
    /* Make sure animations happen */
    globalAnimation->Tick();

    /* Make sure we respect multiple inputs while animating */
    unsigned int max_pages = ((menu->NumberOfEntries() - GAMES_PER_PAGE) + 2) / 3;
    if (io->ButtonPressed(BUTTON_13))
    {
        leftPresses ++;
        if (page < (max_pages - 1))
        {
            /* We're not at the last page, so quick-scroll the current animation */
            globalAnimation->CancelDeceleration();
        }
    }
    if (io->ButtonPressed(BUTTON_14))
    {
        rightPresses ++;
        if (page > 0)
        {
            /* We're not at the last page, so quick-scroll the current animation */
            globalAnimation->CancelDeceleration();
        }
    }

    /* Don't handle game select input while animating */
    if (globalAnimation->IsAnimating())
    {
        InvalidateOnUpdates();
        MessageHandler();
        return;
    }

    /* See if we need to update the page after finishing animation */
    if (newPage >= 0)
    {
        page = newPage;
        newPage = -1;

        InvalidateOnUpdates();
        MessageHandler();
        return;
    }

    /* Grab the currently held buttons, calculate newly pressed */
    bool moved = false;

    /* Figure out actions */
    if (menu->NumberOfEntries() > GAMES_PER_PAGE)
    {
        /* Activate page left/right buttons */
        if (leftPresses > 0)
        {
            /* Scroll left */
            if (page < (max_pages - 1))
            {
                /* We have more pages we can animate scrolling to */
                leftPresses --;
            }
            else
            {
                /* Next page is the end, we should decelerate */
                leftPresses = 0;
            }

            if (page < max_pages)
            {
                globalAnimation->Animate(0, -BUTTON_HORIZONTAL_STRIDE, ANIMATION_SPEED, leftPresses == 0);
                lastLocation = globalAnimation->Position();
                newPage = page + 1;
                moved = true;
            }
        }
        else if (rightPresses > 0)
        {
            /* Scroll right */
            if (page > 1)
            {
                /* We have more pages we can animate scrolling to */
                rightPresses --;
            }
            else
            {
                /* Next page is the end, we should decelerate */
                rightPresses = 0;
            }

            if (page > 0)
            {
                globalAnimation->Animate(-BUTTON_HORIZONTAL_STRIDE, BUTTON_HORIZONTAL_STRIDE, ANIMATION_SPEED, rightPresses == 0);
                lastLocation = globalAnimation->Position();
                page--;
                newPage = page;
                moved = true;
            }
        }
    }

    if (!moved)
    {
        /* Allow button presses to make selections */
        for (unsigned int btn = 0; btn < 12; btn++) {
            if (io->ButtonPressed(1 << btn)) {
                unsigned int item = SELECTION_MAPPING[btn] + (globalPage * 3);
                if (item < menu->NumberOfEntries())
                {
                    selected = item;
                }
            }
        }
    }

    /* Update the screen to show any button presses or selection
    changes. */
    InvalidateOnUpdates();
    MessageHandler();
}

bool Display::WasClosed()
{
    return globalQuit;
}

unsigned int Display::GetSelectedItem()
{
    return selected;
}