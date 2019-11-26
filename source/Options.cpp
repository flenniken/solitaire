// Copyright 2006-2007 Keypict
// All Rights Reserved.

#include "stdafx.h"
#include "people.h"
#include "Klondike.h"
#include <vector>
#include "Game.h"
#include <shlwapi.h>
#include <gdiplus.h>
#include "Stats.h"
#include "Options.h"
#include <WindowsX.h>
#include "Utils.h"
#include <commctrl.h>
#include <algorithm>
#include "SolitaireIniFile.h"
#include "DrawUtils.h"

class Options
{
public:
	Options();
	~Options();

	bool ShowDialog(bool &dealOneAtATime, bool &allowPartialPileMoves, bool &oneTimeThrough);
	void ReadOptions();
	void WriteOptions();
	void GetOptions(bool &dealOneAtATime, bool &allowPartialPileMoves, bool &oneTimeThrough) const;

private:
	static INT_PTR CALLBACK OptionsDialog(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
	void InitializeDialog(HWND hDlg);
	void SaveData(HWND hDlg);

	bool fDealOneAtATime;
	bool fAllowPartialPileMoves;
	bool fOneTimeThrough;
	bool fUserCanceled;
};

Options gOptions;

extern HWND gMainWindow;
extern HINSTANCE gInstance;

static int32 gX = -1; // Last position of dialog.
static int32 gY = -1;
static TCHAR gDealOneAtATimeStr[] = _T("Deal One At a Time");
static TCHAR gAllowPartialPileMovesStr[] = _T("Allow Partial Pile Moves");
static TCHAR gOneTimeThrough[] = _T("One Time Through");

static INT_PTR CALLBACK OptionsDialog(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
static void PositionDialog(HWND hDlg);
static void RememberPosition(HWND hDlg);

// Show the dialog and return true when the user cancels.
bool OptionsUtils::ShowDialog(bool &dealOneAtATime, bool &allowPartialPileMoves, bool &oneTimeThrough)
{
	return gOptions.ShowDialog(dealOneAtATime, allowPartialPileMoves, oneTimeThrough);
}

void OptionsUtils::ReadOptions()
{
	gOptions.ReadOptions();
}

void OptionsUtils::WriteOptions()
{
	gOptions.WriteOptions();
}

void OptionsUtils::GetOptions(bool &dealOneAtATime, bool &allowPartialPileMoves, bool &oneTimeThrough)
{
	gOptions.GetOptions(dealOneAtATime, allowPartialPileMoves, oneTimeThrough);
}

Options::Options() : 
	fDealOneAtATime(true),
	fAllowPartialPileMoves(true),
	fOneTimeThrough(true)
{
}

Options::~Options()
{
}

bool Options::ShowDialog(bool &dealOneAtATime, bool &allowPartialPileMoves, bool &oneTimeThrough)
{
	gOptions.fDealOneAtATime = dealOneAtATime;
	gOptions.fAllowPartialPileMoves = allowPartialPileMoves;
	gOptions.fOneTimeThrough = oneTimeThrough;

	DialogBox(gInstance, MAKEINTRESOURCE(IDD_OPTIONS), gMainWindow, OptionsDialog);

	dealOneAtATime = gOptions.fDealOneAtATime;
	allowPartialPileMoves = gOptions.fAllowPartialPileMoves;
	oneTimeThrough = gOptions.fOneTimeThrough;

	return gOptions.fUserCanceled;
}

INT_PTR CALLBACK Options::OptionsDialog(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_INITDIALOG:
		{
			gOptions.InitializeDialog(hDlg);
			return (INT_PTR)true;
		}

	case WM_DRAWITEM:
		switch (wParam)
		{
		case kPictureControl:
			DrawUtils::PaintControlPicture(hDlg, ((LPDRAWITEMSTRUCT)lParam)->hDC, kPictureControl, Card::kHearts);
			break;
		}
		break;

	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDOK:
			gOptions.SaveData(hDlg);
			// fall thru
		case IDCANCEL:
			RememberPosition(hDlg);
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
		}
	}
	return (INT_PTR)FALSE;
}

void Options::InitializeDialog(HWND hDlg)
{
	fUserCanceled = true;

	// Select the current options.
	Button_SetCheck(GetDlgItem(hDlg, fDealOneAtATime ? IDC_ONEATATIME : IDC_THREEATATIME), true);
	Button_SetCheck(GetDlgItem(hDlg, fAllowPartialPileMoves ? IDC_ALLOWPARTIALPILEMOVES : IDC_ANYCARD), true);
	Button_SetCheck(GetDlgItem(hDlg, fOneTimeThrough ? IDC_ONETIME : IDC_MULTIPLETIMES), true);

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

void Options::SaveData(HWND hDlg)
{
	fDealOneAtATime = (Button_GetState(GetDlgItem(hDlg, IDC_ONEATATIME))) ? true : false;
	fAllowPartialPileMoves = (Button_GetState(GetDlgItem(hDlg, IDC_ALLOWPARTIALPILEMOVES))) ? true : false;
	fOneTimeThrough = (Button_GetState(GetDlgItem(hDlg, IDC_ONETIME))) ? true : false;

	fUserCanceled = false;
}

void Options::ReadOptions()
{
	IniFileAccess *iniFileAccess = SolitaireIniFile::GetIniFileAccess();

	iniFileAccess->GetBoolean(gDealOneAtATimeStr, fDealOneAtATime);
	iniFileAccess->GetBoolean(gAllowPartialPileMovesStr, fAllowPartialPileMoves);
	iniFileAccess->GetBoolean(gOneTimeThrough, fOneTimeThrough);
}

void Options::WriteOptions()
{
	IniFileAccess *iniFileAccess = SolitaireIniFile::GetIniFileAccess();

	iniFileAccess->SetBoolean(gDealOneAtATimeStr, fDealOneAtATime);
	iniFileAccess->SetBoolean(gAllowPartialPileMovesStr, fAllowPartialPileMoves);
	iniFileAccess->SetBoolean(gOneTimeThrough, fOneTimeThrough);
}

void Options::GetOptions(bool &dealOneAtATime, bool &allowPartialPileMoves, bool &oneTimeThrough) const
{
	dealOneAtATime = fDealOneAtATime;
	allowPartialPileMoves = fAllowPartialPileMoves;
	oneTimeThrough = fOneTimeThrough;
}
