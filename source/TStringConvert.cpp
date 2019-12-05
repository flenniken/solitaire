// Copyright 2006-2019 Flenniken
// All Rights Reserved.

#include "People.h"
#include "TStringConvert.h"
#include "ConvertUTF.h"
#define HAS_AUTO_PTR_ETC 1
#include <memory>

static void UTF8StringToWString(const UTF8String &utf8String, std::wstring &string);
static void WStringToUTF8String(const std::wstring &string, UTF8String &utf8String);
static int TestConvert(const UTF8String &string8, const std::wstring &string16);

void TStringConvert::TStringToWString(const TString &string, std::wstring &wstring)
{
#ifdef UNICODE
	wstring = string;
#else
	UTF8StringToWString(string, wstring);
#endif
}

void UTF8StringToWString(const UTF8String &utf8String, std::wstring &string)
{
	size_t count = utf8String.size();
	const UTF8 *sourceStart = (UTF8*)utf8String.c_str();
	const UTF8 *sourceEnd = sourceStart+count;

	// Allocate memory and free it going out of scope.
	std::auto_ptr<UTF16> buffer((UTF16 *)malloc(count*2));
	UTF16 *bufferPtr = buffer.get();
	if (!bufferPtr)
		return; // out of memory

	UTF16 *targetStart = bufferPtr;
	UTF16 *targetEnd = targetStart + count;

	ConversionResult result = ConvertUTF8toUTF16(&sourceStart, sourceEnd, &targetStart, targetEnd, lenientConversion);
	if (result != conversionOK)
		return;

	// Determine the number of resulting characters.
	// targetStart points to the end after conversion.
	uint32 targetCount = (uint32)(targetStart - bufferPtr);
	string.assign((wchar_t *)bufferPtr, targetCount);
}

void WStringToUTF8String(const std::wstring &string, UTF8String &utf8String)
{
	size_t count = string.size();
#if (_MSC_VER >= 1310)
	const UTF16 *sourceStart = (UTF16 *)string.c_str();
#else
	const UTF16 *sourceStart = string.c_str();
#endif
	const UTF16 *sourceEnd = sourceStart+count;

	// Allocate memory and free it going out of scope.
	// Allocate 4 times the count, the maximum it could expand.
	std::auto_ptr<uint8> buffer((uint8 *)malloc(count*4));
	uint8 *bufferPtr = buffer.get();
	if (!bufferPtr)
		return; // out of memory

	UTF8 *targetStart = bufferPtr;
	UTF8 *targetEnd = targetStart + count*4;

	ConversionResult result = ConvertUTF16toUTF8(&sourceStart, sourceEnd, &targetStart, targetEnd, lenientConversion);
	if (result != conversionOK)
		return;

	// Determine the number of resulting bytes.
	// targetStart points to the end after conversion.
	uint32 targetCount = (uint32)(targetStart - bufferPtr);
	utf8String.assign((char *)bufferPtr, targetCount);
}

void TStringConvert::UTF8StringToTString(const UTF8String &utf8String, TString &string)
{
#ifdef UNICODE
	UTF8StringToWString(utf8String, string);
#else
	string = utf8String;
#endif
}

void TStringConvert::TStringToUTF8String(const TString &string, UTF8String &utf8String)
{
#ifdef UNICODE
	WStringToUTF8String(string, utf8String);
#else
	utf8String = string;
#endif
}

void TStringConvert::WStringToTString(const std::wstring &wstring, TString &string)
{
#ifdef UNICODE
	string = wstring;
#else
	WStringToUTF8String(wstring, string);
#endif
}

#ifdef DEBUG
int TStringConvert::Test()
{
	if (TestConvert("abcde", L"abcde"))
		return 1;

	if (TestConvert("A", L"A"))
		return 1;

	if (TestConvert("This is a longer string\nWith a newline in it.", L"This is a longer string\nWith a newline in it."))
		return 1;
	
	if (TestConvert("\xc3\x84x\xc3\xab", L"Äxë")) // c3 84 78 c3 ab
		return 1;

	return 0;
}

int TestConvert(const UTF8String &string8, const std::wstring &string16)
{
	std::wstring wString;
	UTF8String utf8String;

	WStringToUTF8String(string16, utf8String);
	if (utf8String != string8)
		return 1;
	UTF8StringToWString(utf8String, wString);
	if (wString != string16)
		return 1;
	return 0;
}

#endif
