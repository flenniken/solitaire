// Copyright 2007-2019 Flenniken
// All Rights Reserved.

#include "stdafx.h"
#include "people.h"
#include "Klondike.h"
#include "Armadillo.h"
#include "Utils.h"
#include "TStringConvert.h"
#include "SolitaireIniFile.h"
// SoftwarePassport (Armadillo) http://siliconrealms.com/
#include "SecuredSections.h"

#pragma warning ( disable : 4748 ) // /GS can not protect parameters and local variables from local buffer overrun because optimizations are disabled in function

// Optimation are turned off for this file so we can use secure sections.
#define ArmadilloTrialBegin			SECUREBEGIN
#define ArmadilloTrialEnd			SECUREEND
#define ArmadilloFullBegin			SECUREBEGIN_B
#define ArmadilloFullEnd			SECUREEND_B

#if 0
	// This code appears in the full or unprotected version.
	ArmadilloFullBegin
	ArmadilloFullEnd

	// This only appears in the trial version or unprotected version.
	ArmadilloTrialBegin
	ArmadilloTrialEnd
#endif

typedef std::string UTF8String;

class Armadillo
{
public:
	Armadillo() : fLib(nil){}

	int InstallKey(const char *name, const char *codestring) ;
//	int ShowEnterKeyDialog(HWND parent);
	bool IsProtectedVersion();
	void GetNameAndKey(TString &userName, TString &keyValue);

private:
	int InstallArmadilloCode();

	HINSTANCE fLib;
};

class NameAndKey
{
public:
	void ShowDialog(HWND parent);

private:
	static INT_PTR CALLBACK NameAndKeyDialog(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
	void InitializeDialog(HWND hDlg);
	int SaveData(HWND hDlg);

	Armadillo fArmadillo;
};

extern HINSTANCE gInstance;

static NameAndKey gNameAndKey;
static int32 gX = -1; // Last position of dialog.
static int32 gY = -1;
static TString gUserName;
static TString gKeyValue;
static TCHAR gIniStrUserName[] = _T("User Name");
static TCHAR gIniStrKeyValue[] = _T("Key");

static void PositionDialog(HWND hDlg);
static void RememberPosition(HWND hDlg);
static void SetEditboxText(HWND hDlg, uint32 id, TString string);
static void MakeReadOnly(HWND hDlg, uint32 id);
static void GetEnvVariable(const TString &variableName, TString &value);

void ArmadilloUtils::ShowDialog(HWND parent)
{
	gNameAndKey.ShowDialog(parent);
}

void NameAndKey::ShowDialog(HWND parent)
{
	if (!fArmadillo.IsProtectedVersion())
		return; // unprotected version

	DialogBox(gInstance, MAKEINTRESOURCE(IDD_NAMEANDKEY), parent, NameAndKeyDialog);
}

INT_PTR CALLBACK NameAndKey::NameAndKeyDialog(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_INITDIALOG:
		gNameAndKey.InitializeDialog(hDlg);

		// Set the focus on the user name editbox (first control in dialog).
		return (INT_PTR)true; // Tell system to set the focus.

	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDOK:
			if (gNameAndKey.SaveData(hDlg))
				break;
			// fall thru on success
		case IDCANCEL:
			RememberPosition(hDlg);
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
		}
	}
	return (INT_PTR)FALSE;
}

int NameAndKey::SaveData(HWND hDlg)
{
	// This code appears in the full or unprotected version.
	ArmadilloFullBegin
		return 0; // success.  Already running full version, don't save anything.
	ArmadilloFullEnd

	// Get the name.
	UTF8String utf8Name;
	Utils::GetEditboxText(hDlg, kUserName, gUserName);
	TStringConvert::TStringToUTF8String(gUserName, utf8Name);

	// Get the key.
	UTF8String utf8key;
	Utils::GetEditboxText(hDlg, kKeyValue, gKeyValue);
	TStringConvert::TStringToUTF8String(gKeyValue, utf8key);

	// Save them to the ini file.
	IniFileAccess *iniFileAccess = SolitaireIniFile::GetIniFileAccess();
	iniFileAccess->SetString(gIniStrUserName, gUserName);
	iniFileAccess->SetString(gIniStrKeyValue, gKeyValue);

	// Try to install the name and key.
	int rc = fArmadillo.InstallKey(utf8Name.c_str(), utf8key.c_str());

	if (rc)
	{
		TString message = Utils::GetString(gInstance, kStrInvalidNameOrKey);
		Utils::MessageBox(hDlg, message, MB_OK | MB_ICONERROR);
	}
	else
	{
		TString message = Utils::GetString(gInstance, kStrGoodNameAndKey);
		Utils::MessageBox(hDlg, message, MB_OK);
	}

	return rc;
}

