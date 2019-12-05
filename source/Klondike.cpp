// Copyright 2007-2019 Flenniken
// All Rights Reserved.

#include "stdafx.h"
#include "people.h"
#include "Klondike.h"
#include <vector>
#include <algorithm>
#include "TraceToNotepad.h"
#include "Game.h"
#include <shlwapi.h>
#include <gdiplus.h>
#include "Stats.h"
#include "Strategy.h"
#include "BigCard.h"
#include "AboutBox.h"
#include "SolitaireIniFile.h"
#include "Options.h"
#include "Utils.h"

#define MAX_LOADSTRING 100

// Global Variables:
HINSTANCE gInstance;
HWND gMainWindow;
static TCHAR gWindowClass[] = _T("FlennikenKlondike");
static TCHAR gFlennikenKlondikeURL[] = _T("https://github.com/flenniken/solitaire");
static bool gDrawLabels;

TraceToNotepad_Object;
int32 gClientX;
int32 gClientY;
uint32 gCharY;
extern Strategy gStrategy;

Game gGame;
Stats gStats;
static TCHAR gWindowPlacementStr[] = _T("Window Placement");

static ATOM MyRegisterClass(HINSTANCE hInstance);
static bool InitInstance(HINSTANCE, int);
static LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
static INT_PTR CALLBACK About(HWND, UINT, WPARAM, LPARAM);
static void ResizeCard(int32 direction);
static void Quiting();
static void ReadIniFile();
static void WriteIniFile();
static void ReadWindowPos(int32 &show, int32 &x, int32 &y, uint32 &width, uint32 &height);
static void WriteWindowPos();
static void PositionMainWindow();

int APIENTRY _tWinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPTSTR    lpCmdLine,
                     int       nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

	CoInitialize(0);

	// Initialize GDI+.
	Gdiplus::GdiplusStartupInput gdiplusStartupInput;
	ULONG_PTR gdiplusToken;
	Gdiplus::GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);

	// Connect to the open untitled notepad window in debug mode.
	TraceToNotepad_Connect();


 	// TODO: Place code here.
	MSG msg;
	HACCEL hAccelTable;

	// Initialize global strings
	MyRegisterClass(hInstance);

	// Perform application initialization:
	if (!InitInstance (hInstance, nCmdShow))
		return FALSE;

	hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_KLONDIKE_ACCELERATORS));

	// Main message loop:
	while (GetMessage(&msg, NULL, 0, 0))
	{
		if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	Gdiplus::GdiplusShutdown(gdiplusToken);

	return (int) msg.wParam;
}



//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
//  COMMENTS:
//
//    This function and its usage are only necessary if you want this code
//    to be compatible with Win32 systems prior to the 'RegisterClassEx'
//    function that was added to Windows 95. It is important to call this function
//    so that the application will get 'well formed' small icons associated
//    with it.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
	WNDCLASSEX wcex;

	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style			= CS_DBLCLKS; // CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc	= WndProc;
	wcex.cbClsExtra		= 0;
	wcex.cbWndExtra		= 0;
	wcex.hInstance		= hInstance;
	wcex.hIcon			= LoadIcon(hInstance, MAKEINTRESOURCE(IDI_KLONDIKE_ICON));
	wcex.hCursor		= LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground	= (HBRUSH)(COLOR_WINDOW+1);
	wcex.lpszMenuName	= MAKEINTRESOURCE(IDC_KLONDIKE_MENU);
	wcex.lpszClassName	= gWindowClass;
	wcex.hIconSm		= NULL;

	return RegisterClassEx(&wcex);
}

//
//   FUNCTION: InitInstance(HINSTANCE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
bool InitInstance(HINSTANCE hInstance, int nCmdShow)
{
	gInstance = hInstance; // Store instance handle in our global variable

	// Check the Windows version number.
	if (!Utils::IsWindows2000OrAbove())
	{
		TString message = Utils::GetString(gInstance, kStrUnsupportedVersion);
		Utils::MessageBox(nil, message, MB_OK);
		return FALSE;
	}

	TString title = Utils::GetString(gInstance, kStrAppTitle);
	gMainWindow = CreateWindow(gWindowClass, title.c_str(), WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN | WS_VISIBLE,
	CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, NULL, NULL, hInstance, NULL);

	if (!gMainWindow)
		return false;

	if (gGame.OneTimeSettup())
		return false;

	PositionMainWindow();

	if (StatsUtils::CreateStatisticsWindow())
		return false;

	ReadIniFile();

//	ShowWindow(gMainWindow, nCmdShow);
	UpdateWindow(gMainWindow);
	SetFocus(gMainWindow);

	return true;
}

