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
#include "Strings.rc"
#include <vector>
#include "FileIO.h"
#include <algorithm>
#include "shlobj.h"
#include "TStringConvert.h"

#ifndef NOPAINT
#define xLeft 20 // Position where the keywords area starts.

extern HWND gMainWindow;
extern HINSTANCE gInstance;
extern int32 gClientX;
extern int32 gClientY;
extern uint32 gCharY;
#endif

static void GetExeFilename(TString &exeFilename);
static void GetUserDataFolder(TString &userDataFolder);
static void GetCountSize(HDC hdc, int32 number, uint32 &width, uint32 &height);

static TString gUserDataFolder;

void Utils::MakeFullPath(const TString &baseName, TString &fullPath)
{
	TString userDataFolder;
	GetUserDataFolder(userDataFolder);
	if (userDataFolder.empty())
		return;

	userDataFolder.append(baseName);

	if (userDataFolder.size() >= MAX_PATH)
		return;

	fullPath = userDataFolder;
}

void Utils::GetExeFilename(TString &exeFilename)
{
	TCHAR buffer[MAX_PATH];
	GetModuleFileName(NULL, buffer, sizeof(buffer));
	exeFilename = buffer;
}

// Get an array of keywords.
void Utils::ParseKeywords(const TString &keywords, std::vector<UTF8String> &keys)
{
	std::vector<TString> tKeys;
	ParseKeywords(keywords, tKeys);

	size_t count = tKeys.size();

	keys.clear();
	for (size_t i = 0; i < count; i++)
	{
		UTF8String utf8Keyword;
		TStringConvert::TStringToUTF8String(tKeys[i], utf8Keyword);

		keys.push_back(utf8Keyword);
	}
}

// Return a filename that doesn't exist in the same directory as the filename parameter.
int Utils::GetTempName(const TString &filename, TString &newFilename, const TString &extension)
{
	TString currentDirectory;
	TString baseFilename;
	Utils::FileParts(filename, currentDirectory, baseFilename);

	FileIO fp;

	for (int32 i = 0; i < 100; i++)
	{
		TString numberString = Utils::GetNumberString(i);

		newFilename = currentDirectory;
		newFilename.append(_T("kp"));
		newFilename.append(numberString);
		newFilename.append(extension);

		if (!fp.Exists(newFilename))
			return 0; // success
	}

	return 1; // Cannot get a temp name.
}

TString Utils::GetNumberString(int32 number)
{
	TCHAR buffer[256];
#if (_MSC_VER > 1310)
	_itot_s(number, buffer, sizeof(buffer)/sizeof(TCHAR), 10);
#else
	_itot(number, buffer, 10);
#endif
	return buffer;
}

