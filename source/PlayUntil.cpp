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
#include "PlayUntil.h"
#include <WindowsX.h>
#include "Utils.h"
#include <commctrl.h>
#include <algorithm>
#include "DrawUtils.h"

class PlayUntil
{
public:
	PlayUntil();
	~PlayUntil();

	bool ShowDialog(uint32 &numberGames, PlayUntilUtils::Type &type);

private:
	static INT_PTR CALLBACK PlayUntilDialog(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
	void InitializeDialog(HWND hDlg);
	void SaveData(HWND hDlg);

	uint32 fNumber;
	PlayUntilUtils::Type fType;
	bool fUserCanceled;
};

PlayUntil gPlayUntil;

extern HWND gMainWindow;
extern HINSTANCE gInstance;

static int32 gX = -1; // Last position of dialog.
static int32 gY = -1;
static uint32 gPlayGames = 100;
static uint32 gWinGames = 5;
static uint32 gPlayCardsUp = 12;
static uint32 gCurrentRadioButton = 0;

static INT_PTR CALLBACK PlayUntilDialog(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
static void PositionDialog(HWND hDlg);
static void RememberPosition(HWND hDlg);
static void SetEditboxNumber(HWND hDlg, uint32 id, uint32 numberGames);
static uint32 GetControlNumber(HWND hDlg, uint32 id);
static void EnableThree(HWND hDlg, uint32 enable, uint32 disable1, uint32 disable2);
static void SelectRadioButton(HWND hDlg);

// Show the dialog and return true when the user cancels.
bool PlayUntilUtils::ShowDialog(uint32 &number, Type &type)
{
	return gPlayUntil.ShowDialog(number, type);
}

PlayUntil::PlayUntil()
{
}

PlayUntil::~PlayUntil()
{
}

bool PlayUntil::ShowDialog(uint32 &number, PlayUntilUtils::Type &type)
{
	DialogBox(gInstance, MAKEINTRESOURCE(IDD_PLAYUNTIL), gMainWindow, PlayUntilDialog);

	number = gPlayUntil.fNumber;
	type = gPlayUntil.fType;

	return gPlayUntil.fUserCanceled;
}

INT_PTR CALLBACK PlayUntil::PlayUntilDialog(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_INITDIALOG:
		{
			gPlayUntil.InitializeDialog(hDlg);

			// Select all the text in the editbox.
			HWND editbox = GetDlgItem(hDlg, IDC_PLAYUNTILEDITBOX);
			Edit_SetSel(editbox, 0, 100);

			SetFocus(editbox);
			return (INT_PTR)false;
		}
	
	case WM_DRAWITEM:
		switch (wParam)
		{
		case kPlayUntilPicture:
			DrawUtils::PaintControlPicture(hDlg, ((LPDRAWITEMSTRUCT)lParam)->hDC, kPlayUntilPicture, Card::kClubs);
			break;
		}
		break;

	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDC_PLAYUNTILGAMES: // radio button
			EnableThree(hDlg, IDC_PLAYUNTILEDITBOX, IDC_PLAYUNTILCOMBOBOX, IDC_PLAYUNTILEDITBOX2);
			break;
		case IDC_PLAYUNTILCARDSUP: // radio button
			EnableThree(hDlg, IDC_PLAYUNTILCOMBOBOX, IDC_PLAYUNTILEDITBOX, IDC_PLAYUNTILEDITBOX2);
			break;
		case IDC_PLAYUNTILWINMANY:
			EnableThree(hDlg, IDC_PLAYUNTILEDITBOX2, IDC_PLAYUNTILEDITBOX, IDC_PLAYUNTILCOMBOBOX);
			break;
		case IDOK:
			gPlayUntil.SaveData(hDlg);
			// fall thru
		case IDCANCEL:
			RememberPosition(hDlg);
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
		}
	}
	return (INT_PTR)FALSE;
}

void EnableThree(HWND hDlg, uint32 enable, uint32 disable1, uint32 disable2)
{
	EnableWindow(GetDlgItem(hDlg, enable), true);
	EnableWindow(GetDlgItem(hDlg, disable1), false);
	EnableWindow(GetDlgItem(hDlg, disable2), false);
}