//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE:  Processes messages for the main window.
//
//  WM_COMMAND	- process the application menu
//  WM_PAINT	- Paint the main window
//  WM_DESTROY	- post a quit message and return
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	int wmId, wmEvent;

	switch (message)
	{
	case WM_COMMAND:
		wmId    = LOWORD(wParam);
		wmEvent = HIWORD(wParam);
		// Parse the menu selections:
		switch (wmId)
		{
		case IDM_ABOUT:
			AboutBoxUtils::ShowDialog();
			break;
		case IDM_VISITFLENNIKEN:
			Utils::OpenInDefault(gFlennikenKlondikeURL);
			break;
		case ID_FILE_STRATEGY:
			{
			gDrawLabels = true;
			InvalidateRect(gMainWindow, NULL, false);
			Strategy::ShowDialog();
			gDrawLabels = false;
			InvalidateRect(gMainWindow, NULL, false);

			TString strategyLetters;
			gStrategy.GetStrategyLetters(strategyLetters);
			gStats.SetStrategyLetters(strategyLetters);
			break;
			}
		case ID_FILE_PLAY:
			gGame.Play();
			break;
		case ID_FILE_PLAYHAND:
			{
				bool userStoppedGame;
				gGame.PlayHand(1, userStoppedGame);
			}
			break;
		case ID_FILE_MOVE:
			gGame.Move();
			break;
		case ID_FILE_UNDO:
			gGame.Undo();
			break;
		case ID_FILE_PLAYUNTILWIN:
			gGame.PlayUntilWin();
			break;
		case ID_FILE_PLAYUNTIL:
			gGame.PlayUntil();
			break;
		case ID_FILE_OPTIONS:
			gGame.Options();
			break;
		case ID_FILE_REPLAYSAMEDECK:
			gGame.RedealSameDeck();
			break;
		case ID_FILE_EXIT:
		case IDM_EXIT:
			DestroyWindow(hWnd);
			break;
		case ID_VIEW_BIGGER:
			ResizeCard(1);
			break;
		case ID_VIEW_SMALLER:
			ResizeCard(-1);
			break;
		case ID_VIEW_DEFAULT:
			ResizeCard(0);
			break;
		case ID_VIEW_CLEARSTATISTICS:
			gStats.Reset();
			break;
		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
		}
		break;
	case WM_PAINT:
		{
		PAINTSTRUCT ps;
		HDC hdc = BeginPaint(hWnd, &ps);
		
		gGame.DrawGame(hdc, 0, 0, gDrawLabels);

		EndPaint(hWnd, &ps);
		break;
		}

	// No backgroud erasing, we just redraw the whole client area.
	case WM_ERASEBKGND:
		return 1;

	case WM_LBUTTONDBLCLK:
		if (!gGame.MouseDoubleClick(LOWORD(lParam), HIWORD(lParam)))
			gGame.MouseDown(LOWORD(lParam), HIWORD(lParam));
		break;

	case WM_LBUTTONDOWN:
		gGame.MouseDown(LOWORD(lParam), HIWORD(lParam));
		break;

	case WM_RBUTTONDOWN:
		gGame.RightMouseDown(LOWORD(lParam), HIWORD(lParam));
		break;

	case WM_MOUSEMOVE:
		gGame.MouseMove(LOWORD(lParam), HIWORD(lParam));
		break;

	case WM_LBUTTONUP:
		gGame.MouseUp(LOWORD(lParam), HIWORD(lParam));
		break;

	case WM_SIZE:
		// Save the new width and height of the client area. 
		gClientX = LOWORD(lParam); 
		gClientY = HIWORD(lParam);
		gStats.Resize(gClientX, gClientY);
		BigCardWindow::Resize(gClientX, gClientY);
		return 0;

//	case WM_CREATE:
//		break;

	case WM_DESTROY:
		Quiting();
		break;


	case WM_KEYDOWN:
		if (BigCardWindow::IsWindowVisible())
			BigCardWindow::ChangeCard(wParam);
		break;

	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

void ResizeCard(int32 direction)
{
	if (BigCardWindow::IsWindowVisible())
		BigCardWindow::ResizeCard(direction);
	else
		gGame.ResizeCard(direction);
}


void Quiting()
{
	WriteIniFile();

	gGame.QuitGame();
	PostQuitMessage(0);
}

void WriteIniFile()
{
	gStats.WriteStats();
	gStrategy.WriteStrategy();
	OptionsUtils::WriteOptions();

	gGame.WriteCardSize();

	WriteWindowPos();
}

void ReadIniFile()
{
	gStrategy.ReadStrategy();
	TString strategyLetters;
	gStrategy.GetStrategyLetters(strategyLetters);
	gStats.SetStrategyLetters(strategyLetters);

	OptionsUtils::ReadOptions();
	bool dealOneAtATime, allowPartialPileMoves, oneTimeThrough;
	OptionsUtils::GetOptions(dealOneAtATime, allowPartialPileMoves, oneTimeThrough);
	gGame.SetOptions(dealOneAtATime, allowPartialPileMoves, oneTimeThrough);

	gGame.ReadCardSize();
}

void PositionMainWindow()
{
	WINDOWPLACEMENT windowPlacement;

	IniFileAccess *iniFileAccess = SolitaireIniFile::GetIniFileAccess();

	if (iniFileAccess->GetStructure(gWindowPlacementStr, (uint8 *)&windowPlacement, sizeof(windowPlacement)))
		return;

	SetWindowPlacement(gMainWindow, &windowPlacement);
}

void WriteWindowPos()
{
	WINDOWPLACEMENT windowPlacement;
	windowPlacement.length = sizeof(WINDOWPLACEMENT);
	GetWindowPlacement(gMainWindow, &windowPlacement);

	IniFileAccess *iniFileAccess = SolitaireIniFile::GetIniFileAccess();

	iniFileAccess->SetStructure(gWindowPlacementStr, (uint8 *)&windowPlacement, sizeof(windowPlacement));
}
