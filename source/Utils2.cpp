// Copyright 2006-2007 Keypict
// All Rights Reserved.

// keypict includes utils.cpp, utils2.cpp and utils3.cpp
// installer includes utils2.cpp and utils3.cpp
// uninstaller includes utils3.cpp

#include "People.h"
#define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows headers
#include <windows.h>
#include <WindowsX.h>
#include <shellapi.h>
#include "Utils.h"
#include "shlobj.h"

static int32 ResizeString(TString &string, int32 count);
static void StripSpaces(TString &string);

int32 Utils::GetResource(const ResourceID &resourceID, SimpleBuffer &simpleBuffer)
{
	// Find the resource.
	HRSRC handle = FindResource(resourceID.instance, MAKEINTRESOURCE(resourceID.id), resourceID.type.c_str());
	if (!handle)
		return 1; // Could not find the resource.

	// Get the size of the resource.
	DWORD resourceSize = SizeofResource(resourceID.instance, handle);
	if (!resourceSize)
		return 1; // The resource is empty.

	// Load the resource into memory.
	const void* resourceData = LockResource(LoadResource(resourceID.instance, handle));
	if (!resourceData)
		return 1;

	SimpleBuffer buffer(resourceData, resourceSize);
	simpleBuffer = buffer;

	return 0; // success
}

// Free an ITEMIDLIST.
void Utils::FreeItemIDList(ITEMIDLIST *itemIDList)
{
	IMalloc *pMalloc = NULL; 
	if (SUCCEEDED(SHGetMalloc(&pMalloc)) && pMalloc) 
	{  
		pMalloc->Free(itemIDList);  
		pMalloc->Release(); 
	}
}

/** Convert an ITEMIDLIST to a TString folder with an ending slash.
	@param itemIDList is the item list to convert.
	@param folder is returned with the item's folder.
	folder is untouched on an error.
*/
void Utils::GetFolderFromItemIDList(ITEMIDLIST *itemIDList, TString &folder)
{
	TCHAR buffer[MAX_PATH];
	if (!SHGetPathFromIDList(itemIDList, buffer))
		return;

	Utils::AppendName(buffer, _T(""), folder);
}

IStream *Utils::CreateStreamFromResource(const ResourceID &resourceID)
{
	SimpleBuffer simpleBuffer(nil, 0);

	if (GetResource(resourceID, simpleBuffer))
		return nil;

	return Utils::BufferToStream(simpleBuffer);
}

void Utils::OpenInDefault(const TString &string)
{
	SHELLEXECUTEINFO sei = { sizeof(sei) };
	sei.nShow = SW_SHOWNORMAL;
	sei.lpVerb = _T("Open");
	sei.lpFile = string.c_str();
	ShellExecuteEx(&sei);
}

void Utils::GetEditboxText(HWND hDlg, int editboxID, TString &string)
{
	HWND hwndControl = GetDlgItem(hDlg, editboxID);
	int length = Edit_GetTextLength(hwndControl);
	std::vector<TCHAR> buffer;
	buffer.resize(length+1);
	Edit_GetText(hwndControl, &buffer[0], length+1);
	string = &buffer[0];
}

void Utils::GetStringResource(const ResourceID &resourceID, TString &string)
{
	SimpleBuffer simpleBuffer(nil, 0);

	if (GetResource(resourceID, simpleBuffer))
		return;

	if (ResizeString(string, simpleBuffer.ByteCount()/sizeof(TCHAR)))
		return;

	simpleBuffer.Copy(&string[0]);
}

