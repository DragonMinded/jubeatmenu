#include <stdio.h>
#include <windows.h>

#include "Display.h"

static Menu *globalMenu;
static bool globalQuit;
static unsigned int globalButtonsHeld;
static unsigned int globalSelected;
static unsigned int globalPage;
static unsigned int globalSeconds;

static const unsigned int SELECTION_MAPPING[12] = {
	0, 3, 6, 9, 1, 4, 7, 10, 2, 5, 8, 11
};

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

			/* Start patinting */
			PAINTSTRUCT ps;
			HDC hdc = BeginPaint(hwnd, &ps);

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

			/* Draw top instructions */
			{
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
			for( unsigned int position = 0; position < 15; position++ )
			{
				unsigned int xPos = position % 4;
				unsigned int yPos = position / 4;

				/* There is an extra pixel of bump for the second half of the squares */
				unsigned int xBump = xPos >= 2 ? 1 : 0;
				unsigned int yBump = yPos >= 2 ? 1 : 0;

				unsigned int top = BUTTON_TOP + (BUTTON_VERTICAL_STRIDE * yPos) + yBump;
				unsigned int bottom = top + BUTTON_WIDTH;
				unsigned int left = BUTTON_LEFT + (BUTTON_HORIZONTAL_STRIDE * xPos) + xBump;
				unsigned int right = left + BUTTON_HEIGHT;
				
				/* Set up background colors */
				SelectObject(hdc, GetStockObject(DC_BRUSH));
				SetDCBrushColor(hdc, RGB(0,0,0));

				// Set up border color to show current held buttons
				SelectObject(hdc, GetStockObject(DC_PEN));
				if ((globalButtonsHeld >> position) & 1)
				{
					SetDCPenColor(hdc, RGB(255,0,0));
				}
				else
				{
					continue;
				}
				
				// Draw bounding rectangle
				Rectangle(hdc, left, top, right, bottom);
			}

			/* Draw selection button */
			{
				unsigned int top = BUTTON_TOP + (BUTTON_VERTICAL_STRIDE * 3) + 1;
				unsigned int bottom = top + BUTTON_WIDTH;
				unsigned int left = BUTTON_LEFT + (BUTTON_HORIZONTAL_STRIDE * 3) + 1;
				unsigned int right = left + BUTTON_HEIGHT;

				SelectObject(hdc, GetStockObject(DC_BRUSH));
				SetDCBrushColor(hdc, RGB(24,24,24));
				SetDCPenColor(hdc, RGB(255,255,255));
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
				DrawText(hdc, wString, -1, &rect, DT_HIDEPREFIX | DT_CENTER | DT_TOP);
				delete wString;
			}

			/* Draw previous/next page buttons */
			if (maxEntries > GAMES_PER_PAGE)
			{
				unsigned int max_pages = ((maxEntries - GAMES_PER_PAGE) + 2) / 3;
				
				/* Scroll left button */
				if (globalPage < max_pages)
				{
					unsigned int top = BUTTON_TOP + (BUTTON_VERTICAL_STRIDE * 3) + 1;
					unsigned int bottom = top + BUTTON_WIDTH;
					unsigned int left = BUTTON_LEFT + (BUTTON_HORIZONTAL_STRIDE * 0) + 1;
					unsigned int right = left + BUTTON_HEIGHT;

					SelectObject(hdc, GetStockObject(DC_BRUSH));
					SetDCBrushColor(hdc, RGB(24,24,24));
					if ((globalButtonsHeld >> 12) & 1)
					{
						SetDCPenColor(hdc, RGB(255,0,0));
					}
					else
					{
						SetDCPenColor(hdc, RGB(255,255,255));
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
					unsigned int left = BUTTON_LEFT + (BUTTON_HORIZONTAL_STRIDE * 1) + 1;
					unsigned int right = left + BUTTON_HEIGHT;

					SelectObject(hdc, GetStockObject(DC_BRUSH));
					SetDCBrushColor(hdc, RGB(24,24,24));
					if ((globalButtonsHeld >> 13) & 1)
					{
						SetDCPenColor(hdc, RGB(255,0,0));
					}
					else
					{
						SetDCPenColor(hdc, RGB(255,255,255));
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
			for( unsigned int position = 0; position < 12; position++ ) {
				unsigned int xPos = position % 4;
				unsigned int yPos = position / 4;

				/* Look up the actual item at this position */
				unsigned int item = SELECTION_MAPPING[position] + (globalPage * 3);
				if (item >= maxEntries) { continue; }

				/* There is an extra pixel of bump for the second half of the squares */
				unsigned int xBump = xPos >= 2 ? 1 : 0;
				unsigned int yBump = yPos >= 2 ? 1 : 0;

				unsigned int top = BUTTON_TOP + (BUTTON_VERTICAL_STRIDE * yPos) + yBump;
				unsigned int bottom = top + BUTTON_WIDTH;
				unsigned int left = BUTTON_LEFT + (BUTTON_HORIZONTAL_STRIDE * xPos) + xBump;
				unsigned int right = left + BUTTON_HEIGHT;
				
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
				SelectObject(hdc, GetStockObject(DC_PEN));
				if ((globalButtonsHeld >> position) & 1)
				{
					SetDCPenColor(hdc, RGB(255,0,0));
				}
				else
				{
					SetDCPenColor(hdc, RGB(255,255,255));
				}
				
				// Draw bounding rectangle
				Rectangle(hdc, left, top, right, bottom);

				// Set text color
				SelectObject(hdc, GetStockObject(DC_PEN));
				SetDCPenColor(hdc, RGB(255,255,255));

				// Draw text
				RECT rect;
				rect.top = top + TEXT_PADDING;
				rect.bottom = bottom - TEXT_PADDING;
				rect.left = left + TEXT_PADDING;
				rect.right = right - TEXT_PADDING;

				wchar_t* wString = new wchar_t[4096];
				MultiByteToWideChar(CP_ACP, 0, globalMenu->GetEntryName(item), -1, wString, 4096);
				SelectObject(hdc, hItemFont);
				DrawText(hdc, wString, -1, &rect, DT_HIDEPREFIX | DT_CENTER | DT_TOP | DT_WORDBREAK);
				delete wString;
        	}

			DeleteObject(hItemFont);
			DeleteObject(hInstructionsFont);
			DeleteObject(hArrowFont);
			
            EndPaint(hwnd, &ps);
			return 0;
	}

    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}


Display::Display(HINSTANCE hInstance, IO *ioInst, Menu *mInst)
{
	inst = hInstance;
	globalMenu = mInst;
	io = ioInst;
	menu = mInst;
	page = 0;
	selected = 0;

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

Display::~Display(void)
{
	ShowCursor(true);
	DestroyWindow(hwnd);
	UnregisterClass(CLASS_NAME, inst);
}

void Display::Tick(void)
{
	/* Handle windows message pump */
    MSG msg = { };
    while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
	}

	/* Grab the currently held buttons, calculate newly pressed */
	bool moved = false;
	unsigned int buttonsHeld = io->ButtonsHeld();

	/* Figure out actions */
	if (menu->NumberOfEntries() > GAMES_PER_PAGE)
	{
		unsigned int max_pages = ((menu->NumberOfEntries() - GAMES_PER_PAGE) + 2) / 3;

		/* Activate page left/right buttons */
		if (io->ButtonPressed(BUTTON_13))
		{
			/* Scroll left */
			if (page < max_pages)
			{
				page++;
				moved = true;
			}
		}

		if (io->ButtonPressed(BUTTON_14))
		{
			/* Scroll right */
			if (page > 0)
			{
				page--;
				moved = true;
			}
		}
	}

	if (!moved) {
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
	if (globalPage != page)
	{
		globalPage = page;
		update = true;
	}
	if (globalSeconds != menu->SecondsLeft()) {
		globalSeconds = menu->SecondsLeft();
		update = true;
	}

	if (update)
	{
		InvalidateRect(hwnd, NULL, NULL);
		UpdateWindow(hwnd);
	}
}

bool Display::WasClosed()
{
	return globalQuit;
}

unsigned int Display::GetSelectedItem()
{
	return selected;
}