void PlayUntil::InitializeDialog(HWND hDlg)
{
	fUserCanceled = true;

	SetEditboxNumber(hDlg, IDC_PLAYUNTILEDITBOX, gPlayGames);

	// Select one of the radio buttons.
	SelectRadioButton(hDlg);

	switch (gCurrentRadioButton)
	{
		case 0: // radio button
			EnableThree(hDlg, IDC_PLAYUNTILEDITBOX, IDC_PLAYUNTILCOMBOBOX, IDC_PLAYUNTILEDITBOX2);
			break;
		case 1: // radio button
			EnableThree(hDlg, IDC_PLAYUNTILCOMBOBOX, IDC_PLAYUNTILEDITBOX, IDC_PLAYUNTILEDITBOX2);
			break;
		case 2:
			EnableThree(hDlg, IDC_PLAYUNTILEDITBOX2, IDC_PLAYUNTILEDITBOX, IDC_PLAYUNTILCOMBOBOX);
			break;
	}

	// Fill in the combobox.
	HWND combobox = GetDlgItem(hDlg, IDC_PLAYUNTILCOMBOBOX);
	for (int32 i = 0; i <= 52; i++)
	{
		TString string = Utils::GetNumberString(i);
		ComboBox_AddString(combobox, string.c_str());
	}

	ComboBox_SetCurSel(combobox, gPlayCardsUp);


	SetEditboxNumber(hDlg, IDC_PLAYUNTILEDITBOX2, gWinGames);

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

uint32 gRadioID[] = {IDC_PLAYUNTILGAMES, IDC_PLAYUNTILCARDSUP, IDC_PLAYUNTILWINMANY};

void SelectRadioButton(HWND hDlg)
{
	HWND radio = GetDlgItem(hDlg, gRadioID[gCurrentRadioButton]);
	if (!radio)
		return;
	Button_SetCheck(radio, true);
}


void RememberPosition(HWND hDlg)
{
	RECT rect;
	GetWindowRect(hDlg, &rect);
	gX = rect.left;
	gY = rect.top;
}

void PlayUntil::SaveData(HWND hDlg)
{
	// Determine which radio button is checked.
	HWND radio = GetDlgItem(hDlg, IDC_PLAYUNTILGAMES);
	if (Button_GetState(radio))
	{
		fNumber = GetControlNumber(hDlg, IDC_PLAYUNTILEDITBOX);
		fType = PlayUntilUtils::kGames;
		gPlayGames = fNumber;
		gCurrentRadioButton = 0;
	}
	else if (Button_GetState(GetDlgItem(hDlg, IDC_PLAYUNTILCARDSUP)))
	{
		fNumber = GetControlNumber(hDlg, IDC_PLAYUNTILCOMBOBOX);
		fType = PlayUntilUtils::kCardsUp;
		gPlayCardsUp = fNumber;
		gCurrentRadioButton = 1;
	}
	else
	{
		fNumber = GetControlNumber(hDlg, IDC_PLAYUNTILEDITBOX2);
		fType = PlayUntilUtils::kWins;
		gWinGames = fNumber;
		gCurrentRadioButton = 2;
	}

	fUserCanceled = false;
}

uint32 GetControlNumber(HWND hDlg, uint32 id)
{
	HWND control = GetDlgItem(hDlg, id);
	if (!control)
		return 0;

	// Get the number from the control.
	TCHAR buffer[100];
	Edit_GetText(control, buffer, sizeof(buffer));

	int number = _ttoi(buffer);
	if (number < 0)
		number = 0;

	return number;
}

void SetEditboxNumber(HWND hDlg, uint32 id, uint32 numberGames)
{
	HWND editbox = GetDlgItem(hDlg, id);
	if (!editbox)
		return;

	TString string = Utils::GetNumberString((int32)numberGames);

	// Add the number to the editbox.
	Edit_SetText(editbox, string.c_str());
}
