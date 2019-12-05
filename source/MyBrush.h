// Copyright 2007-2019 Flenniken
// All Rights Reserved.

class MyBrush
{
public:
	MyBrush(HDC hdc, const COLORREF &color, int style);
	MyBrush(int stockStyle);
	MyBrush(HDC hdc, COLORREF color);
	~MyBrush();
	HBRUSH Get(){return fBrush;};
private:
	HDC fHDC;
	HBRUSH fBrush;	
	HBRUSH fOldBrush;
	bool fStockStyle;
};
