// Copyright 2007 Keypict
// All Rights Reserved.

#pragma once

class IDrawFunction
{
public:
	virtual int Draw(HDC hdc) const = 0;
};


class DragBits
{
public:
	DragBits();
	~DragBits();

	int GetBackground(int32 x, int32 y, uint32 width, uint32 height, IDrawFunction &drawFunction);
	int GetForeground(IDrawFunction &drawFunction);

//	HDC GetBackgroundHDC(int32 x, int32 y, uint32 width, uint32 height);
//	HDC GetForegroundHDC();
//	void GrabBackgroundBits(HDC hdc, int32 x, int32 y, uint32 width, uint32 height);
//	void GrabForegroundBits(HDC hdc);
	int MouseMove(HDC hdc, int32 x, int32 y);

private:
	int DrawToWorking(int32 wx, int32 wy, HBITMAP sourceBitmap, int32 x, int32 y);
	int GrabWorkingBits(int32 wx, int32 wy, HBITMAP destinationBitmap, int32 x, int32 y);
	int GrabScreen(HDC hdc, HBITMAP destination, int32 x, int32 y);
	int CreateBackgroundBits(int32 x, int32 y, uint32 width, uint32 height);
	int CreateForegroundBits();

	HBITMAP fBackground;
	HBITMAP fForeground;
	HBITMAP fWorking;
	uint32 fWorkingWidth;
	uint32 fWorkingHeight;
	int32 fx0;
	int32 fy0;
	uint32 fWidth;
	uint32 fHeight;
};

int DrawToBitmap(HBITMAP bitmap, IDrawFunction &drawFunction);
int DrawBitmap(HDC hdc, HBITMAP bitmap, int x, int y);
