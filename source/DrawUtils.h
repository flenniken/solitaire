// Copyright 2007-2019 Flenniken
// All Rights Reserved.

#pragma once

#include <shlwapi.h>
#include "Gdiplus.h"
#include "Card.h"

class DrawUtils
{
public:
	static void MeasureString(Gdiplus::Graphics &graphics, const TString &string, const Gdiplus::Font &font, float &width, float &height);

	/** Create a bitmap compatible with the given window.
		@param hwnd is a window.
		@param width is the width to create the bitmap.
		@param height is the height to create the bitmap.
		@param bitmap IN/OUT is return with the new bitmap.  
				Bitmap is deleted when not nil before assigning.
		@return 0 when successful. On failure the original bitmap is not deleted.
	*/
	static int CreateBitmap(HWND hwnd, uint32 width, uint32 height, HBITMAP &bitmap);

	/** Draw a suit in the client area of the given control.
	*/
	static void DrawUtils::PaintControlPicture(HWND hDlg, HDC hdc, uint32 id, Card::Suit suit);
};

class CardSizer
{
public:
	CardSizer(uint32 defaultCardWidth, uint32 defaultCardHeight, uint32 maximumCardWidth, uint32 minimumCardWidth, double resizeIncrement) : 
		fDefaultCardWidth(defaultCardWidth), 
		fDefaultCardHeight(defaultCardHeight), 
		fMaximumCardWidth(maximumCardWidth), 
		fMinimumCardWidth(minimumCardWidth), 
		fResizeIncrement(resizeIncrement)
		{}
		~CardSizer(){};

	void ResizeTheCard(uint32 currentWidth, uint32 currentHeight, int32 direction, uint32 &newWidth, uint32 &newHeight) const;
	bool IsValidSizeCard(uint32 width, uint32 height) const;

private:
	uint32 fDefaultCardWidth;
	uint32 fDefaultCardHeight;
	uint32 fMaximumCardWidth;
	uint32 fMinimumCardWidth;
	double fResizeIncrement;
};

class AutoSelectObject
{
public:
	AutoSelectObject(HDC hdc, HBITMAP bitmap) : fHDC(hdc), fBitmap(bitmap) {}
	~AutoSelectObject()
	{
		SelectObject(fHDC, fBitmap);
	}

private:
	HDC fHDC;
	HBITMAP fBitmap;
};

class AutoDeleteObject
{
public:
	AutoDeleteObject(HBITMAP bitmap) : fBitmap(bitmap) {}
	~AutoDeleteObject()
	{
		if (fBitmap)
			DeleteObject(fBitmap);
	}
	HBITMAP Get(){return fBitmap;}
	HBITMAP Release() 
	{
		HBITMAP temp = fBitmap;
		fBitmap = NULL;
		return temp;
	}

private:
	HBITMAP fBitmap;
};

class AutoDeleteDC
{
public:
	AutoDeleteDC(HDC hdc) : fHDC(hdc) {}
	~AutoDeleteDC()
	{
		if (fHDC)
			DeleteDC(fHDC);
	}
	HDC Get(){return fHDC;}
	HDC Release() 
	{
		HDC temp = fHDC;
		fHDC = NULL;
		return temp;
	}

private:
	HDC fHDC;
};

class AutoReleaseDC
{
public:
	AutoReleaseDC() : fWindow(nil), fHDC(nil) {}
	AutoReleaseDC(HWND window, HDC hdc) : fWindow(window), fHDC(hdc) {}
	~AutoReleaseDC()
	{
		if (fHDC)
			ReleaseDC(fWindow, fHDC);
	}
	HDC Get(){return fHDC;}

	HDC GetDC(HWND hwnd)
	{
		if (fHDC || fWindow)
			return nil;
		fWindow = hwnd;
		fHDC = ::GetDC(hwnd);
		SetGraphicsMode(fHDC, GM_ADVANCED);
		return fHDC;
	}

	HDC Release() 
	{
		HDC temp = fHDC;
		fHDC = NULL;
		return temp;
	}

private:
	HDC fHDC;
	HWND fWindow;
};

