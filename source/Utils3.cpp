// Copyright 2006-2007 Keypict
// All Rights Reserved.

// keypict includes utils.cpp, utils2.cpp and utils3.cpp
// installer includes utils2.cpp and utils3.cpp
// uninstaller includes utils3.cpp

#include "People.h"
#define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows headers
#include "windows.h"
#include "Utils.h"
#include "shlobj.h"

static TCHAR gKeypictString[] = _T("Keypict");


void Utils::FileParts(const TString &currentFilename, TString &currentDirectory, TString &baseFilename)
{
	// Get the current folder from the current filename.
	size_t index = currentFilename.find_last_of(_T("\\"));
	if (index != -1)
	{
		currentDirectory = currentFilename.substr(0, index+1);
		baseFilename = currentFilename.substr(index+1);
	}
}

int32 Utils::MessageBox(HWND owner, const TString &message, uint32 flags)
{
	return ::MessageBox(owner, message.c_str(), gKeypictString, flags);
}

#define kMaxMessage 1000 // The maximum message size in characters.

TString Utils::GetString(HINSTANCE instance, unsigned int id)
{
	TString message;
	message.resize(kMaxMessage);

	// See FormatMessage to get a different string based on the current language.

	int charsLoaded = LoadString(instance, id, &message[0], (int)message.size());
	message.resize(charsLoaded);

	return message;
}

/** Combine a path and a name to create a full path.
	@param folder is a folder name, with or without an ending slash.
	@param name is the name to append. If name is empty, the returned folder
		will have an ending slash.
	@param filename is returned with the result.  It is untouched on an error.
*/
void Utils::AppendName(const TString &folder, const TString &name, TString &filename)
{
	// Make sure the folder ends with a slash.
	// If there is no slash or the last character is not a slash,
	// append one.
	TString temp = folder;
	size_t index = temp.find_last_of(_T("\\"));
	if (index == -1 || index+1 != temp.size())
		temp += _T("\\");

	if (!name.empty())
		temp += name;

	if (temp.size() >= MAX_PATH)
		return;

	filename = temp;
}

void Utils::GetStandardFilename(int32 folderID, const TString &name, TString &filename)
{
	// Get the standard folder.
	TCHAR path[MAX_PATH];
	path[0] = 0;
	SHGetFolderPath(NULL, folderID, NULL, 0, path);
	if (!path[0])
		return;

	AppendName(path, name, filename);
}
