// Copyright 2007-2019 Flenniken
// All Rights Reserved.

#include "stdafx.h"
#include "people.h"
#include "Klondike.h"
#include "MyBrush.h"


MyBrush::MyBrush(HDC hdc, const COLORREF &color, int style)
{
/*
HS_BDIAGONAL   A 45-degree upward, left-to-right hatch 
HS_CROSS   Horizontal and vertical crosshatch 
HS_DIAGCROSS   45-degree crosshatch 
HS_FDIAGONAL   A 45-degree downward, left-to-right hatch 
HS_HORIZONTAL   Horizontal hatch 
HS_VERTICAL   Vertical hatch
*/
	fHDC = hdc;

	LOGBRUSH logBrush;
	logBrush.lbStyle = BS_HATCHED;
	logBrush.lbColor = color;
	logBrush.lbHatch = style;

	fBrush = CreateBrushIndirect(&logBrush);
	fOldBrush = (HBRUSH)SelectObject(hdc, fBrush);

	fStockStyle = false;
}

MyBrush::MyBrush(int stockStyle)
{
	// BLACK_BRUSH, DKGRAY_BRUSH, GRAY_BRUSH, LTGRAY_BRUSH
	fBrush = (HBRUSH)GetStockObject(stockStyle);
	fStockStyle = true;
}

MyBrush::MyBrush(HDC hdc, COLORREF color)
{
	fBrush = (HBRUSH)CreateSolidBrush(color);
	fOldBrush = (HBRUSH)SelectObject(hdc, fBrush);
	fStockStyle = false;
}

MyBrush::~MyBrush()
{
	if (fStockStyle)
		return;

	SelectObject(fHDC, fOldBrush);
	DeleteObject(fBrush);
}

