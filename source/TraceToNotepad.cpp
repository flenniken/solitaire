// Copyright 2006-2019 Flenniken
// All Rights Reserved.

#include "stdafx.h"
#include "people.h"
#include "TraceToNotepad.h"

#ifdef DEBUG

// Find an untitled notepad window's editbox.
void TraceToNotepad::Connect()
{
	if (IsWindow(fEditWindow))
		return;

	fEditWindow = 0;

	HWND hwnd = FindWindow(_T("Notepad"), _T("Untitled - Notepad"));
	if (!hwnd)
		return;

	fEditWindow = FindWindowEx(hwnd, NULL, _T("Edit"), NULL);
}

void TraceToNotepad::Write(const TString &string) const
{
	if (!fEditWindow)
		return;
	SendMessage(fEditWindow, EM_REPLACESEL, false, (LPARAM)string.c_str());
}

void TraceToNotepad::WriteNewline() const
{
	Write(_T("\r\n"));
}

#endif
