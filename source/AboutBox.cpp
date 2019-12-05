// Copyright 2006-2019 Flenniken
// All Rights Reserved.

#include "stdafx.h"
#include "people.h"
#include "Klondike.h"
#include <vector>
#include "Game.h"
#include <shlwapi.h>
#include <gdiplus.h>
#include "Stats.h"
#include "AboutBox.h"
#include <WindowsX.h>
#include "Utils.h"
#include <commctrl.h>
#include <algorithm>
#include "BigCard.h"
#include "Armadillo.h"

class AboutBox
{
public:
	AboutBox();
	~AboutBox();

	void ShowDialog();

private:
	static INT_PTR CALLBACK AboutBoxDialog(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
	void InitializeDialog(HWND hDlg);
};

AboutBox gAboutBox;

extern HWND gMainWindow;
extern HINSTANCE gInstance;
extern Game gGame;

static int32 gX = -1; // Last position of dialog.
static int32 gY = -1;

static INT_PTR CALLBACK AboutBoxDialog(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
static void PositionDialog(HWND hDlg);
static void RememberPosition(HWND hDlg);
static void SetEditboxNumber(HWND hDlg, uint32 numberGames);
static uint32 GetControlNumber(HWND hDlg, uint32 id);
static void EnableThree(HWND hDlg, uint32 enable, uint32 disable1, uint32 disable2);
static void PaintAboutPicture(HWND hDlg, HDC hdc);

// Show the dialog and return true when the user cancels.
void AboutBoxUtils::ShowDialog()
{
	gAboutBox.ShowDialog();
}

AboutBox::AboutBox()
{
}

AboutBox::~AboutBox()
{
}

void AboutBox::ShowDialog()
{
	DialogBox(gInstance, MAKEINTRESOURCE(IDD_ABOUTBOX), gMainWindow, AboutBoxDialog);
}

INT_PTR CALLBACK AboutBox::AboutBoxDialog(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_INITDIALOG:
		{
			gAboutBox.InitializeDialog(hDlg);
			return (INT_PTR)true;
		}
	case WM_COMMAND:
		if (!(LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL))
			break;
		// Fall thru

	case WM_LBUTTONDOWN:
		RememberPosition(hDlg);
		EndDialog(hDlg, LOWORD(wParam));
		return (INT_PTR)TRUE;

	case WM_RBUTTONDOWN:
//		ArmadilloUtils::ShowDialog(hDlg);
		break;

	// Draw the top border and logo.
	case WM_DRAWITEM:
		switch (wParam)
		{
		case IDC_ABOUTPICTURE:
			PaintAboutPicture(hDlg, ((LPDRAWITEMSTRUCT)lParam)->hDC);
			break;
		}
		break;

	// Set the background color of the dialog and of the dialog's static text.
	case WM_CTLCOLORDLG:
	case WM_CTLCOLORSTATIC:
		SetBkColor((HDC)wParam, RGB(0, 0, 0));
		SetTextColor((HDC)wParam, RGB(255, 255, 255));
		return (LRESULT)GetStockObject(NULL_BRUSH);
	}
	return (INT_PTR)FALSE;
}

void AboutBox::InitializeDialog(HWND hDlg)
{

	// Position the dialog at the last place it was.
	PositionDialog(hDlg);
}

void PositionDialog(HWND hDlg)
{
	if (gX == -1)
		Utils::CenterWindow(hDlg);
	else
	{
		RECT rect;
		GetWindowRect(hDlg, &rect);
		MoveWindow(hDlg, gX, gY, rect.right-rect.left, rect.bottom-rect.top, false);
	}
}

void RememberPosition(HWND hDlg)
{
	RECT rect;
	GetWindowRect(hDlg, &rect);
	gX = rect.left;
	gY = rect.top;
}

void PaintAboutPicture(HWND hDlg, HDC hdc)
{
	SetGraphicsMode(hdc, GM_ADVANCED);

	HWND hwndControl = GetDlgItem(hDlg, IDC_ABOUTPICTURE);
	RECT clientRect;
	GetClientRect(hwndControl, &clientRect);


	Card card(Card::kAce, Card::kSpades);
	DrawCardDirect(hdc, 0, 0, clientRect.right, clientRect.bottom, card);
}

