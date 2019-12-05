// Copyright 2006-2019 Flenniken
// All Rights Reserved.

#include "People.h"
#define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows headers
#include <windows.h>
#include "SolitaireIniFile.h"
#include "Utils.h"
#include "FileIO.h"
#include <stdlib.h>

#define kSpecialInvalidUint32 0xffffffff
#define kSpecialInvalidInt32 0x7fffffff
#define kMaxStringLength 1000
#define kMaxBufferLength 1000

IniFileAccess gIniFileAccess(_T("Klondike.ini"), _T("Flenniken Klondike Solitaire"));

#ifdef DEBUG
bool gConstructorCalled;
#endif

IniFileAccess *SolitaireIniFile::GetIniFileAccess()
{
	// Make sure we are not calling this method too early in startup.
	_ASSERT(gConstructorCalled);

	return &gIniFileAccess;
}

IniFileAccess::IniFileAccess(const TString &baseName, const TString &sectionName)
{
	// Make a full path in the users data folder in "klondike" folder.
	Utils::MakeFullPath(baseName, fFilename);

	fSectionName = sectionName;

#ifdef DEBUG
	gConstructorCalled = true;
#endif
}

IniFileAccess::~IniFileAccess()
{
#ifdef DEBUG
	gConstructorCalled = false;
#endif
}

int IniFileAccess::GetBoolean(const TString &key, bool &value) const
{
	// Determine whether we are calling this method before or after it exists.
	_ASSERT(gConstructorCalled);

	if (fFilename.empty())
		return 1;

	// Get the key's value. If it doesn't exist, the default 2 is returned.
	// 2 isn't a valid boolean.
	UINT temp = GetPrivateProfileInt(fSectionName.c_str(), key.c_str(), 2, fFilename.c_str());
	if (temp == 2)
		return 1;

	if (temp)
		value = true;
	else
		value = false;

	return 0; // success
}

int IniFileAccess::SetBoolean(const TString &key, bool value)
{
	// Determine whether we are calling this method before or after it exists.
	_ASSERT(gConstructorCalled);

	if (fFilename.empty())
		return 1;

	if (WritePrivateProfileString(fSectionName.c_str(), key.c_str(), value ? _T("1") : _T("0"), fFilename.c_str()) == 0)
		return 1;

	return 0; // success
}


int IniFileAccess::GetInt32(const TString &key, int32 &value) const
{
	// Determine whether we are calling this method before or after it exists.
	_ASSERT(gConstructorCalled);

	if (fFilename.empty())
		return 1;

	// Get the key's value. 
	int32 temp = GetPrivateProfileInt(fSectionName.c_str(), key.c_str(), kSpecialInvalidInt32, fFilename.c_str());
	if (temp == kSpecialInvalidInt32)
		return 1;

	value = temp;

	return 0; // success
}

int IniFileAccess::SetInt32(const TString &key, int32 value)
{
	// Determine whether we are calling this method before or after it exists.
	_ASSERT(gConstructorCalled);

	if (fFilename.empty())
		return 1;

	if (value == kSpecialInvalidInt32)
		return 1;

	TString numberString = Utils::GetNumberString((uint64)value);

	if (WritePrivateProfileString(fSectionName.c_str(), key.c_str(), numberString.c_str(), fFilename.c_str()) == 0)
		return 1;

	return 0; // success
}


int IniFileAccess::GetUint32(const TString &key, uint32 &value) const
{
	// Determine whether we are calling this method before or after it exists.
	_ASSERT(gConstructorCalled);

	if (fFilename.empty())
		return 1;

	TCHAR returnedString[kMaxStringLength+1];
	if (!GetPrivateProfileString(fSectionName.c_str(), key.c_str(), _T(""), returnedString, sizeof(returnedString), fFilename.c_str()))
		return 1;

	if (returnedString[0] == 0)
		return 1;

	int64 number = _tstoi64(returnedString);
	if (number >= UINT_MAX || number < 0)
		return 1;

	value = (uint32)number;

	return 0; // success
}

int IniFileAccess::SetUint32(const TString &key, uint32 value)
{
	// Determine whether we are calling this method before or after it exists.
	_ASSERT(gConstructorCalled);

	if (fFilename.empty())
		return 1;

	if (value == kSpecialInvalidUint32)
		return 1;

	TString numberString = Utils::GetNumberString((uint64)value);

	if (WritePrivateProfileString(fSectionName.c_str(), key.c_str(), numberString.c_str(), fFilename.c_str()) == 0)
		return 1;

	return 0; // success
}

int IniFileAccess::GetString(const TString &key, TString &value) const
{
	// Determine whether we are calling this method before or after it exists.
	_ASSERT(gConstructorCalled);

	if (fFilename.empty())
		return 1;

	// Get the key's value. If it doesn't exist, the default "" is returned.
	// "" isn't allowed to be set.

	TCHAR returnedString[kMaxStringLength+1];
	if (!GetPrivateProfileString(fSectionName.c_str(), key.c_str(), _T(""), returnedString, sizeof(returnedString), fFilename.c_str()))
		return 1;

	if (returnedString[0] == 0)
		return 1;

	value = returnedString;

	return 0; // success
}

int IniFileAccess::SetString(const TString &key, const TString &value)
{
	// Determine whether we are calling this method before or after it exists.
	_ASSERT(gConstructorCalled);

	if (fFilename.empty())
		return 1;

	if (value.empty())
		return 1;

	if (value.size() > kMaxStringLength)
		return 1;

	if (WritePrivateProfileString(fSectionName.c_str(), key.c_str(), value.c_str(), fFilename.c_str()) == 0)
		return 1;

	return 0; // success
}

int IniFileAccess::GetStructure(const TString &key, uint8 *buffer, uint32 bufferSize) const
{
	// Determine whether we are calling this method before or after it exists.
	_ASSERT(gConstructorCalled);

	if (fFilename.empty())
		return 1;

	if (bufferSize == 0)
		return 1;

	if (bufferSize > kMaxBufferLength)
		return 1;

	if (GetPrivateProfileStruct(fSectionName.c_str(), key.c_str(), buffer, bufferSize, fFilename.c_str()) == 0)
		return 1;

	return 0; // success
}

int IniFileAccess::SetStructure(const TString &key, uint8 *buffer, uint32 bufferSize)
{
	// Determine whether we are calling this method before or after it exists.
	_ASSERT(gConstructorCalled);

	if (fFilename.empty())
		return 1;

	if (bufferSize == 0)
		return 1;

	if (bufferSize > kMaxBufferLength)
		return 1;

	if (WritePrivateProfileStruct(fSectionName.c_str(), key.c_str(), buffer, bufferSize, fFilename.c_str()) == 0)
		return 1;

	return 0; // success
}
