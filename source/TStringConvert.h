// Copyright 2006-2007 Keypict
// All Rights Reserved.

#pragma once

#include <vector>

class TStringConvert
{
public:
	/** Convert TString to WString.
	*/
	static void TStringToWString(const TString &string, std::wstring &wstring);

	/** Convert WString to TString.
	*/
	static void WStringToTString(const std::wstring &wstring, TString &string);

	/** Convert UTF8String to TString.
	*/
	static void UTF8StringToTString(const UTF8String &utf8String, TString &string);

	/** Convert TString to UTF8String.
	*/
	static void TStringToUTF8String(const TString &string, UTF8String &utf8String);

#ifdef DEBUG
	static int Test();
#endif
};
