// Copyright 2006-2019 Flenniken
// All Rights Reserved.

#pragma once


// Work around to compiler error. 
// I think installing VC 2005 caused this error to appear in VC 2003.
struct IServiceProvider;


// Windows 2000
#define _WIN32_WINNT 0x0500

#ifdef _DEBUG
#define DEBUG
#endif

#ifndef UNICODE
#define UNICODE
#ifdef UNICODE
	#define _UNICODE
#endif
#endif

#include <string>
#include <tchar.h>

#ifdef UNICODE
	typedef std::wstring TString;
#else
	typedef std::string TString;
#endif

typedef std::string UTF8String;

#define nil 0

typedef signed char int8;
typedef signed short int16;
typedef signed long int32;
typedef signed long long int64;

typedef unsigned char uint8;
typedef unsigned short uint16;
typedef unsigned long uint32;
typedef unsigned long long uint64;