void NameAndKey::InitializeDialog(HWND hDlg)
{
	// This only appears in the trial version or unprotected version.
	ArmadilloTrialBegin
		// Get the name and key from the ini file.
		IniFileAccess *iniFileAccess = SolitaireIniFile::GetIniFileAccess();
		iniFileAccess->GetString(gIniStrUserName, gUserName);
		iniFileAccess->GetString(gIniStrKeyValue, gKeyValue);
	ArmadilloTrialEnd

	// This code appears in the full or unprotected version.
	ArmadilloFullBegin
		// Get the name and key from Armadillo.
		fArmadillo.GetNameAndKey(gUserName, gKeyValue);
	ArmadilloFullEnd

	SetEditboxText(hDlg, kUserName, gUserName);
	SetEditboxText(hDlg, kKeyValue, gKeyValue);

	// This code appears in the full or unprotected version.
	ArmadilloFullBegin
		// Already running full version. Make name and key readonly and hide instructions.
		ShowWindow(GetDlgItem(hDlg, kNameAndKeyInstructions), SW_HIDE);
		MakeReadOnly(hDlg, kUserName);
		MakeReadOnly(hDlg, kKeyValue);
	ArmadilloFullEnd

	// Position the dialog at the last place it was.
	PositionDialog(hDlg);
}

// Make the name and key read only.
void MakeReadOnly(HWND hDlg, uint32 id)
{
	HWND control = GetDlgItem(hDlg, id);
	SendMessage(control, EM_SETREADONLY, true, 0);
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


int Armadillo::InstallArmadilloCode() 
{ 
	if (fLib)
		return 0; // Success, already loaded.

	fLib = LoadLibraryA("ArmAccess.DLL");  
	if (!fLib) 
		return 1; // Couldn't load library

	return 0; // success
}


typedef bool (__stdcall *InstallKeyFn)(const char *name, const char *code); 

int Armadillo::InstallKey(const char *name, const char *codestring)
{
	if (InstallArmadilloCode())
		return 1;

	InstallKeyFn InstallKey = (InstallKeyFn)GetProcAddress(fLib, "InstallKey");  
	if (InstallKey == 0) 
		return 1; // Couldn't find the function.

	if (InstallKey(name, codestring) == 0)
		return 2; // not valid

	return 0; // success
} 

void Armadillo::GetNameAndKey(TString &userName, TString &keyValue)
{
	if (InstallArmadilloCode())
		return;

	// Get the user name that bought the license key.
	GetEnvVariable(_T("ALTUSERNAME"), userName);
	GetEnvVariable(_T("USERKEY"), keyValue);
} 

bool Armadillo::IsProtectedVersion()
{
	if (InstallArmadilloCode())
		return false;

	InstallKeyFn InstallKey = (InstallKeyFn)GetProcAddress(fLib, "InstallKey");  
	if (InstallKey == 0) 
		return false; // Couldn't find the function.

	return true;
} 

#if 0
typedef bool (__stdcall *ShowEnterKeyDialogFn)(HWND parent);  

int Armadillo::ShowEnterKeyDialog(HWND parent)
{
	if (InstallArmadilloCode())
		return 1;

	ShowEnterKeyDialogFn function = (ShowEnterKeyDialogFn)GetProcAddress(fLib, "ShowEnterKeyDialog");  
	if (function == 0) 
		return 1; // Couldn't find the function.

	Utils::MessageBox(parent, _T("testing"), MB_OK);

	if (function(parent) == 0)
		return 2; // not valid

	return 0; // success
} 
#endif

void SetEditboxText(HWND hDlg, uint32 id, TString string)
{
	HWND editbox = GetDlgItem(hDlg, id);
	if (!editbox)
		return;

	SetWindowText(editbox, string.c_str());
}

void GetEnvVariable(const TString &variableName, TString &value)
{
    TCHAR name[256]=_T("");
	GetEnvironmentVariable(variableName.c_str(), name, sizeof(name)/sizeof(TCHAR));
	value = name;
}