// Get the names of all files in the specified parent folder.
void Utils::GetFileList(const TString &parentDirectory, std::vector<TString> &fileList, 
				std::vector<TString> &subDirList)
{
	fileList.clear();
	subDirList.clear();

	// Append *.* to the parent directory to get the filter.
	TString parentDirectoryFilter(parentDirectory);
	parentDirectoryFilter.append(_T("*.*"));

	WIN32_FIND_DATA findFileData;
	HANDLE handle = FindFirstFile(parentDirectoryFilter.c_str(), &findFileData);
	if (handle == INVALID_HANDLE_VALUE)
		return;

	while (1)
	{
		// Skip hidden, system and temporary files.
		if (!(findFileData.dwFileAttributes & (FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM | 
			FILE_ATTRIBUTE_TEMPORARY)))
		{
			TString name(findFileData.cFileName);

			// Add the files to the file list and sub directories to the sub dir list.
			if (findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
				subDirList.push_back(name);
			else
				fileList.push_back(name);
		}

		if (!FindNextFile(handle, &findFileData))
			break; // no more files.
	}

	FindClose(handle);

	// Sort the files alphabetically.
	std::sort(fileList.begin(), fileList.end(), CompareFilenamesLess);
	std::sort(subDirList.begin(), subDirList.end(), CompareFilenamesLess);
}

int Utils::FindFile(const std::vector<TString> &list, const TString &filename)
{
	if (!list.size())
		return -1; // no files
	std::vector<TString>::const_iterator it = std::lower_bound(list.begin(), list.end(), filename, CompareFilenamesLess);
	if (it == list.end())
		return -1;
	if (!CompareFilenames(*it, filename))
		return -1;
	return (int)(it - list.begin());
}

// Return true when filename one is less than filename two.
bool Utils::CompareFilenamesLess(const TString &one, const TString &two)
{
	// Use case insensitive compare.
	if (_tcsicmp(one.c_str(), two.c_str()) < 0)
		return true;
	return false;	
}

bool Utils::CompareFilenames(const TString &one, const TString &two)
{
	// Use case insensitive compare.
	if (_tcsicmp(one.c_str(), two.c_str()) == 0)
		return true;
	return false;	
}

/** Get the folder for user data.
	@param userDataFolder is filled in with the folder.
	userDataFolder is empty when there is an error.
*/
void GetUserDataFolder(TString &userDataFolder)
{
	// The first time after launching get the user data folder.
	if (gUserDataFolder.empty())
	{
		TString folder;
		Utils::GetStandardFilename(CSIDL_APPDATA, _T("Keypict\\"), folder);
		if (folder.empty())
			return;

		// Create the folder it it doesn't exist.
		CreateDirectory(folder.c_str(), NULL);
	
		gUserDataFolder = folder;
	}

	userDataFolder = gUserDataFolder;
}

// Get the size of the file in bytes.
uint64 Utils::GetFileSize(const TString &filename)
{
	WIN32_FIND_DATA findFileData;
	HANDLE handle = FindFirstFile(filename.c_str(), &findFileData);
	if (handle == INVALID_HANDLE_VALUE)
		return 0;

	uint64 fileSize = (((uint64)findFileData.nFileSizeHigh) << 32) + findFileData.nFileSizeLow;

	FindClose(handle);

	return fileSize;
}

#ifdef DEBUG
void Utils::WriteMetadataFile(const TString &baseName, const UTF8String &metadata)
{
	TString filename;
	Utils::MakeFullPath(baseName, filename);
	if (filename.empty())
		return;

	FileIO fp;
	if (fp.Open(filename, _T("wb")))
		return;
	fp.WriteBytes(&metadata[0], (FileIO::FileLength)metadata.size());
}
#endif

void Utils::MakeKeywordString(const std::vector<TString> &keys, TString &keywords)
{
	keywords.clear();
	size_t count = keys.size();

	for (size_t i = 0; i < count; i++)
	{
		if (i > 0)
			keywords += _T(", ");

		keywords += keys[i];
	}
}

#pragma warning ( disable : 4996 ) // warning C4996: 'wcscpy' was declared deprecated

/** Copy a string to a buffer.
	@param string to copy
	@param buffer points to the location to copy the string.
	@param bufferCount is the number of bytes in the buffer.
	@return a pointer to the buffer.
*/
TCHAR *Utils::CopyString(const TString &string, TCHAR *buffer, uint32 bufferSize)
{
	uint32 bufferCount = bufferSize / sizeof(TCHAR);
	if (bufferCount)
	{
		_tcsncpy(buffer, string.c_str(), bufferCount-1);
		buffer[bufferCount-1] = 0;
	}
	return buffer;
}

#ifndef NOPAINT
// Position where the keywords area starts.
#define xLeft 20

void Utils::GetIconBox(RECT &iconBox)
{
	iconBox.left = 0;
	iconBox.top = gClientY-gCharY;
	iconBox.right = xLeft;
	iconBox.bottom = gClientY;
}
#endif

#ifndef NOPAINT
void Utils::GetCountBoxes(HDC hdc, int current, int total, RECT &bigBox, RECT &starBox, RECT &countBox, RECT &totalBox)
{
	uint32 totalWidth;
	uint32 totalHeight;
	GetCountSize(hdc, total, totalWidth, totalHeight);

	uint32 currentWidth;
	uint32 currentHeight;
	GetCountSize(hdc, current, currentWidth, currentHeight);

	int xRight = gClientX-currentWidth-totalWidth-30-41;

	bigBox.left = xRight;
	bigBox.top = gClientY-gCharY;
	bigBox.right = gClientX;
	bigBox.bottom = gClientY;

	starBox.left = xRight+2;
	starBox.top = bigBox.top;
	starBox.right = xRight+2+54-4;
	starBox.bottom = gClientY;

	countBox.left = starBox.right+4;
	countBox.top = bigBox.top;
	countBox.right = starBox.right+4+currentWidth+3;
	countBox.bottom = gClientY;

	totalBox.left = gClientX-totalWidth-2;
	totalBox.top = bigBox.top;
	totalBox.right = gClientX;
	totalBox.bottom = gClientY;
}
#endif

void GetCountSize(HDC hdc, int32 number, uint32 &width, uint32 &height)
{
	TString numberString = Utils::GetNumberString(number);

	SIZE theSize;
	GetTextExtentPoint32(hdc, numberString.c_str(), (int)numberString.size(), &theSize);
	width = theSize.cx;
	height = theSize.cy;
}

#ifndef NOPAINT
void Utils::SetWindowTitle(const TString &metadataTitle, const TString &filename, const TString &message)
{
	TString title = Utils::GetString(gInstance, kStrAppTitle);

	// If there is a metadata title, append it to the title.
	if (!metadataTitle.empty())
	{
		title += _T(" - ");
		title += metadataTitle;
	}

	// If there is a filename, append it to the title.
	if (!filename.empty())
	{
		TString currentDirectory;
		TString baseFilename;
		Utils::FileParts(filename, currentDirectory, baseFilename);

		title += _T(" - ");
		title += baseFilename;
	}

	if (!message.empty())
	{
		title += _T(" - ");	
		title += message;
	}

	SetWindowText(gMainWindow, title.c_str());
}
#endif

void Utils::Orient(uint32 orientation, uint32 &width, uint32 &height)
{
	uint32 temp;
	switch (orientation)
	{
	default:
		break;
	case 5:
	case 6:
	case 7:
	case 8:
		temp = width;
		width = height;
		height = temp;
		break;
	}
}

int Utils::OpenReadClose(const TString &filename, uint32 fileOffset, uint32 length, UTF8String &data)
{
	if (!length)
		return 0; // Success, nothing to read.

	// Open for read.
	FileIO fp;
	if (fp.Open(filename, _T("rb")))
		return 1; // Unable to open the file.

	if (fp.Seek(fileOffset))
		return 1; // unable to seek

	if (fp.ReadBytes(data, length))
		return 1; // unable to read the bytes

	return 0; // success
}