void Utils::CenterWindow(HWND hwndChild)
{
    RECT    rChild, rParent;
    int     wChild, hChild, wParent, hParent;
    int     wScreen, hScreen, xNew, yNew;
    HDC     hdc;

	HWND hwndParent = GetWindow(hwndChild, GW_OWNER);
	if (!hwndParent)
		hwndParent = GetDesktopWindow();

    // Get the Height and Width of the child window
    GetWindowRect(hwndChild, &rChild);
    wChild = rChild.right - rChild.left;
    hChild = rChild.bottom - rChild.top;

    // Get the Height and Width of the parent window
    GetWindowRect (hwndParent, &rParent);
    wParent = rParent.right - rParent.left;
    hParent = rParent.bottom - rParent.top;

    // Get the display limits
    hdc = GetDC(hwndChild);
    wScreen = GetDeviceCaps (hdc, HORZRES);
    hScreen = GetDeviceCaps (hdc, VERTRES);
    ReleaseDC(hwndChild, hdc);

    // Calculate new X position, then adjust for screen
    xNew = rParent.left + ((wParent - wChild) /2);
    if (xNew < 0) {
            xNew = 0;
    } else if ((xNew+wChild) > wScreen) {
            xNew = wScreen - wChild;
    }

    // Calculate new Y position, then adjust for screen
    yNew = rParent.top  + ((hParent - hChild) /2);
    if (yNew < 0) {
            yNew = 0;
    } else if ((yNew+hChild) > hScreen) {
            yNew = hScreen - hChild;
    }

    // Set it, and return
    SetWindowPos(hwndChild, NULL, xNew, yNew, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
}

/** Replace part of a string with another string.
	@param message is the string to change.
	@param findString is a sub-string of message to be replaced.
	@param replacementString is the replacement string.
*/
void Utils::ReplaceString(TString &message, const TString &findString, const TString &replacementString)
{
	size_t index = message.find(findString);
	if (index == -1)
		return; // findString not found.

	message.replace(index, findString.size(), replacementString);
}

#pragma warning( disable : 4800) // warning C4800: 'BOOL' : forcing value to bool 'true' or 'false' (performance warning)

bool Utils::IsWindows2000OrAbove()
{
#if 0
	// Make sure the major version is Windows 2000 or above.
	OSVERSIONINFO versionInfo;
	versionInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
	GetVersionEx(&versionInfo);
	if (versionInfo.dwMajorVersion < 5)
		return false;
#endif

	return true;
#if 0
	// VerifyVersionInfo only works on Windows 2000 or above.
	OSVERSIONINFOEX osvi;
	DWORDLONG dwlConditionMask = 0;

	ZeroMemory(&osvi, sizeof(OSVERSIONINFOEX));
	osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
	osvi.dwMajorVersion = 5;

	VER_SET_CONDITION(dwlConditionMask, VER_MAJORVERSION, VER_GREATER_EQUAL);

	return VerifyVersionInfo(&osvi, VER_MAJORVERSION | VER_MINORVERSION | VER_SERVICEPACKMAJOR,
		dwlConditionMask);
#endif
}

int32 ResizeString(TString &string, int32 count)
{
	int32 rc = 0;
	try
	{
		string.resize(count);
	}
	catch (...)
	{
		rc = 1;
	}
	return rc;
}

void Utils::ParseKeywords(const TString &keywords, std::vector<TString> &keys)
{
	keys.clear();

	size_t start = 0;
	size_t finish;
	bool done = false;

	while (!done)
	{
		// Find a keyword.
		TString value;
		finish = keywords.find(_T(","), start);
		if (finish == TString::npos)
		{
			value = keywords.substr(start);
			done = true;
		}
		else
		{
			value = keywords.substr(start, finish-start);
			start = finish+1;
		}

		StripSpaces(value);

		if (value.size())
			keys.push_back(value);
	}
}

// Remove leading and trailing white space from the string.
void StripSpaces(TString &string)
{
	size_t firstNonSpace = string.find_first_not_of(_T(" \t"));

	if (firstNonSpace == TString::npos)
	{
		// Empty or all spaces.
		string.clear();
		return;
	}

	size_t lastNonSpace = string.find_last_not_of(_T(" \t"));
	if (lastNonSpace == TString::npos)
		return;

	string = string.substr(firstNonSpace, lastNonSpace-firstNonSpace+1);
}

// Automatically free global memory when going out of scope.
// Call Release to prevent the free from happening.
class AutoFreeGlobal
{
public:
	AutoFreeGlobal(HGLOBAL hGlobal) : fhGlobal(hGlobal){}
	~AutoFreeGlobal()
	{
		if (fhGlobal)
			GlobalFree(fhGlobal);		
	}
	HGLOBAL Get() {return fhGlobal;}
	void Release() {fhGlobal = nil;}

private:
	HGLOBAL fhGlobal;
};

IStream *Utils::BufferToStream(const ISimpleBuffer &simpleBuffer)
{
	// Wow, allocate another buffer.

	// Allocate another memory buffer for the resource.
	// CreateStreamOnHGlobal requires this.
	uint32 byteCount = simpleBuffer.ByteCount();
	AutoFreeGlobal hGlobal(GlobalAlloc(GMEM_MOVEABLE, byteCount));
	if (!hGlobal.Get())
		return nil;

	void* hGlobalBuffer = ::GlobalLock(hGlobal.Get());
	if (!hGlobalBuffer)
		return nil;

	simpleBuffer.Copy(hGlobalBuffer);

	// Create an IStream out of the tiffMemoryStream.
	// The memory is freed when the stream is released.
	IStream *tiffStream;
	if (CreateStreamOnHGlobal(hGlobal.Get(), true, &tiffStream) != S_OK)
		return nil;

	// Don't free memory when going out of scope.
	hGlobal.Release();

	return tiffStream;
}

TString Utils::GetNumberString(uint64 number)
{
	TCHAR buffer[256];
#if (_MSC_VER > 1310)
	_ui64tot_s(number, buffer, sizeof(buffer)/sizeof(TCHAR), 10);
#else
	_ui64tot(number, buffer, 10);
#endif
	return buffer;
}

void Utils::Swap(unsigned short *value)
{
	unsigned char *ptr = (unsigned char *)value;
	unsigned char temp = ptr[0];
	ptr[0] = ptr[1];
	ptr[1] = temp;
}

void Utils::Swap(unsigned long *value)
{
	unsigned char *ptr = (unsigned char *)value;
	unsigned char temp = ptr[0];
	ptr[0] = ptr[3];
	ptr[3] = temp;
	temp = ptr[1];
	ptr[1] = ptr[2];
	ptr[2] = temp;
}

