// Copyright 2007 Keypict
// All Rights Reserved.

#include "stdafx.h"
#include "people.h"
#include "Klondike.h"
#include <shlwapi.h>
#include "Gdiplus.h"
#include "DrawUtils.h"

void DrawSuitCentered(HDC hdc, const Card &card, int32 x, int32 y, uint32 width, bool flip);

void DrawUtils::MeasureString(Gdiplus::Graphics &graphics, const TString &string, const Gdiplus::Font &font, float &width, float &height)
{
	Gdiplus::StringFormat format;

	Gdiplus::SizeF layoutSize(1000, 200);
	Gdiplus::SizeF stringSize;
	graphics.MeasureString(string.c_str(), (int)string.size(), &font, layoutSize, &format, &stringSize);

	width = stringSize.Width;
	height = stringSize.Height;
}

void CardSizer::ResizeTheCard(uint32 currentWidth, uint32 currentHeight, int32 direction, uint32 &newWidth, uint32 &newHeight) const
{
	if (direction == 0)
	{
		newWidth = fDefaultCardWidth;
		newHeight = fDefaultCardHeight;
		return;
	}

	newWidth = currentWidth;
	newHeight = currentHeight;

	{
		uint32 amount = (uint32)((double)currentWidth * fResizeIncrement);

		uint32 width;
		if (direction > 0)
			width = currentWidth + amount; 
		else
			width = currentWidth - amount; 

		// Don't go over the maximum.
		if (width > fMaximumCardWidth)
			return;

		// Don't go under the minimum.
		if (width < fMinimumCardWidth)
			return;

		newWidth = width;
	}

	{
		uint32 hAmount = (uint32)((double)currentHeight * fResizeIncrement);

		uint32 height;
		if (direction > 0)
			height = currentHeight + hAmount; 
		else
			height = currentHeight - hAmount;

		newHeight = height;
	}
}

bool CardSizer::IsValidSizeCard(uint32 width, uint32 height) const
{
	if (width < fMinimumCardWidth || width > fMaximumCardWidth)
		return false;
//	if (height < asdf || height > asdf)
//		return;
	return true;
}

int DrawUtils::CreateBitmap(HWND hwnd, uint32 width, uint32 height, HBITMAP &bitmap)
{
	if (!hwnd)
		return 1;

	AutoReleaseDC autoReleaseDC;
	HDC hdc = autoReleaseDC.GetDC(hwnd);

	HBITMAP temp = CreateCompatibleBitmap(hdc, width, height);
	if (!temp)
		return 1; // out of memory

	if (bitmap)
		DeleteObject(bitmap);

	bitmap = temp;

	return 0; // success
}

void DrawUtils::PaintControlPicture(HWND hDlg, HDC hdc, uint32 id, Card::Suit suit)
{
	SetGraphicsMode(hdc, GM_ADVANCED);

	HWND hwndControl = GetDlgItem(hDlg, id);
	RECT clientRect;
	GetClientRect(hwndControl, &clientRect);

	Card card(Card::kAce, suit);
	DrawSuitCentered(hdc, card, clientRect.right/2, clientRect.bottom/2, clientRect.right-6, false);
}
